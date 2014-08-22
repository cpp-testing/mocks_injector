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
struct ilogger { virtual ~ilogger() { }; virtual void log(const std::string&) = 0; };
struct ilogic { virtual ~ilogic() { }; virtual void do_it() = 0; };

class example {
public:
    example(std::shared_ptr<ilogger> logger, const std::unique_ptr<ilogic>& logic)
        : logger_(logger), logic_(std::move(logic))
    { }

    void run() {
        logic_->do_it();
        logger_->log("hello world");
    }

private:
    std::shared_ptr<ilogger> logger_;
    std::unique_ptr<ilogic> logic_;
};

#include <mocks_injector.hpp>

int main() {
    //1. create mocks injector and example class
    auto _ = di::make_mocks_injector();
    example sut{_, _};

    //2. set up expectations
    EXPECT_CALL(_, ilogic::do_it);
    EXPECT_CALL(_, ilogger::log).With("hello world");

    //3. run tests
    sut.run();

    return 0;
}
```

### Integration Tests
```cpp

class app {
    explicit app(std::unique_ptr<example> e, bool flag)
        : exampe_(e), flag_(flag)
    { }

    void run() {
        if (flag_) {
            example_->run();
        }
    }

private:
    std::unique_ptr<example> exampe_;
    bool flag_ = false;
};

#include <mocks_injector.hpp>

int main() {
    //1. create mocks injector
    auto mi = di::make_mocks_injector(
        di::bind<bool>::to(true)
    );

    //2. set up expectations
    EXPECT_CALL(mi, ilogic::do_it);
    EXPECT_CALL(mi, ilogger::log).With("hello world");

    //3. create example class and run it
    mi.create<app>()->run();

    return 0;
}
```

## License
Distributed under the [Boost Software License, Version 1.0](http://www.boost.org/LICENSE_1_0.txt).

