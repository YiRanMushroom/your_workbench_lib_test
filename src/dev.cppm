module;

#include <cassert>

export module dev;

import ywl.prelude;

export class co_awaitable_base {
public:
    virtual ~co_awaitable_base() = default;

    virtual std::coroutine_handle<> get_handle() = 0;
};

export class co_executor_base {
public:
    virtual ~co_executor_base() = default;

    virtual void initial_schedule_task(std::coroutine_handle<> handle) = 0;

    virtual void schedule_dependency_all(std::coroutine_handle<>, std::coroutine_handle<>) = 0;

    virtual void schedule_dependency_any(std::coroutine_handle<>, std::coroutine_handle<>) = 0;

    virtual void run() = 0;
};

export inline thread_local co_executor_base *current_executor = nullptr;

export class simple_co_executor : public co_executor_base {
private:
    std::queue <std::coroutine_handle<>> m_queue;
    std::unordered_map <std::coroutine_handle<>, std::coroutine_handle<>> dependency_of;
    std::unordered_map <std::coroutine_handle<>, std::unordered_set<std::coroutine_handle<>>> depend_on;

    std::unordered_set <std::coroutine_handle<>> need_any_rather_than_all;
    std::unordered_set <std::coroutine_handle<>> tasks_to_cancel;

public:
    void initial_schedule_task(std::coroutine_handle<> handle) override {
        m_queue.push(handle);
    }

    void schedule_dependency_all(std::coroutine_handle<> issuer, std::coroutine_handle<> task) override {
        assert(issuer != nullptr);
        //        ywl::printf_ln("Coroutine {} is added as a dependency of {}.", co_awaitable->get_handle().address(),
        //                       current_handle.address()).flush();
        m_queue.push(task);
        dependency_of[task] = issuer;
        depend_on[issuer].insert(task);
    }

    void run() override {
        while (!m_queue.empty()) {
            auto handle = m_queue.front();
            m_queue.pop();

            if (tasks_to_cancel.contains(handle)) {
                tasks_to_cancel.erase(handle);
                continue;
            }

            if (depend_on.contains(handle)) {
//                ywl::printf_ln("Coroutine {} is waiting for dependencies.", handle.address()).flush();
                m_queue.push(handle);
                continue;
            }

            current_executor = this;

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

    void cancel_all_dependencies_of(std::coroutine_handle<> handle) {
        if (depend_on.contains(handle)) {
            for (const auto &other: depend_on[handle]) {
                tasks_to_cancel.insert(other);
                cancel_all_dependencies_of(other);
                dependency_of.erase(other);
            }
            depend_on.erase(handle);
        }
    }

    void handle_finish(std::coroutine_handle<> handle) {
        //        ywl::printf_ln("{} is finished.", handle.address()).flush();
        auto issuer = dependency_of[handle];
        if (need_any_rather_than_all.contains(issuer)) {
            depend_on[issuer].erase(handle);
            for (const auto &other: depend_on[issuer]) {
                if (other != handle) {
                    tasks_to_cancel.insert(other);
                    cancel_all_dependencies_of(other);
                }
                dependency_of.erase(other);
            }
            depend_on.erase(issuer);
            need_any_rather_than_all.erase(issuer);
        } else {
            depend_on[issuer].erase(handle);
            if (depend_on[issuer].empty()) {
                depend_on.erase(issuer);
            }
            dependency_of.erase(handle);
        }
    }

    void schedule_dependency_any(std::coroutine_handle<> issuer, std::coroutine_handle<> task) override {
//        ywl::printf_ln("Coroutine {} is added as a dependency of {}.", task.address(), issuer.address()).flush();
        need_any_rather_than_all.insert(issuer);
        m_queue.push(task);
        dependency_of[task] = issuer;
        depend_on[issuer].insert(task);
    }
};


export template<typename T>
class co_awaitable : public co_awaitable_base {
public:
    using value_type = T;

    co_awaitable() = default;

    struct promise_type {
    private:
        std::optional <T> m_value;

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

        void return_value(T value) {
            m_value = std::move(value);
        }

        T get_value() {
            if (!m_value.has_value()) {
                throw ywl::basic::logic_error("value is not set");

            }
            return std::exchange(m_value, std::nullopt).value();
        }

        std::optional <T> get_value_optional() {
            return std::exchange(m_value, std::nullopt);
        }

        ~promise_type() = default;
    };

    explicit co_awaitable(std::coroutine_handle <promise_type> handle) : m_handle(handle) {
    }

private:
    std::coroutine_handle <promise_type> m_handle;

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

    [[nodiscard]] std::optional <T> get_value_optional() {
        return m_handle.promise().get_value_optional();
    }

    [[nodiscard]] bool await_ready() const {
        return false;
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        current_executor->schedule_dependency_all(issuer, m_handle);
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

export class co_context {
private:
    std::unique_ptr <co_executor_base> m_executor;

    explicit co_context(std::unique_ptr <co_executor_base> executor) : m_executor(std::move(executor)) {
    }

public:
    template<std::derived_from <co_executor_base> T, typename... Args>
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
    T block_on(co_awaitable<T> &&co_awaitable) {
        m_executor->initial_schedule_task(co_awaitable.get_handle());
        m_executor->run();
        return co_awaitable.get_value();
    }
};

template<typename... Args>
class wait_all_of_t {
private:
    std::tuple<co_awaitable<Args>...> m_co_awaitables;

public:
    explicit wait_all_of_t(co_awaitable<Args> &&... co_awaitables) : m_co_awaitables(std::move(co_awaitables)...) {
    }

    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return false;
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        std::apply([&issuer](auto &... co_awaitables) {
            ((current_executor->schedule_dependency_all(issuer, co_awaitables.get_handle())), ...);
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

template<typename ...Args>
class wait_any_of_t {
private:
    std::tuple<co_awaitable<Args>...> m_co_awaitables;

public:
    explicit wait_any_of_t(co_awaitable<Args> &&... co_awaitables) : m_co_awaitables(std::move(co_awaitables)...) {
    }

    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return false;
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        std::apply([&issuer](auto &... co_awaitables) {
            ((current_executor->schedule_dependency_any(issuer, co_awaitables.get_handle())), ...);
        }, m_co_awaitables);
    }

    std::tuple<std::optional < Args>...>

    await_resume() {
        return std::apply([](auto &&... co_awaitables) {
            return std::make_tuple(co_awaitables.get_value_optional()...);
        }, m_co_awaitables);
    }
};

export template<typename ...Args>
wait_any_of_t<Args...> wait_any_of(co_awaitable<Args> &&... co_awaitables) {
    return wait_any_of_t<Args...>(std::move(co_awaitables)...);
}