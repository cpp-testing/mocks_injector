//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef MOCKS_INJECTOR_HPP
#define MOCKS_INJECTOR_HPP

#include <typeinfo>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/has_xxx.hpp>
#define DEFAULT_AUTOEXPECT false
#include <hippomocks.h>
#include <boost/di.hpp>

namespace di {

template<typename TInjector = boost::di::injector<> >
class mocks_injector
    : public MockRepository
{
    BOOST_MPL_HAS_XXX_TRAIT_DEF(element_type)

    class type_comparator {
    public:
        bool operator()(const std::type_info* lhs, const std::type_info* rhs) const {
            return lhs->before(*rhs);
        }
    };
    typedef std::map<const std::type_info*, void*, type_comparator> mocks_type;

    template<typename T>
    class mock_allocator : boost::noncopyable {
    public:
        mock_allocator(mocks_injector& mi, mocks_type& mocks)
            : mocks_injector_(mi), mocks_(mocks)
        { }

        template<typename TExpected, typename TGiven, typename... TArgs>
        typename boost::disable_if_c<
            boost::is_same<T, TExpected>::value || !boost::is_polymorphic<TExpected>::value
          , TExpected*
        >::type
        allocate(TArgs&&...) const {
            return mocks_injector_.acquire<TExpected>();
        }

        template<typename TExpected, typename TGiven, typename... TArgs>
        typename boost::enable_if_c<
            boost::is_same<T, TExpected>::value || !boost::is_polymorphic<TExpected>::value
          , TExpected*
        >::type
        allocate(TArgs&&... args) const {
            return new TGiven(std::forward<TArgs>(args)...);
        }

        mocks_injector& mocks_injector_;
        mocks_type& mocks_;
    };

    template<typename T>
    class mock_policy
    {
    public:
        mock_policy(mocks_injector& mi)
            : mocks_injector_(mi)
        { }

        template<typename TDependency>
        typename boost::enable_if_c<boost::is_same<T, typename TDependency::type>::value>::type
        assert_policy() const { }

        template<typename TDependency>
        typename boost::disable_if_c<boost::is_same<T, typename TDependency::type>::value>::type
        assert_policy() const {
            expect_call_destructor<typename TDependency::type>();
        }

    private:
        template<typename TExpected>
        typename boost::disable_if<has_element_type<typename boost::di::type_traits::remove_accessors<TExpected>::type> >::type
        expect_call_destructor() const { }

        template<typename TExpected>
        typename boost::enable_if<has_element_type<typename boost::di::type_traits::remove_accessors<TExpected>::type> >::type
        expect_call_destructor() const {
            using type = typename boost::di::type_traits::remove_accessors<TExpected>::type;
            mocks_injector_.ExpectCallDestructor(mocks_injector_.template acquire<typename type::element_type>());
        }

        mocks_injector& mocks_injector_;
    };

public:
    mocks_injector() { }

    explicit mocks_injector(const TInjector& injector)
        : injector_(injector)
    { }

    template<typename T>
    T create() {
        return injector_.template allocate<T>(mock_allocator<T>(*this, mocks_), mock_policy<T>(*this));
    }

    template<typename T> T* acquire() {
        typename mocks_type::const_iterator it = mocks_.find(&typeid(T));
        if (it != mocks_.end()) {
            return static_cast<T*>(it->second);
        }

        auto* ptr = Mock<T>();
        mocks_[&typeid(T)] = static_cast<void*>(ptr);
        return ptr;
    }

private:
    mocks_type mocks_;
    TInjector injector_;
};

template<typename... TArgs>
auto make_mocks_injector(const TArgs&... args) -> decltype(mocks_injector<decltype(boost::di::make_injector(args...))>(boost::di::make_injector(args...))) {
    return mocks_injector<decltype(boost::di::make_injector(args...))>(boost::di::make_injector(args...));
}

} // namespace di

#define EXPECT_CALL(obj, func) \
    obj.ExpectCall(obj.acquire<typename boost::di::type_traits::function_traits<decltype(&func)>::base_type>(), func)

#endif

