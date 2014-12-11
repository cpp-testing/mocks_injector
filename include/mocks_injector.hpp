//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_MOCKS_INJECTOR_HPP
#define BOOST_DI_MOCKS_INJECTOR_HPP

#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <functional>
#include <unordered_map>
#define DEFAULT_AUTOEXPECT false
#include <hippomocks.h>
#include <boost/di.hpp>

namespace boost {
namespace di {

template<typename TInjector>
class mocks_injector {
public:
    mocks_injector() = default;

    explicit mocks_injector(const TInjector& injector)
        : injector_(injector)
    { }

    template<typename T>
    T create() const noexcept {
        return injector_.template create<T>();
    }

    template<typename T>
    operator T() const noexcept {
        return injector_.template create<T>();
    }

//private:
    TInjector injector_;
};

class mocks_config {
    class mock_provider {
    public:
        explicit mock_provider(mocks_config& cfg) noexcept
            : cfg_(cfg)
        { }

        template<class T, class TMemory, class... TArgs>
        std::enable_if_t<!std::is_polymorphic<di::aux::make_plain_t<T>>{}, T*>
        get(const type_traits::direct&, const TMemory&, TArgs&&... args) const noexcept {
            return new (std::nothrow) T(std::forward<TArgs>(args)...);
        }

        template<class T, class TMemory, class... TArgs>
        std::enable_if_t<std::is_polymorphic<di::aux::make_plain_t<T>>{}, T*>
        get(const type_traits::direct&, const TMemory&, TArgs&&...) const noexcept {
            return cfg_.acquire<di::aux::make_plain_t<T>>();
        }

        template<class T, class TMemory, class... TArgs>
        std::enable_if_t<!std::is_polymorphic<di::aux::make_plain_t<T>>{}, T*>
        get(const type_traits::aggregate&, const TMemory&, TArgs&&... args) const noexcept {
            return new (std::nothrow) T{std::forward<TArgs>(args)...};
        }

        template<class T, class TMemory, class... TArgs>
        std::enable_if_t<std::is_polymorphic<di::aux::make_plain_t<T>>{}, T*>
        get(const type_traits::aggregate&, const TMemory&, TArgs&&...) const noexcept {
            return cfg_.acquire<di::aux::make_plain_t<T>>();
        }

    private:
        mocks_config& cfg_;
    };

public:
    template<class T>
    T* acquire() const {
        auto it = mocks_.find(std::type_index(typeid(T)));
        if (it != mocks_.end()) {
            return static_cast<T*>(it->second);
        }

        auto* ptr = repository_.Mock<T>();
        mocks_[std::type_index(typeid(T))] = ptr;
        return ptr;
    }

    auto provider() const noexcept {
        return mock_provider{(mocks_config&)*this};
    }

    auto policies() const noexcept {
        return di::make_policies(
            [&](const auto& type) noexcept {
                using T = aux::make_plain_t<typename aux::make_plain_t<decltype(type)>::type>;
                if (std::is_polymorphic<T>{}) {
                    repository_.ExpectCallDestructor(acquire<T>());
                }
            }
        );
    }

    mutable MockRepository repository_;
    mutable std::unordered_map<std::type_index, void*> mocks_;
};

template<typename... TArgs>
auto make_mocks_injector(const TArgs&... args) {
    return mocks_injector<decltype(make_injector<mocks_config>(args...))>(make_injector<mocks_config>(args...));
}

} // namespace di
} // namespace boost

#define EXPECT_CALL(obj, func) \
    obj.injector_.config_.repository_.ExpectCall(obj.injector_.config_.acquire<typename boost::di::aux::function_traits<decltype(&func)>::base_type>(), func)

#endif

