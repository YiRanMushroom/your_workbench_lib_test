module;

#include <cassert>

export module main;

import ywl.prelude;
import dev;

template<int value>
co_awaitable<int> await_int() {
    ywl::printf_ln("In await_int: {}", value);
    int v = co_await await_int<value - 1>();
    co_return value;
}

template<>
co_awaitable<int> await_int<0>() {
    ywl::print_ln("In await_int: 0");
    co_return 0;
}

co_awaitable<int> add_many() {
    int result{};
    {
        auto value = co_await await_int<1>();
        ywl::print_ln("value: ", value);
        result += value;
    }
    {
        auto value = co_await await_int<2>();
        ywl::print_ln("value: ", value);
        result += value;
    }
    {
        auto value = co_await await_int<3>();
        ywl::print_ln("value: ", value);
        result += value;
    }

    co_return result;
}

co_awaitable<int> add_so_many() {
    auto [r1, r2, r3, r4, r5] =
            co_await wait_all_of(await_int<1>(), await_int<2>(), await_int<3>(),
                                 await_int<4>(), await_int<5>());
    co_return r1 + r2 + r3 + r4 + r5;
}

co_awaitable<int> test_wait_any_of() {
    auto [r1, r2, r3, r4, r5] =
            co_await wait_any_of(await_int<5>(), await_int<2>(), await_int<3>(),
                                 await_int<4>(), await_int<1>());

    if (r1) {
        ywl::print_ln("r1: ", *r1);
        co_return *r1;
    } else if (r2) {
        ywl::print_ln("r2: ", *r2);
        co_return *r2;
    } else if (r3) {
        ywl::print_ln("r3: ", *r3);
        co_return *r3;
    } else if (r4) {
        ywl::print_ln("r4: ", *r4);
        co_return *r4;
    } else if (r5) {
        ywl::print_ln("r5: ", *r5);
        co_return *r5;
    }
}

int main_catch(int argc, char *argv[]) {
    /*co_executor executor;
    auto result = add_many();
    executor.initial_schedule_task(&result);
    executor.run();*/
    auto co_context = co_context::from_executor<simple_co_executor>();
    ywl::print_ln("result: ", co_context.block_on(add_so_many()));
/*        auto input_file1 = ywl::utils::read_or_create_file("input.txt");
        auto input_file2 = ywl::utils::read_or_create_file("input/input2.txt");

        auto output_file1 = ywl::utils::read_or_create_file("output.txt");
        auto output_file2 = ywl::utils::read_or_create_file("output/output2.txt");

        std::deque<int> v1 = {1, 2, 3, 4, 5};

        auto strings =
                v1 | ywl::overloads::ops::move() |
                ywl::overloads::ops::mapped_to<std::vector<std::string>>([](int &&i) { return std::to_string(i); });

        strings | ywl::overloads::ops::for_each([](const std::string &s) { ywl::printf("{} ", s); });*/

    return 0;
}

int main(int argc, char **argv) {
    //    ywl::app::vm::run<main_catch>(argc, argv);
    try {
        main_catch(argc, argv);
    } catch (std::exception &e) {
        ywl::err_printf_ln("Exception: {}", e.what());
    } catch (...) {
        ywl::err_printf_ln("Unknown exception");
    }
}
