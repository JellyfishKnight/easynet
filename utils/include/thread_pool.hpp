#pragma once

#include <cassert>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <future>
#include <optional>
#include <queue>

namespace utils {

enum class QueueFullPolicy : uint8_t {
    AbortPolicy = 0,
    CallerRunsPolicy,
    DiscardPolicy,
    DiscardOldestPolicy
};

enum class TaskStatus : uint8_t {
    WAITING = 0,
    RUNNING,
    FINISHED
};

enum class WorkerStatus : uint8_t {
    IDLE = 0,
    RUNNING
};

class ThreadPool {
private:
    struct Task {
        std::function<void()> task;
        std::chrono::time_point<std::chrono::system_clock> create_time;
        std::string name;
        TaskStatus status = TaskStatus::WAITING;

        Task() = default;

        Task(const Task& other) = default;

        Task(std::function<void()>&& task, const std::chrono::time_point<std::chrono::system_clock>& create_time, std::string name)
            : task(std::move(task)), create_time(create_time), name(std::move(name)) {}

        void operator()() const {
            task();
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
    explicit ThreadPool(std::size_t nums_threads): m_max_workers_num(std::thread::hardware_concurrency()), m_max_tasks_num(m_max_workers_num * 10) {
        if (nums_threads > m_max_workers_num) {
            nums_threads = m_max_workers_num;
        }
        auto worker_func = [this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                    this->m_condition.wait(lock, [this] {
                        return this->m_stop || !this->m_tasks.empty();
                    });
                    if (this->m_stop || this->m_tasks.empty()) {
                        return ;
                    }
                    task = this->m_tasks.front();
                    this->m_tasks.pop();
                }
                task();
            }
        };
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back(
                Worker {
                    .worker_thread = std::thread(worker_func),
                    .task = nullptr,
                    .status = WorkerStatus::IDLE,
                }
            );
        }
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>> submit(F&& f, Args&&... args) {
        if (m_tasks.size() >= m_max_tasks_num) {
            ///TODO: discard policy
        }

        using ReturnType = std::result_of_t<F(Args...)>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        Task t ([task] { (*task)(); },
                std::chrono::system_clock::now(),
                generate_random_name());

        m_names[t.name] = true;
        
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.emplace(t);
        }

        m_condition.notify_one();
        return result;
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>>
    submit(const std::string& name, F&& f, Args&&... args) {
        assert(name.empty() == false);

        assert(m_names.find(name) == m_names.end());

        m_names[name] = true;
        
        if (m_tasks.size() >= m_max_tasks_num) {
            switch (m_queue_full_policy) {
                case QueueFullPolicy::AbortPolicy:
                    return std::nullopt;
                case QueueFullPolicy::CallerRunsPolicy:
                    // std::thread([this, f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)] {
                    //     f(std::get<Args>(args)...);
                    // }).detach();
                    return std::nullopt;
                case QueueFullPolicy::DiscardPolicy:
                    return std::nullopt;
                case QueueFullPolicy::DiscardOldestPolicy:
                    m_tasks.pop();
                    break;
            }
        }
        
        using ReturnType = std::result_of_t<F(Args...)>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        Task t ([task]() { (*task)(); },
                std::chrono::system_clock::now(),
                name);
        
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.emplace(t);
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

    // it won't take effect immediately because this will wait for enought workers to finish their current task
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
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->m_queue_mutex);
                    this->m_condition.wait(lock, [this] {
                        return this->m_stop || !this->m_tasks.empty();
                    });
                    if (this->m_stop || this->m_tasks.empty()) {
                        return ;
                    }
                    task = this->m_tasks.front();
                    this->m_tasks.pop();
                }
                task();
            }
        };
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back(
                Worker{
                    .worker_thread = std::thread(worker_func),
                    .task = nullptr,
                    .status = WorkerStatus::IDLE,
                }
            );
        }
        m_max_tasks_num = m_workers.size() * 10;
    }

    void delete_worker(std::size_t nums_threads) {
        if (m_workers.empty()) {
            return ;
        }
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.back().join();
            m_workers.pop_back();
        }
        m_max_tasks_num = m_workers.size() * 10;
    }

    bool is_running() const {
        return !m_stop;
    }

    ~ThreadPool() {
        this->stop();
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
        if (m_names.find(name) == m_names.end()) {
            return false;
        }
        m_names.erase(name);
        return true;
    }

    std::size_t max_workers_num() const {
        return m_max_workers_num;
    }

    std::size_t max_tasks_num() const {
        return m_max_tasks_num;
    }

private:
    static std::string generate_random_name() {
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return std::to_string(t);
    }
    
    std::vector<Worker> m_workers;
    std::queue<Task> m_tasks;

    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    std::unordered_map<std::string, bool> m_names;

    bool m_stop = false;

    // let system decide
    std::size_t m_max_workers_num;
    std::size_t m_max_tasks_num{};

    QueueFullPolicy m_queue_full_policy = QueueFullPolicy::AbortPolicy;
};



} // namespace utils