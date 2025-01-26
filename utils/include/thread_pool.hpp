#pragma once

#include "enum_parser.hpp"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace utils {

enum class QueueFullPolicy : uint8_t {
    AbortPolicy = 0,
    CallerRunsPolicy,
    DiscardRandomInQueuePolicy,
};

enum class TaskStatus : uint8_t { WAITING = 0, RUNNING, FINISHED };

enum class WorkerStatus : uint8_t { IDLE = 0, RUNNING };

class ThreadPool {
private:
    struct Task {
        std::function<void()> task;
        std::chrono::time_point<std::chrono::system_clock> create_time;
        std::string name;
        TaskStatus status = TaskStatus::WAITING;

        Task() = default;

        Task(const Task& other) = delete;

        Task operator=(const Task& other) = delete;

        Task(Task&& other) noexcept: create_time(other.create_time), name(std::move(other.name)) {
            task = other.task;
        }

        Task& operator=(Task&& other) noexcept {
            task = other.task;
            create_time = other.create_time;
            name = std::move(other.name);
            return *this;
        }

        Task(
            std::function<void()>&& task,
            const std::chrono::time_point<std::chrono::system_clock>& create_time,
            std::string name
        ):
            task(std::move(task)),
            create_time(create_time),
            name(std::move(name)) {}

        void operator()() {
            status = TaskStatus::RUNNING;
            task();
            status = TaskStatus::FINISHED;
        }

        bool operator==(const Task& other) const {
            return name == other.name && create_time == other.create_time;
        }
    };

    struct Worker {
        std::thread worker_thread;
        std::shared_ptr<Task> task;
        WorkerStatus status = WorkerStatus::IDLE;

        void join() {
            worker_thread.join();
        }
    };

public:
    explicit ThreadPool(std::size_t nums_threads):
        m_max_workers_num(std::thread::hardware_concurrency()),
        m_max_tasks_num(m_max_workers_num * 10) {
        if (nums_threads > m_max_workers_num) {
            nums_threads = m_max_workers_num;
        }
        auto worker_func = [this] {
            while (true) {
                std::string name;
                {
                    std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                    this->m_condition.wait(lock, [this] {
                        return this->m_stop || !this->m_tasks.empty();
                    });
                    if (this->m_stop || this->m_tasks.empty()) {
                        return;
                    }
                    name = this->m_tasks.front().name;
                    m_task_pool.insert({ name, std::move(this->m_tasks.front()) });
                    if (m_task_pool.size() > m_max_tasks_num) [[unlikely]] {
                        for (auto it = m_task_pool.begin(); it != m_task_pool.end();) {
                            if (it->second.status == TaskStatus::FINISHED) {
                                it = m_task_pool.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    this->m_tasks.pop_front();
                }
                try {
                    if (m_task_pool.at(name).status == TaskStatus::WAITING) {
                        m_task_pool.at(name)();
                    }
                } catch (const std::future_error& e) {
                    std::cerr << dump_enum(m_task_pool.at(name).status) << " " << e.what()
                              << std::endl;
                }
            }
        };
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back(Worker {
                .worker_thread = std::thread(worker_func),
                .task = nullptr,
                .status = WorkerStatus::IDLE,
            });
        }
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>> submit(F&& f, Args&&... args) {
        using ReturnType = std::result_of_t<F(Args...)>;
        if (m_tasks.size() >= m_max_tasks_num) {
            switch (m_queue_full_policy) {
                case QueueFullPolicy::AbortPolicy:
                    return std::nullopt;
                case QueueFullPolicy::CallerRunsPolicy: {
                    return std::async(
                        std::launch::async,
                        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                    );
                }
                case QueueFullPolicy::DiscardRandomInQueuePolicy:
                    if (m_tasks.empty()) [[unlikely]] {
                        m_tasks.pop_front();
                    }
                    break;
            }
        }

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        {
            Task t([task] { (*task)(); }, std::chrono::system_clock::now(), generate_random_name());
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.push_back(std::move(t));
        }

        m_condition.notify_one();
        return result;
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>>
    submit(const std::string& name, F&& f, Args&&... args) {
        assert(name.empty() == false && "Task name cannot be empty");

        using ReturnType = std::result_of_t<F(Args...)>;
        if (m_tasks.size() >= m_max_tasks_num) {
            switch (m_queue_full_policy) {
                case QueueFullPolicy::AbortPolicy:
                    return std::nullopt;
                case QueueFullPolicy::CallerRunsPolicy: {
                    return std::async(
                        std::launch::async,
                        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                    );
                }
                case QueueFullPolicy::DiscardRandomInQueuePolicy:
                    if (m_tasks.empty()) [[unlikely]] {
                        m_tasks.pop_front();
                    }
                    break;
            }
        }

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        {
            Task t([task]() { (*task)(); }, std::chrono::system_clock::now(), name);
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.push_back(std::move(t));
        }

        m_condition.notify_one();
        return result;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_stop = true;
        }
        m_condition.notify_all();

        for (auto& worker: m_workers) {
            worker.join();
        }
    }

    void stop_now() {
        m_stop = true;
        m_condition.notify_all();

        for (auto& worker: m_workers) {
            worker.join();
        }
    }

    // it won't take effect immediately because this will wait for enough workers to finish their current task
    void resize_worker(std::size_t nums_threads) {
        if (nums_threads > m_max_workers_num) {
            nums_threads = m_max_workers_num;
        }
        // lock has been got by sub function in "add worker" and "delete worker"
        if (nums_threads > m_workers.size()) {
            add_worker(nums_threads - m_workers.size());
        } else if (nums_threads < m_workers.size()) {
            delete_worker(m_workers.size() - nums_threads);
        }
    }

    void add_worker(std::size_t nums_threads) {
        if (nums_threads > m_max_workers_num) {
            nums_threads = m_max_workers_num;
        }
        auto worker_func = [this] {
            while (true) {
                std::string name;
                {
                    std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                    this->m_condition.wait(lock, [this] {
                        return this->m_stop || !this->m_tasks.empty();
                    });
                    if (this->m_stop || this->m_tasks.empty()) {
                        return;
                    }
                    name = this->m_tasks.front().name;
                    m_task_pool.insert({ m_tasks.front().name, std::move(this->m_tasks.front()) });
                    if (m_task_pool.size() > m_max_tasks_num) [[unlikely]] {
                        ///TODO: Segmentation fault happened here
                        for (auto it = m_task_pool.begin(); it != m_task_pool.end(); ++it) {
                            if (it->second.status == TaskStatus::FINISHED) {
                                m_task_pool.erase(it);
                            }
                        }
                    }
                    this->m_tasks.pop_front();
                }
                try {
                    if (m_task_pool.at(name).status == TaskStatus::WAITING) {
                        m_task_pool.at(name)();
                    }
                } catch (const std::future_error& e) {
                    std::cerr << dump_enum(m_task_pool.at(name).status) << " " << e.what()
                              << std::endl;
                }
            }
        };
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back(Worker {
                .worker_thread = std::thread(worker_func),
                .task = nullptr,
                .status = WorkerStatus::IDLE,
            });
        }
        m_max_tasks_num = m_workers.size() * 10;
    }

    void delete_worker(std::size_t nums_threads) {
        if (m_workers.empty()) {
            return;
        }
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.back().join();
            m_workers.pop_back();
        }
        m_max_tasks_num = m_workers.size() * 10;
    }

    [[nodiscard]] bool is_running() const {
        return !m_stop;
    }

    ~ThreadPool() {
        if (!this->m_stop) {
            this->stop();
        }
    }

    /**
     * @brief set the max number of tasks in the queue
     *
     * @param max_tasks_num the max number of tasks in the queue
     */
    void set_queue_full_policy(QueueFullPolicy policy) {
        m_queue_full_policy = policy;
    }

    /**
     * @brief remove a task from the queue
     *
     * @param name the name of the task
     * @return true if the task is removed successfully
     * @return false if the task is not found or is running
     */
    bool remove_task(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
            if (it->name == name) {
                m_tasks.erase(it);
                return true;
            }
        }
        if (m_task_pool.contains(name)) {
            if (m_task_pool[name].status == TaskStatus::RUNNING) {
                return false;
            }
            m_task_pool.erase(name);
            return true;
        } else {
            return false;
        }
    }

    std::optional<std::tuple<TaskStatus, std::chrono::duration<double>>>
    get_task_status(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (const auto& task: m_tasks) {
            if (task.name == name) {
                return std::make_tuple(
                    task.status,
                    std::chrono::duration<double>(
                        std::chrono::system_clock::now() - task.create_time
                    )
                );
            }
        }
        if (m_task_pool.contains(name)) {
            return std::make_tuple(
                m_task_pool[name].status,
                std::chrono::duration<double>(
                    std::chrono::system_clock::now() - m_task_pool[name].create_time
                )
            );
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t max_workers_num() const {
        return m_max_workers_num;
    }

    [[nodiscard]] std::size_t max_tasks_num() const {
        return m_max_tasks_num;
    }

    std::size_t worker_num() const {
        return m_workers.size();
    }

    /**
     * @brief get the number of tasks in the idle queue
     */
    std::size_t task_num() const {
        return m_tasks.size();
    }

    using SharedPtr = std::shared_ptr<ThreadPool>;
    using UniquePtr = std::unique_ptr<ThreadPool>;

private:
    static std::string generate_random_name() {
        std::random_device rd;
        std::mt19937_64 generation(rd());
        std::uniform_int_distribution<uint64_t> uniform_distribution;

        uint64_t part1 = uniform_distribution(generation);
        uint64_t part2 = uniform_distribution(generation);

        std::ostringstream oss;
        oss << std::hex << part1 << part2;
        return oss.str();
    }

    std::vector<Worker> m_workers;
    std::deque<Task> m_tasks;

    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    bool m_stop = false;

    // let system decide
    std::size_t m_max_workers_num;
    std::size_t m_max_tasks_num {};

    std::unordered_map<std::string, Task> m_task_pool;

    QueueFullPolicy m_queue_full_policy = QueueFullPolicy::AbortPolicy;
};

} // namespace utils