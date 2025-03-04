#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <sstream>

#include <cassert>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <unordered_set>

import ywl.all;

struct MyType {
    int value;

    [[nodiscard]] std::string to_string() const {
        return "{value: " + std::to_string(value) + "}";
    }
};

namespace ywl {
    template<>
    struct format_type_function_ud<MyType> {
        static std::string to_string(const MyType &t) {
            return t.to_string();
        }
    };
}

std::unordered_set<uint32_t> test_set;

struct int32_constructor_type {
    uint32_t operator()(int32_t value) const {
        test_set.insert(value);
        return value;
    }
};

struct int32_destructor_type {
    void operator()(uint32_t value) const {
        test_set.erase(value);
    }
};

struct holder_shared_int32_type {
    using holder_value_type = uint32_t;

    static inline std::mutex mutex;

    static void destroy_resource(holder_value_type &&value) {
        auto lock = std::scoped_lock{mutex};
        std::cout << "destroy_resource: " << value << std::endl;
        test_set.erase(value);
    }

    static holder_value_type create(int32_t value) {
        auto lock = std::scoped_lock{mutex};
        test_set.insert(value);
        return value;
    }
};

static_assert(ywl::miscellaneous::is_shared_resource_holder_hint_type<holder_shared_int32_type>);

void fn_const_ptr(const int *ptr) {
    std::cout << "fn_const_ptr: " << *ptr << std::endl;
}

void fn_ptr(int *ptr) {
    std::cout << "fn_ptr: " << *ptr << std::endl;
}

int main() {
    auto &logger = ywl::util::default_logger;

    /*struct A {
        int a;
        operator int() const {
            return a;
        }
    };

    struct B {
        operator A() const {
            return A{42};
        }
    };*/

    /*static_assert(std::is_same_v<decltype(A{}), A>);
    static_assert(std::is_same_v<decltype((A{})), A>);

    static_assert(std::is_same_v<decltype(A{}.a), int>);
    static_assert(std::is_same_v<decltype((A{}.a)), int &&>);

    [[maybe_unused]] A ins = A{};

    static_assert(std::is_same_v<decltype(ins), A>);
    static_assert(std::is_same_v<decltype((ins)), A &>);

    static_assert(std::is_same_v<decltype(ins.a), int>);
    static_assert(std::is_same_v<decltype((ins.a)), int &>);

    [[maybe_unused]] A &&rref = A{};

    static_assert(std::is_same_v<decltype(rref.a), int>);
    static_assert(std::is_same_v<decltype((rref.a)), int &>);

    static_assert(std::is_same_v<decltype(std::move(rref).a), int>);
    static_assert(std::is_same_v<decltype((std::move(rref).a)), int &&>);

    auto [str] = A{};
    static_assert(std::is_same_v<decltype(str), int>);*/

    struct A {
        int a;
        operator int() const {
            return a;
        }
    };

    logger["Hi, mom!"];
    logger.log_fmt("Hi, dad!");

    logger.log_no_fmt("Hi, {}!", "mom");
    logger.log_fmt("Hi, {}!", "dad");

    MyType obj{42};
    logger("obj: {}", obj);

    std::unordered_map<int, int> map{{1, 2}, {3, 4}};

    logger("map: {}", map);

    auto random_gen = ywl::miscellaneous::random_generator_real(0., 100.);
    logger("random_gen: {}", random_gen());

    ywl::miscellaneous::scoped_timer timer;

    logger[timer.stop_and_report()];

    auto id_gen = ywl::miscellaneous::discrete_id_generator{};

    auto id1 = id_gen.generate();
    auto id2 = id_gen.generate();

    logger[id1, id2];

    id_gen.free(std::move(id1)); // NOLINT
    id_gen.free(std::move(id2)); // NOLINT

    id1 = id_gen.generate();
    id2 = id_gen.generate();

    logger[id1, id2];

    using unique_int32_holder = ywl::miscellaneous::unique_holder<ywl::miscellaneous::unique_hint_base_default<
        uint32_t, int32_constructor_type, int32_destructor_type, 0> >;
    // additional tests for unique_holder
    {
        // Test default constructor and has_value
        unique_int32_holder holder1;
        assert(!holder1.has_value());

        // Test create and move constructor
        unique_int32_holder holder2 = unique_int32_holder::create(100);
        assert(holder2.has_value());

        unique_int32_holder holder3 = std::move(holder2);
        assert(holder3.has_value());
        assert(!holder2.has_value());

        // Test move assignment operator
        unique_int32_holder holder4 = unique_int32_holder::create(200);
        holder4 = std::move(holder3);
        assert(holder4.has_value());
        assert(!holder3.has_value());

        // Test release and reset
        uint32_t value1 = holder4.release();
        test_set.erase(value1);
        assert(value1 == 100);
        assert(!holder4.has_value());

        holder4 = unique_int32_holder::create(300);
        holder4.reset();
        assert(!holder4.has_value());

        // Test create with multiple arguments
        unique_int32_holder holder5 = unique_int32_holder::create(400);
        assert(holder5.has_value());

        // Test get and operator*
        uint32_t value2 = holder5.get();
        assert(value2 == 400);

        uint32_t value3 = *holder5;
        assert(value3 == 400);

        // Log the values in test_set
        for (const auto &value: test_set) {
            logger("value: {}", value);
        }
    }

    assert(test_set.empty());

    using shared_int32_holder = ywl::miscellaneous::shared_holder<holder_shared_int32_type>;
    // std::cout << "test_set.size(): " << test_set.size() << std::endl;

    // block
    {
        std::vector<shared_int32_holder> holders1, holders2;
        shared_int32_holder holder_original = shared_int32_holder::create(100);

        std::vector<shared_int32_holder::weak_type> weak_holders;

        shared_int32_holder::weak_type weak_holder_cp(holder_original);

        std::jthread thread1([&holders1, &holder_original] {
            for (int i = 0; i < 100000; ++i) {
                holders1.emplace_back(holder_original);
            }
        });

        std::jthread thread3([&holder_original, &weak_holders] {
            for (int i = 0; i < 100000; ++i) {
                weak_holders.emplace_back(holder_original);
            }
        });

        std::jthread thread2([&holders2, &holder_original] {
            for (int i = 0; i < 100000; ++i) {
                holders2.emplace_back(holder_original);
            }
        });

        thread1.join();
        std::jthread([&holders1, &weak_holder_cp] {
            holders1.emplace_back(weak_holder_cp.lock());
        }).join();
    }

    std::cout << "test_set.size(): " << test_set.size() << std::endl;

    auto exception = ywl::basic::ywl_impl_error{"test exception"};

    std::cout << exception.what() << std::endl;

    return 0;
}
