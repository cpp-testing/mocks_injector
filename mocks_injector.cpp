#include <memory>
#include <typeinfo>
#include <map>
#include <type_traits>
#include <utility>
#include <boost/assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/noncopyable.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <hippomocks.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/di.hpp>

namespace di = boost::di;

template<typename TInjector = di::injector<> >
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

    typedef std::map<
        const std::type_info*
      , void*
      , type_comparator
    > mocks_type;

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

        ~mock_policy() {
            for (auto it = v.begin(); it != v.end(); ++it) {
                (*it)();
            }
        }

        template<typename TDependency>
        typename boost::enable_if_c<boost::is_same<T, typename TDependency::type>::value>::type
        assert_policy() const { }

        template<typename TDependency>
        typename boost::disable_if_c<boost::is_same<T, typename TDependency::type>::value>::type
        assert_policy() const {
            v.push_back(boost::bind(&mock_policy::expect_call_destructor<typename TDependency::type>, this));
        }

    private:
        template<typename TExpected>
        typename boost::disable_if<has_element_type<typename di::type_traits::remove_accessors<TExpected>::type> >::type
        expect_call_destructor() const { }

        template<typename TExpected>
        typename boost::enable_if<has_element_type<typename di::type_traits::remove_accessors<TExpected>::type> >::type
        expect_call_destructor() const {
            using type = typename di::type_traits::remove_accessors<TExpected>::type;
            mocks_injector_.ExpectCallDestructor(mocks_injector_.template acquire<typename type::element_type>());
        }

        mocks_injector& mocks_injector_;
        mutable std::vector<boost::function<void()>> v;
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

        T* ptr = Mock<T>();
        mocks_[&typeid(T)] = static_cast<void*>(ptr);
        return ptr;
    }

private:
    mocks_type mocks_;
    TInjector injector_;
};

template<typename... TArgs>
auto make_mocks_injector(const TArgs&... args) {
    return mocks_injector<BOOST_DI_FEATURE_DECLTYPE(di::make_injector(args...))>(di::make_injector(args...));
}

#define EXPECT_CALL(obj, func) \
    obj.ExpectCall(obj.acquire<typename boost::di::type_traits::function_traits<BOOST_DI_FEATURE_DECLTYPE(&func)>::base_type>(), func)

/////////////////////////////////

class ilogger {
public:
    virtual ~ilogger() {};
    virtual void log(const std::string&) = 0;
};

class ilogic {
public:
    virtual ~ilogic() {};
    virtual void do_it() = 0;
};

class example {
public:
    example(std::unique_ptr<ilogic> logic, const std::shared_ptr<ilogger>& logger)
        : logic_(std::move(logic)), logger_(logger)
    { }

    void run() {
        logic_->do_it();
        logger_->log("hello world");
    }

    std::unique_ptr<ilogic> logic_;
    std::shared_ptr<ilogger> logger_;
};

int main()
{
    auto mi = make_mocks_injector();
    EXPECT_CALL(mi, ilogic::do_it);
    EXPECT_CALL(mi, ilogger::log).With("hello world");
    mi.create<std::unique_ptr<example>>()->run();

    return 0;
}

