//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <cassert>
#include "mocks_provider.hpp"

namespace {

class ilogger {
public:
    virtual ~ilogger() = default;
    virtual void log(const std::string&) = 0;
};

class ilogic {
public:
    virtual ~ilogic() = default;
    virtual void do_it() = 0;
};

class example_sp {
public:
    example_sp(std::shared_ptr<ilogic> logic, std::shared_ptr<ilogger> logger)
        : logic_(logic), logger_(logger)
    { }

    void run() {
        logic_->do_it();
        logger_->log("hello world");
    }

private:
    std::shared_ptr<ilogic> logic_;
    std::shared_ptr<ilogger> logger_;
};

class example_sp_order {
public:
    example_sp_order(std::shared_ptr<ilogger> logger, std::shared_ptr<ilogic> logic)
        : logger_(logger), logic_(logic)
    { }

    void run() {
        logic_->do_it();
        logger_->log("hello world");
    }

private:
    std::shared_ptr<ilogger> logger_;
    std::shared_ptr<ilogic> logic_;
};

class example_sp_up {
public:
    example_sp_up(const std::shared_ptr<ilogger>& logger, std::unique_ptr<ilogic> logic)
        : logic_(std::move(logic)), logger_(logger)
    { }

    void run() {
        logic_->do_it();
        logger_->log("hello world");
    }

private:
    std::unique_ptr<ilogic> logic_;
    std::shared_ptr<ilogger> logger_;
};

class example_up_int {
public:
    example_up_int(std::unique_ptr<ilogic> logic, int i)
        : logic_(std::move(logic)), i_(i)
    { }

    void run() {
        logic_->do_it();
    }

    int get_int() const {
        return i_;
    }

private:
    std::unique_ptr<ilogic> logic_;
    int i_ = 0;
};

} // namespace

namespace di = boost::di;

int main() {
    auto test_mocks_injection_unit_test = [] {
        auto _ = di::make_injector<di::mocks_provider>();
        EXPECT_CALL(_, ilogger::log).With("hello world");
        EXPECT_CALL(_, ilogic::do_it);
        example_sp sut{_, _};
        sut.run();
    };

    auto test_mocks_injection_sp = [] {
        auto mi = di::make_injector<di::mocks_provider>();
        EXPECT_CALL(mi, ilogger::log).With("hello world");
        EXPECT_CALL(mi, ilogic::do_it);
        example_sp sut((mi));
        sut.run();
    };

    auto test_mocks_injection_sp_fail_due_to_empty_log = [] {
        bool exception = false;
        try {
            auto mi = di::make_injector<di::mocks_provider>();
            EXPECT_CALL(mi, ilogger::log).With("");
            EXPECT_CALL(mi, ilogic::do_it);
            mi.create<example_sp>().run();
        } catch(...) {
            exception = true;
        }
        assert(exception);
    };

    auto test_mocks_injection_sp_order = [] {
        auto mi = di::make_injector<di::mocks_provider>();
        EXPECT_CALL(mi, ilogger::log).With("hello world");
        EXPECT_CALL(mi, ilogic::do_it);
        mi.create<example_sp_order>().run();
    };

    auto test_mocks_injection_sp_up = [] {
        auto mi = di::make_injector<di::mocks_provider>();
        EXPECT_CALL(mi, ilogger::log).With("hello world");
        EXPECT_CALL(mi, ilogic::do_it);
        mi.create<std::unique_ptr<example_sp_up>>()->run();
    };

    auto test_mocks_injection_up_int = [] {
        using namespace boost::di;
        const int i = 42;
        auto mi = di::make_injector<di::mocks_provider>(
            bind<int>.to(i)
        );
        EXPECT_CALL(mi, ilogic::do_it);
        auto example = mi.create<std::unique_ptr<example_up_int>>();
        example->run();
        assert(i == example->get_int());
    };

    // run tests
    test_mocks_injection_unit_test();
    test_mocks_injection_sp();
    test_mocks_injection_sp_fail_due_to_empty_log();
    test_mocks_injection_sp_order();
    test_mocks_injection_sp_up();
    test_mocks_injection_up_int();

    return 0;
}

