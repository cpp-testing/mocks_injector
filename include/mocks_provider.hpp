//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_MOCKS_PROVIDER_HPP
#define BOOST_DI_MOCKS_PROVIDER_HPP

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

template<class TInjector>
class mocks_provider {
    struct not_resolved { };

    template<class T>
    using is_resolvable = std::integral_constant<
        bool
      , !std::is_same<
            not_resolved
          , decltype(core::binder::resolve<T, no_name, not_resolved>((TInjector*)nullptr))
        >{}
    >;

    struct mock_provider {
        template<
            class I
          , class T
          , class TInitialization
          , class TMemory
          , class... TArgs
          , std::enable_if_t<is_resolvable<I>{} || !std::is_polymorphic<T>{}, int> = 0
        > auto get(const TInitialization&
                 , const TMemory&
                 , TArgs&&... args) const {
            return new T(std::forward<TArgs>(args)...);
        }

        template<
            class I
          , class T
          , class TInitialization
          , class TMemory
          , class... TArgs
          , std::enable_if_t<!is_resolvable<I>{} && std::is_polymorphic<T>{}, int> = 0
        > auto get(const TInitialization&
                 , const TMemory&
                 , TArgs&&...) const {
            return self_.acquire<T>();
        }

        const mocks_provider& self_;
    };

public:
    explicit mocks_provider(const TInjector& injector)
        : injector_(injector)
    { }

    template<class T>
    operator T() const {
        return injector_.template create<T>();
    }

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
        return mock_provider{*this};
    }

    auto policies() const noexcept {
        return di::make_policies(
            [&](auto type) {
                using T = typename decltype(type)::type;
                expect_dtor<T>(
                    is_resolvable<aux::decay_t<T>>{}
                  , aux::is_smart_ptr<aux::remove_accessors_t<T>>{}
                  , std::is_polymorphic<aux::decay_t<T>>{}
                );
            }
        );
    };

    template<class T>
    void expect_dtor(const std::false_type& // is_resolvable
                   , const std::true_type& // is_smart_ptr
                   , const std::true_type& // is_polymorphic
    ) const {
        repository_.ExpectCallDestructor(acquire<aux::decay_t<T>>());
    }

    template<class>
    void expect_dtor(...) const { }

    mutable MockRepository repository_;

private:
    const TInjector& injector_;
    mutable std::unordered_map<std::type_index, void*> mocks_;
};

} // namespace di
} // namespace boost

#define EXPECT_CALL(obj, func) \
    obj.repository_.ExpectCall(obj.acquire<typename boost::di::aux::function_traits<decltype(&func)>::base_type>(), func)

#endif

