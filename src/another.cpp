/*
// google test
#include <gtest/gtest.h>

import ywl.prelude;

TEST(ThreadSafeQueue, Basic_Queue) {
    // this test is bad, it does not guarantee to work correctly even if the implementation is correct
    ywl::miscellaneous::multi_threading::thread_safe_queue<std::queue<int>> queue;

    std::atomic_int counter = 0;

    std::function<void()> pop = [&queue] {
        for (int i = 0; i < 1000; ++i) {
            ywl::print_ln("pop: ", queue.pop().value());
        }
    };

    std::function<void()> push = [&queue, &counter] {
        for (int i = 0; i < 1000; ++i) {
            queue.emplace(counter++);
        }
    };

    std::vector<std::jthread> threads_push;
    // 1000
    for (int i = 0; i < 500; ++i) {
        threads_push.emplace_back(push);
    }

    std::vector<std::jthread> threads_pop;
    // 1000
    for (int i = 0; i < 500; ++i) {
        threads_push.emplace_back(push);
        threads_pop.emplace_back(pop);
    }

    //
    for (int i = 0; i < 500; ++i) {
        threads_pop.emplace_back(pop);
    }
}

TEST(ThreadSafeQueue, Priority_Queue) {
    // this test is bad, it does not guarantee to work correctly even if the implementation is correct
    ywl::miscellaneous::multi_threading::thread_safe_queue<
        std::priority_queue<int, std::vector<int>, std::greater<int>>> queue;

    std::atomic_int counter = 0;

    std::function<void()> pop = [&queue] {
        for (int i = 0; i < 1000; ++i) {
            ywl::print_ln("pop: ", queue.pop().value());
        }
    };

    std::function<void()> push = [&queue, &counter] {
        for (int i = 0; i < 1000; ++i) {
            queue.emplace(counter++);
        }
    };

    std::vector<std::jthread> threads_push;
    // 1000
    for (int i = 0; i < 500; ++i) {
        threads_push.emplace_back(push);
    }

    std::vector<std::jthread> threads_pop;
    // 1000
    for (int i = 0; i < 500; ++i) {
        threads_push.emplace_back(push);
        threads_pop.emplace_back(pop);
    }

    for (int i = 0; i < 500; ++i) {
        threads_pop.emplace_back(pop);
    }
}

TEST(MPMC_Channel, Basic_Queue) {
    try {
        auto receiver =
                ywl::miscellaneous::multi_threading::make_simple_mpmc_channel<int>().second;

        std::atomic_int counter = 0;
        std::function<void()> send = [sender = receiver.subscribe(), &counter] {
            for (int i = 0; i < 100; ++i) {
                try {
                    sender.send(counter++);
                } catch (const std::exception &e) {
                    ywl::print_ln("send: ", e.what()).flush();
                    return;
                }
            }
        };

        std::atomic_bool finished = false;

        std::function<void()> receive = [receiver, &finished] {
            while (!finished) {
                auto value = receiver.receive_strong();
                if (value.has_value()) {
                    ywl::print_ln("receive: ", value.value());
                }
            }
        };

        std::vector<std::jthread> threads_receive; {
            std::vector<std::jthread> threads_send;

            for (int i = 0; i < 5; ++i) {
                threads_send.emplace_back(send);
            }

            for (int i = 0; i < 50; ++i) {
                threads_receive.emplace_back(receive);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        finished = true;
    } catch (std::exception &e) {
        ywl::print_ln("Exception: ", e.what());
    } catch (...) {
        ywl::print_ln("Unknown exception");
    }
}

TEST(Simple_Buffer, Basic) {
    static_assert(std::is_same_v<ywl::basic::reverse_index_sequence_t<3>, std::index_sequence<2, 1, 0>>);
    static_assert(std::is_same_v<ywl::basic::index_sequence_t<3>, std::index_sequence<0, 1, 2>>);

    ywl::miscellaneous::simple_buffer buffer{};

    std::tuple<int, double, std::string> tuple{1, 2.0, "3"};
    buffer.push_back(tuple);

    ywl::print_ln("buffer tuple: ", buffer.pop_back<std::tuple<int, double, std::string>>());
}
*/
