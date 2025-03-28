module;

#include <cassert>

export module dev;

import ywl.prelude;

export class co_awaitable_base {
public:
    virtual ~co_awaitable_base() = default;

    virtual std::coroutine_handle<> get_handle() = 0;
};

export template<typename T>
class co_awaitable;

export class co_executor_base {
public:
    virtual ~co_executor_base() = default;

    virtual void initial_schedule_task(co_awaitable_base *issuer, co_awaitable_base *co_awaitable) = 0;

    virtual void reschedule_task(co_awaitable_base *co_awaitable) = 0;

    virtual void schedule_dependency_of_current_task(co_awaitable_base *co_awaitable) = 0;

    template<typename... Args>
    void await_all(co_awaitable<Args> &... co_awaitables);

    template<typename... Args>
    static std::tuple<Args...> yield_all(co_awaitable<Args> &... co_awaitables);

    virtual void run() = 0;

    virtual void handle_finish(std::coroutine_handle<> co_awaitable) = 0;
};

// awaiter
export class wait_for_current_executor_t {
public:
    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return false;
    }

    void await_suspend(std::coroutine_handle<> issuer) {
    }

    void await_resume() {
    }
};

export class simple_co_executor;

export inline thread_local co_executor_base *current_executor = nullptr;

export class simple_co_executor : public co_executor_base {
private:
    std::queue<std::coroutine_handle<>> m_queue;
    std::unordered_map<std::coroutine_handle<>, std::coroutine_handle<>> dependency_of;
    std::unordered_map<std::coroutine_handle<>, std::unordered_set<std::coroutine_handle<>>> depend_on;
    std::coroutine_handle<> current_handle = nullptr;

public:
    void initial_schedule_task(co_awaitable_base *issuer, co_awaitable_base *co_awaitable) override {
        //        ywl::printf_ln("Coroutine {} is added to the queue.", co_awaitable->get_handle().address()).flush();
        m_queue.push(co_awaitable->get_handle());
        if (issuer == nullptr) {
            dependency_of[co_awaitable->get_handle()] = nullptr;
            return;
        }

        dependency_of[co_awaitable->get_handle()] = issuer->get_handle();
        depend_on[issuer->get_handle()].insert(co_awaitable->get_handle());
    }

    void reschedule_task(co_awaitable_base *co_awaitable) override {
        //        ywl::printf_ln("Coroutine {} is rescheduled.", co_awaitable->get_handle().address()).flush();
        m_queue.push(co_awaitable->get_handle());
    }

    void schedule_dependency_of_current_task(co_awaitable_base *co_awaitable) override {
        assert(current_handle != nullptr);
        //        ywl::printf_ln("Coroutine {} is added as a dependency of {}.", co_awaitable->get_handle().address(),
        //                       current_handle.address()).flush();
        m_queue.push(co_awaitable->get_handle());
        dependency_of[co_awaitable->get_handle()] = current_handle;
        depend_on[current_handle].insert(co_awaitable->get_handle());
    }

    void run() override {
        while (!m_queue.empty()) {
            auto handle = m_queue.front();
            m_queue.pop();

            if (depend_on.contains(handle)) {
                //                ywl::printf_ln("Coroutine {} is waiting for dependencies.", handle.address()).flush();
                m_queue.push(handle);
                continue;
            }

            current_executor = this;
            current_handle = handle;

            try {
                //                ywl::printf_ln("Running coroutine: {}", handle.address()).flush();
                handle.resume();
                if (!handle.done()) {
                    //                    ywl::printf_ln("Coroutine {} is not finished.", handle.address()).flush();
                    m_queue.push(handle);
                } else {
                    //                    ywl::printf_ln("Coroutine {} is finished.", handle.address()).flush();
                    handle_finish(handle);
                }
            } catch (const std::exception &e) {
                //                ywl::printf_ln("Exception was thrown in coroutine: {}", e.what()).flush();
                std::rethrow_exception(std::current_exception());
            }
        }
    }

    void handle_finish(std::coroutine_handle<> handle) override {
        //        ywl::printf_ln("{} is finished.", handle.address()).flush();
        auto issuer = dependency_of[handle];
        depend_on[issuer].erase(handle);
        if (depend_on[issuer].empty()) {
            depend_on.erase(issuer);
        }
        dependency_of.erase(handle);
    }
};

export class co_context {
private:
    std::unique_ptr<co_executor_base> m_executor;

    co_context(std::unique_ptr<co_executor_base> executor) : m_executor(std::move(executor)) {
    }

public:
    template<std::derived_from<co_executor_base> T, typename... Args>
    static co_context from_executor(Args &&... args) {
        return co_context(std::make_unique<T>(std::forward<Args>(args)...));
    }

    co_executor_base *get_executor() {
        return m_executor.get();
    }

    void run() {
        m_executor->run();
    }

    template<typename T>
    T block_on(co_awaitable<T> &&co_awaitable);

    void schedule(co_awaitable_base *issuer, co_awaitable_base *co_awaitable) {
        m_executor->initial_schedule_task(issuer, co_awaitable);
    }
};


export template<typename T>
class co_awaitable : public co_awaitable_base {
public:
    using value_type = T;

    co_awaitable() = default;

    struct promise_type;

    co_awaitable(std::coroutine_handle<promise_type> handle) : m_handle(handle) {
    }

    struct promise_type {
    private:
        std::optional<T> m_value;

    public:
        co_awaitable get_return_object() {
            return co_awaitable{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() {
            std::rethrow_exception(std::current_exception());
        }

        // return value
        void return_value(T value) {
            m_value = std::move(value);
        }

        T get_value() {
            if (!m_value.has_value()) {
                throw ywl::basic::logic_error("value is not set");

            }
            return std::exchange(m_value, std::nullopt).value();
        }

        ~promise_type() = default;

        // await_transform
        /*template<typename U>
        auto &&await_transform(co_awaitable<U> &&co_awaitable) {
            return co_awaitable;
        }*/

        decltype(auto) await_transform(auto &&arg) {
            return arg;
        }

        wait_for_current_executor_t await_transform(wait_for_current_executor_t &&wait_for_current_executor) {
            return wait_for_current_executor;
        }
    };

private:
    std::coroutine_handle<promise_type> m_handle;

public:
    co_awaitable(const co_awaitable &) = delete;

    co_awaitable &operator=(const co_awaitable &) = delete;

    co_awaitable(co_awaitable &&other) noexcept: m_handle(std::exchange(other.m_handle, nullptr)) {
    }

    co_awaitable &operator=(co_awaitable &&other) noexcept {
        if (this != &other) {
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    [[nodiscard]] T get_value() {
        return m_handle.promise().get_value();
    }

    [[nodiscard]] bool await_ready() const {
        return m_handle.done();
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        current_executor->schedule_dependency_of_current_task(this);
    }

    T await_resume() {
        return get_value();
    }

    std::coroutine_handle<> get_handle() override {
        return m_handle;
    }

    ~co_awaitable() override {
        if (m_handle) {
            m_handle.destroy();
        }
    }
};

template<typename T>
T co_context::block_on(co_awaitable<T> &&co_awaitable) {
    m_executor->initial_schedule_task(nullptr, &co_awaitable);
    m_executor->run();
    return co_awaitable.get_value();
}

template<typename... Args>
void co_executor_base::await_all(co_awaitable<Args> &... co_awaitables) {
    ((this->schedule_dependency_of_current_task(&co_awaitables)), ...);
}

template<typename... Args>
std::tuple<Args...> co_executor_base::yield_all(co_awaitable<Args> &... co_awaitables) {
    return std::tuple<Args...>(co_awaitables.get_value()...);
}

template<typename... Args>
class wait_all_of_t {
private:
    std::tuple<co_awaitable<Args>...> m_co_awaitables;

public:
    wait_all_of_t(co_awaitable<Args> &&... co_awaitables) : m_co_awaitables(std::move(co_awaitables)...) {
    }

    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return false;
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        std::apply([&issuer](auto &... co_awaitables) {
            ((current_executor->schedule_dependency_of_current_task(&co_awaitables)), ...);
        }, m_co_awaitables);
    }

    std::tuple<Args...> await_resume() {
        return std::apply([](auto &&... co_awaitables) {
            return std::make_tuple(co_awaitables.get_value()...);
        }, m_co_awaitables);
    }
};

export template<typename... Args>
wait_all_of_t<Args...> wait_all_of(co_awaitable<Args> &&... co_awaitables) {
    return wait_all_of_t<Args...>(std::move(co_awaitables)...);
}