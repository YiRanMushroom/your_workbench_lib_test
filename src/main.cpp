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

int main() {
    auto &&logger = ywl::util::default_logger;

    std::string str = "Str1, Str2!";

    ywl::miscellaneous::simple_buffer buffer{};
    buffer.push_back(str);

    logger[buffer.pop_back<std::string>()];

    std::vector<std::string> vec = {"Str1", "Str2", "!"};
    buffer.push_back(vec);

    logger[buffer.pop_back<std::vector<std::string> >()];

    std::unordered_map<std::string, std::tuple<int, double, std::string> > map = {
        {"Str1", {1, 3.14, "Str2"}},
        {"Str2", {2, 6.28, "!"}}
    };

    buffer.push_back(map);

    logger[buffer.pop_back<std::unordered_map<std::string, std::tuple<int, double, std::string> > >()];

    using unique_int32_holder = ywl::miscellaneous::unique_holder<ywl::miscellaneous::unique_hint_base_default<
        uint32_t, int32_constructor_type, int32_destructor_type> >;
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

    using shared_int32_holder = ywl::miscellaneous::shared_holder<holder_shared_int32_type>; {
        // std::vector<shared_int32_holder> holders1, holders2, holders3;
        shared_int32_holder holder_original = shared_int32_holder::create(100);

        // std::vector<shared_int32_holder::weak_type> weak_holders;

        shared_int32_holder::weak_type weak_holder_cp(holder_original);

        std::jthread thread1{
            [&holder_original] {
                std::vector<shared_int32_holder> holders1;
                for (int i = 0; i < 10000000; ++i) {
                    holders1.emplace_back(holder_original);
                }
            }
        };

        std::jthread thread2{
            [&holder_original] {
                std::vector<shared_int32_holder> holders2;
                for (int i = 0; i < 10000000; ++i) {

                    holders2.emplace_back(holder_original);
                }
            }
        };

        std::jthread thread3{
            [&holder_original] {
                std::vector<shared_int32_holder::weak_type> weak_holders;
                for (int i = 0; i < 10000000; ++i) {
                    weak_holders.emplace_back(holder_original);
                }
            }
        };

        std::jthread thread4{
            [&weak_holder_cp] {
                std::vector<shared_int32_holder> holders3;
                for (int i = 0; i < 10000000; ++i) {
                    holders3.emplace_back(weak_holder_cp.lock());
                }
            }
        };
    }

    std::cout << "test_set.size(): " << test_set.size() << std::endl;

    try {
        throw ywl::basic::runtime_error{"Runtime Error occurred!"};
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
