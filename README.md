**C++ Automatic Mocks Injector**

## Introduction
C++ Automatic Mocks Injector is C++11 header only library providing following functionality:
* Automatically create required mocks
* Automatically inject mocks to tested classes via constructor
* Automatically register for required destructor's in case of smart pointers (supports testing of unique_ptr)
* Uses [HippoMocks](https://github.com/dascandy/hippomocks) as Mocking library
* Uses [DI](https://github.com/krzysztof-jusiak/di) as Dependency Injection library

### Hello World
```cpp
#include <mocks_injector.hpp>

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

int main() {
    //1. create mocks injector
    auto mi = di::make_mocks_injector();

    //2. set up expectations
    EXPECT_CALL(mi, ilogic::do_it);
    EXPECT_CALL(mi, ilogger::log).With("hello world");

    //3. create example class and run it
    mi.create<example>().run(); // or mi.create<std::unique_ptr<example>>()->run();

    return 0;
}
```

## License
Distributed under the [Boost Software License, Version 1.0](http://www.boost.org/LICENSE_1_0.txt).

