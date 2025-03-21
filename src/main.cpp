#include "cassert"

import ywl.prelude;
import ywl.app.vm;

import ywl.miscellaneous.multithreading.thread_pool;

import ywl.overloads.std_pipe_operation;

using namespace ywl::miscellaneous::multithreading;

import ywl.basic.move_only_function;
import ywl.basic.algorithm;

template<typename T>
void foo(T &&t) {
    ywl::print_ln(__PRETTY_FUNCTION__);
}

int main_catch(int argc, char *argv[]) {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9};

    assert(*ywl::basic::first_to_satisfy(v.begin(), v.end(), [](const int &x) {
        return x > 5;
    }).value() == 6);

    assert(*ywl::basic::last_to_satisfy(v.begin(), v.end(), [](const int &x) {
        return x < 5;
    }).value() == 4);

    assert(ywl::basic::first_to_satisfy(v.begin(), v.end(), [](const int &x) {
        return x > 9;
    }) == std::nullopt);

    assert(ywl::basic::last_to_satisfy(v.begin(), v.end(), [](const int &x) {
        return x < 0;
    }) == std::nullopt);

    try {
        ywl::basic::first_to_satisfy(v.begin(), v.end(), [](const int &x) {
            return x < 5;
        });
    } catch (std::exception &e) {
        ywl::err_print_ln(e.what());
    }

    try {
        ywl::basic::last_to_satisfy(v.begin(), v.end(), [](const int &x) {
            return x > 5;
        });
    } catch (std::exception &e) {
        ywl::err_print_ln(e.what());
    }

    try {
        throw ywl::basic::runtime_error("Test exception");
    } catch (std::exception &e) {
        ywl::err_print_ln(e.what());
    }

    std::optional<int> opt = 1;

    static_assert(std::is_same_v < decltype(opt | ywl::overloads::constexpr_pipe<[](auto &&x) -> decltype(auto) {
        return std::forward<decltype(x)>(x).value();
    }>()), int &>);

    static_assert(std::is_same_v < decltype(std::move(opt) | ywl::overloads::constexpr_pipe
            <[]<typename T>(T &&x) -> decltype(auto) {
                return std::forward<T>(x).value();
            }>()), int &&>);

    assert(1 == (std::move(opt) | ywl::overloads::constexpr_pipe<[](auto &&x)
            -> decltype(auto) {
        return std::forward<decltype(x)>(x).value();
    }>()));

    std::vector<int> v1{1, 2, 3, 4, 5, 6, 7, 8, 9};

    v1 | ywl::overloads::ops::sort(std::greater < int > {});
    for (const auto &x: v1) {
        ywl::printf("{} ", x);
    }

    std::move(v1) | ywl::overloads::ops::map([](int &&x) {
        return x * x;
    });

    for (const auto &x: v1) {
        ywl::printf("{} ", x);
    }

    auto strings = v1
                   | ywl::overloads::ops::clone()
                   | ywl::overloads::ops::mapped([](int &&x) {
        return std::to_string(x);
    });

    strings | ywl::overloads::ops::move() | ywl::overloads::ops::for_each([](std::string &&s) {
        ywl::print_ln(s);
    });

    return 0;
}

int main(int argc, char **argv) {
    ywl::app::vm::run<main_catch>(argc, argv);
}
