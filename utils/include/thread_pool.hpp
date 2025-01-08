#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>
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


class ThreadPool {
private:
    struct Task {
        std::function<void()> task;
        std::chrono::time_point<std::chrono::system_clock> create_time;
        std::string name;
    };
public:
    ThreadPool(std::size_t nums_threads): m_max_workers_num(std::thread::hardware_concurrency()) {
        if (nums_threads > m_max_workers_num) {
            nums_threads = m_max_workers_num;
        }
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back([this] {
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
                        this->m_tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>> submit(F&& f, Args&&... args) {
        using ReturnType = std::result_of_t<F(Args...)>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        Task t {
            .task = [task]() { (*task)(); },
            .create_time = std::chrono::system_clock::now(),
            .name = generate_random_name()
        };
        
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one();
        return result;
    }

    template<typename F, typename... Args>
    std::optional<std::future<std::result_of_t<F(Args...)>>> submit(const std::string& name, F&& f, Args&&... args) {
        using ReturnType = std::result_of_t<F(Args...)>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        Task t { .task = [task]() { (*task)(); },
                 .create_time = std::chrono::system_clock::now(),
                 .name = name };
        
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_stop) {
                return std::nullopt;
            }
            m_tasks.emplace([task]() { (*task)(); });
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
    void resize_woker(std::size_t nums_threads) {
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
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.emplace_back([this] {
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
                        this->m_tasks.pop();
                    }
                    task();
                }
            });
        }
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
    }

    bool is_running() const {
        return !m_stop;
    }

    ~ThreadPool() {
        this->stop();
    }

private:
    std::string generate_random_name() {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return std::to_string(t);
    }
    
    std::vector<std::thread> m_workers;
    std::queue<Task> m_tasks;

    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    bool m_stop = false;

    // let system decide
    std::size_t m_max_workers_num;

};



} // namespace utils