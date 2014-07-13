#include <memory>
#include <type_traits>
#include <utility>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <hippomocks.h>
#include <boost/di.hpp>

namespace di = boost::di;

class ilogger {
public:
    virtual ~ilogger() {};
    virtual void log(const std::string&) = 0;
};

class example {
public:
    example(std::shared_ptr<ilogger> logger)
        : logger_(logger)
    { }

    void run() {
        logger_->log("hello world");
    }

    std::shared_ptr<ilogger> logger_;
};

struct mocks_injector : MockRepository
{
    template<typename T>
    struct allocator {
        allocator(mocks_injector& injector, std::vector<void*>& v)
            : injector(injector), v(v)
        { }

        template<typename TExpected, typename TGiven, typename... TArgs>
        typename boost::disable_if<boost::is_same<T, TExpected>, TExpected*>::type
        allocate(TArgs&&...) const {
             TExpected* ptr = (injector.Mock<TExpected>());
             v.push_back(static_cast<void*>(ptr));
             return ptr;
        }

        template<typename TExpected, typename TGiven, typename... TArgs>
        typename boost::enable_if<boost::is_same<T, TExpected>, TExpected*>::type
        allocate(TArgs&&... args) const {
            return new TGiven(std::forward<TArgs>(args)...);
        }

        mocks_injector& injector;
        std::vector<void*>& v;
    };

    template<typename T> T* get() const {
        return static_cast<T*>(v.back());
    }

    template<typename T> T create() {
        return di::make_injector().allocate<T>(allocator<T>(*this, v));
    }

   std::vector<void*> v;
};

int main()
{
    mocks_injector mi;
    auto ex = mi.create<example>();
    mi.ExpectCall(mi.get<ilogger>(), ilogger::log).With("hello world");
    mi.ExpectCallDestructor(mi.get<ilogger>());
    //mi.expect_call(&ilogger::log).With("Hello world");
    //mi.expect_call_destructor<ilogger>(); // register destructor in case of smart ptr via visitor
    ex.run();

    return 0;
}
