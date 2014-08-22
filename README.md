**C++ Automatic Mocks Injector**

## Introduction
C++ Automatic Mocks Injector is C++11 header only library providing following functionality:
* Automatically create required mocks
* Automatically inject mocks to tested classes via constructor
* Automatically register for required destructor's in case of smart pointers (supports testing of unique_ptr)
* Uses [HippoMocks](https://github.com/dascandy/hippomocks) as Mocking library
* Uses [DI](https://github.com/krzysztof-jusiak/di) as Dependency Injection library

### Unit Tests
```cpp
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

#include <mocks_injector.hpp>

int main() {
    //1. create mocks injector and example class
    auto _ = boost::di::make_mocks_injector();
    example sut{_, _, "hello world"};

    //2. set up expectations
    EXPECT_CALL(_, ilogic::do_it);
    EXPECT_CALL(_, ilogger::log).With("hello world");

    //3. run tests
    assert(0 == sut.run());
}
```

### Integration Tests
```cpp
struct logic : ilogic { void do_it() override { } };

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

    //1. create mocks injector with dependencies
    auto mi = di::make_mocks_injector(
        di::bind<std::string>::to("hello world")
      , di::bind<ilogic, logic>() // inject real logic
    );

    //2. set up expectations
    EXPECT_CALL(mi, ilogger::log).With("hello world");

    //3. create example class and run it
    assert(!mi.create<app>().run());
}
```

## License
Distributed under the [Boost Software License, Version 1.0](http://www.boost.org/LICENSE_1_0.txt).

