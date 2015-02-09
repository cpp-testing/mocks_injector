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

#define BOOST_DI_TYPE_TRAITS_SCOPE_TRAITS_HPP
#include <boost/shared_ptr.hpp>
#include <boost/di/scopes/unique.hpp>
#include <boost/di/scopes/shared.hpp>
#include <boost/di/scopes/external.hpp>

namespace boost { namespace di { namespace type_traits {

template<class T>
struct scope_traits {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<T&> {
    using type = scopes::external;
};

template<class T>
struct scope_traits<const T&> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<T*> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<const T*> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<T&&> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<const T&&> {
    using type = scopes::unique;
};

template<class T, class TDeleter>
struct scope_traits<std::unique_ptr<T, TDeleter>> {
    using type = scopes::unique;
};

template<class T, class TDeleter>
struct scope_traits<const std::unique_ptr<T, TDeleter>&> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<std::shared_ptr<T>> {
    using type = scopes::shared;
};

template<class T>
struct scope_traits<const std::shared_ptr<T>&> {
    using type = scopes::shared;
};

#if (__has_include(<boost/shared_ptr.hpp>))
    template<class T>
    struct scope_traits<boost::shared_ptr<T>> {
        using type = scopes::shared;
    };

    template<class T>
    struct scope_traits<const boost::shared_ptr<T>&> {
        using type = scopes::shared;
    };
#endif

template<class T>
struct scope_traits<std::weak_ptr<T>> {
    using type = scopes::shared;
};

template<class T>
struct scope_traits<const std::weak_ptr<T>&> {
    using type = scopes::shared;
};

template<class T>
using scope_traits_t = typename scope_traits<T>::type;

}}} // boost::di::type_traits

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
                 , TArgs&&... args) {
            return new T{std::forward<TArgs>(args)...};
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
                 , TArgs&&...) {
            return self_.acquire<T>();
        }

        mocks_provider& self_;
    };

public:
    explicit mocks_provider(const TInjector& injector)
        : injector_(injector)
    { }

    auto provider() const noexcept {
        return mock_provider{(mocks_provider&)*this};
    }

    auto policies() noexcept {
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
    operator T() const {
        return injector_.template create<T>();
    }

    MockRepository& repository() {
        return repository_;
    }

    template<class T>
    T* acquire() {
        auto it = mocks_.find(std::type_index(typeid(T)));
        if (it != mocks_.end()) {
            return static_cast<T*>(it->second);
        }

        auto* ptr = repository_.Mock<T>();
        mocks_[std::type_index(typeid(T))] = ptr;
        return ptr;
    }

private:
    template<class T>
    void expect_dtor(const std::false_type& /*is_resolvable*/
                   , const std::true_type& /*is_smart_ptr*/
                   , const std::true_type& /*is_polymorphic*/) {
        repository_.ExpectCallDestructor(acquire<aux::decay_t<T>>());
    }

    template<class>
    void expect_dtor(...) { }

    const TInjector& injector_;
    std::unordered_map<std::type_index, void*> mocks_;
    MockRepository repository_;
};

} // namespace di
} // namespace boost

#define EXPECT_CALL(obj, func) \
    obj.repository().ExpectCall(obj.acquire<typename boost::di::aux::function_traits<decltype(&func)>::base_type>(), func)

#endif

