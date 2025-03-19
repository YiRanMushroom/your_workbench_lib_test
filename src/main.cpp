import ywl.prelude;

import ywl.miscellaneous.multithreading.thread_pool;

using namespace ywl::miscellaneous::multithreading;

import ywl.basic.move_only_function;

int main(int argc, char *argv[]) {
    thread_pool pool{32};

    std::vector <std::future<int>> futures{32};
    for (size_t i = 0; i < 32; ++i) {
        futures[i] = pool.submit([i] {
            return static_cast<int>(i);
        });
    }

    auto other_pool{std::move(pool)};

    for (size_t i = 0; i < 32; ++i) {
        ywl::print_ln(futures[i].get());
    }

    using ywl::basic::move_only_function;

    move_only_function<int(int)> f{[](int x) {
        return x * x;
    }};

    try {
        move_only_function<int (*)(int)> other = std::move(f);
        other(0);

        f(0);
    } catch (std::exception &e) {
        ywl::print(e.what());
    }
}
