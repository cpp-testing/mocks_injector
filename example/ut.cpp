#include <string>
#include <memory>
#include <utility>

struct ilogger { virtual ~ilogger() { }; virtual void log(const std::string&) = 0; };
struct ilogic { virtual ~ilogic() { }; virtual void do_it() = 0; };

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

#include <mocks_provider.hpp>

int main() {
    namespace di = boost::di;

    //1. create mocks injector and example class
    const std::string hello_world = "hello world";
    auto _ = di::make_injector<di::mocks_provider>();

    //2. set-up expectations
    EXPECT_CALL(_, ilogic::do_it);
    EXPECT_CALL(_, ilogger::log).With(hello_world);

    //3. run tests
    example sut{_, _, hello_world};
    assert(!sut.run());
}

