module;

#include <cassert>

export module dev;

import ywl.prelude;

export class co_awaitable_base {
public:
    virtual ~co_awaitable_base() = default;

    // define awaiter functions
    virtual bool finished() const = 0;

    virtual void resume() = 0;

    virtual void *get_handle() = 0;
};

export template<typename T>
class co_awaitable;

export class co_executor_base {
public:
    virtual ~co_executor_base() = default;

    virtual void initial_schedule_task(co_awaitable_base *issuer, co_awaitable_base *co_awaitable) = 0;

    virtual void reschedule_task(co_awaitable_base *co_awaitable) = 0;

    virtual void schedule_dependency_of_current_task(co_awaitable_base *co_awaitable) = 0;

    template<typename ...Args>
    void await_all(co_awaitable<Args> &... co_awaitables);

    template<typename ...Args>
    static std::tuple<Args...> yield_all(co_awaitable<Args> &... co_awaitables);

    virtual void run() = 0;

    virtual void handle_finish(co_awaitable_base *co_awaitable) = 0;
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

    void await_resume() {}
};

export class simple_co_executor;

export inline thread_local co_executor_base *current_executor = nullptr;

export class simple_co_executor : public co_executor_base {
private:
    std::queue<co_awaitable_base *> m_queue;
    std::unordered_map<void *, void *> dependency_of;
    std::unordered_map<void *, std::unordered_set<void * >> depend_on;
    void *current_handle = nullptr;
public:
    void initial_schedule_task(co_awaitable_base *issuer, co_awaitable_base *co_awaitable) override {
/*        ywl::printf_ln("initial_schedule_task: {}, issuer: {}",
                       co_awaitable->get_handle(), issuer ? issuer->get_handle() : nullptr);*/
        m_queue.push(co_awaitable);
        if (issuer == nullptr) {
            dependency_of[co_awaitable->get_handle()] = nullptr;
            return;
        }

        dependency_of[co_awaitable->get_handle()] = issuer->get_handle();
        depend_on[issuer->get_handle()].insert(co_awaitable->get_handle());
    }

    void reschedule_task(co_awaitable_base *co_awaitable) override {
//        ywl::printf_ln("reschedule_task: {}", co_awaitable->get_handle());
        m_queue.push(co_awaitable);
    }

    void schedule_dependency_of_current_task(co_awaitable_base *co_awaitable) override {
        assert(current_handle != nullptr);
/*        ywl::printf_ln("schedule_dependency_of_current_task: {}, current_task: {}", co_awaitable->get_handle(),
                       current_handle).flush();*/
        m_queue.push(co_awaitable);
        depend_on[current_handle].insert(co_awaitable->get_handle());
        dependency_of[co_awaitable->get_handle()] = current_handle;
    }

    void run() override {
//        static int counter = 0;
        while (!m_queue.empty()) {
            auto co_awaitable = m_queue.front();
            m_queue.pop();

/*            if (counter++ > 1000) {
                ywl::printf_ln("Infinite loop detected, exiting...").flush();
                break;
            }*/

//            ywl::printf_ln("processing: {}", co_awaitable->get_handle());

//            if (!dependency_of.contains(co_awaitable->get_handle())) {
//                ywl::printf_ln("{} should be finished.", co_awaitable->get_handle());
//                continue;
//            }

            if (co_awaitable->finished()) {
                handle_finish(co_awaitable);
            }

            if (depend_on.contains(co_awaitable->get_handle())) {
/*                for (auto &dep: depend_on[co_awaitable->get_handle()]) {
                    ywl::printf_ln("depend_on[{}]: {}", co_awaitable->get_handle(), dep).flush();
                }*/
                m_queue.push(co_awaitable);
                continue;
            }

            current_executor = this;
            current_handle = co_awaitable->get_handle();

            try {
                co_awaitable->resume();
            } catch (const std::exception &e) {
                ywl::printf_ln("Exception was thrown in coroutine: {}", e.what()).flush();
                std::rethrow_exception(std::current_exception());
            }
        }
    }

    void handle_finish(co_awaitable_base *co_awaitable) override {
//        ywl::printf_ln("{} is finished.", co_awaitable->get_handle());
        auto issuer = dependency_of[co_awaitable->get_handle()];
//                ywl::printf_ln("issuer: {}", issuer).flush();
//                assert(issuer != nullptr);
//                assert(depend_on.contains(issuer));
        depend_on[issuer].erase(co_awaitable->get_handle());
//                ywl::printf_ln("Previously, issuer has {} dependencies", depend_on[issuer].size()).flush();
/*                for (auto &dep : depend_on[issuer]) {
                    ywl::printf_ln("depend_on[{}]: {}", issuer, dep).flush();
                }*/
//                ywl::printf_ln("Just erased {} from depend_on[{}]", co_awaitable->get_handle(), issuer).flush();
//        ywl::printf_ln("Now issuer has {} dependencies", depend_on[issuer].size()).flush();
/*                for (auto &dep : depend_on[issuer]) {
                    ywl::printf_ln("depend_on[{}]: {}", issuer, dep).flush();
                }*/
        if (depend_on[issuer].empty()) {
            depend_on.erase(issuer);
        }
        dependency_of.erase(co_awaitable->get_handle());
    }
};

export class co_context {
private:
    std::unique_ptr <co_executor_base> m_executor;

    co_context(std::unique_ptr <co_executor_base> executor) : m_executor(std::move(executor)) {}

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

    co_awaitable(std::coroutine_handle <promise_type> handle) : m_handle(handle) {}

    struct promise_type {
    private:
        std::optional <T> m_value;
        bool finished = false;
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
            if (finished) {
                throw ywl::basic::logic_error("value is already consumed");
            }
            finished = true;
            m_value = std::move(value);
        }

        [[nodiscard]] bool is_finished() const {
            return finished;
        }

        T get_value() {
            if (!m_value.has_value() || !finished) {
                throw ywl::basic::logic_error("value is not set");
            }
            return std::exchange(m_value, std::nullopt).value();
        }

        ~promise_type() = default;

        // await_transform
        template<typename U>
        auto &&await_transform(co_awaitable<U> &&co_awaitable) {
            current_executor->schedule_dependency_of_current_task(&co_awaitable);
            return co_awaitable;
        }

        wait_for_current_executor_t await_transform(wait_for_current_executor_t &&wait_for_current_executor) {
            return wait_for_current_executor;
        }
    };

private:
    std::coroutine_handle <promise_type> m_handle;

public:
    co_awaitable(const co_awaitable &) = delete;

    co_awaitable &operator=(const co_awaitable &) = delete;

    co_awaitable(co_awaitable &&other) = delete;

    co_awaitable &operator=(co_awaitable &&other) = delete;

    [[nodiscard]] bool finished() const override {
        return m_handle.promise().is_finished();
    }

    [[nodiscard]] T get_value() {
        return m_handle.promise().get_value();
    }

    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return finished();
    }

    void await_suspend(std::coroutine_handle<> issuer) {
        current_executor->schedule_dependency_of_current_task(this);
    }

    T await_resume() {
//        assert(finished());
        return get_value();
    }

    void resume() override {
        if (!finished())
            this->m_handle.resume();
        if (!finished()) {
            current_executor->reschedule_task(this);
        } else {
            current_executor->handle_finish(this);
        }
    }

    void *get_handle() override {
        return static_cast<void *>(m_handle.address());
    }

    ~co_awaitable() override {
        if (m_handle) {
            m_handle.destroy();
        }
    }
};

/*template<>
class co_awaitable<void> : public co_awaitable_base {
public:
    using value_type = void;

    co_awaitable() = default;

    struct promise_type;

    co_awaitable(std::coroutine_handle <promise_type> handle) : m_handle(handle) {}

    struct promise_type {
    private:
        bool finished = false;
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
        void return_void() {
            finished = true;
        }

        bool is_finished() const {
            return finished;
        }
    };

private:
    std::coroutine_handle <promise_type> m_handle;

public:
    co_awaitable(const co_awaitable &) = delete;

    co_awaitable &operator=(const co_awaitable &) = delete;

    co_awaitable(co_awaitable &&other) noexcept: m_handle(std::exchange(other.m_handle, nullptr)) {}

    co_awaitable &operator=(co_awaitable &&other) noexcept {
        if (this != &other) {
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    [[nodiscard]] bool finished() const override {
        return m_handle.promise().is_finished();
    }

    void get_value() {}

    // define awaiter functions
    [[nodiscard]] bool await_ready() const {
        return m_handle.promise().is_finished();
    }

    void await_suspend(std::coroutine_handle<>) {
        m_handle.resume();
    }

    void await_resume() {
        if (!finished()) {
            m_handle.resume();
        }
    }

    void resume(co_executor_base *executor) override {
        if (!finished())
            this->m_handle.resume();
        if (!finished()) {
            executor->initial_schedule_task(this);
        }
    }

    ~co_awaitable() override {
        if (m_handle) {
            m_handle.destroy();
        }
    }
};*/

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

