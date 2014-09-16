#include <string>
#include <memory>
#include <utility>

struct ilogger { virtual ~ilogger() { }; virtual void log(const std::string&) = 0; };
struct ilogic { virtual ~ilogic() { }; virtual void do_it() = 0; };
struct logic : ilogic { void do_it() override { } };

class example {
public:
    example(const std::shared_ptr<ilogger>& logger
          , std::unique_ptr<ilogic> logic
          , const std::string& text)
        : logger_(logger)
        , logic_(std::move(logic))
        , text_(text)
    { }

    int run() {
        logic_->do_it();
        logger_->log(text_);
        return 0;
    }

private:
    std::shared_ptr<ilogger> logger_;
    std::unique_ptr<ilogic> logic_;
    std::string text_;
};

class app {
public:
    app(std::shared_ptr<example> e)
        : example_(e)
    { }

    int run() {
        return example_->run();
    }

private:
    std::shared_ptr<example> example_;
};

#include <mocks_injector.hpp>

int main() {
    namespace di = boost::di;

    const std::string hello_world = "hello world";

    //1. create mocks injector with dependencies
    auto mi = di::make_mocks_injector(
        di::bind<std::string>::to(hello_world)
      , di::bind<ilogic, logic>() // inject real logic
    );

    //2. set up expectations
    EXPECT_CALL(mi, ilogger::log).With(hello_world);

    //3. create example class and run it
    assert(!mi.create<app>().run());
}

