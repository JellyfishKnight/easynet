#pragma once

#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <optional>
#include <queue>

namespace utils {



class ThreadPool {
public:
    ThreadPool(std::size_t nums_threads) {
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

    // it won't take effect immediately because this will wait for enought workers to finish their current task
    void resize_woker(std::size_t nums_threads) {
        // lock has been got by sub function in "add worker" and "delete worker"
        if (nums_threads > m_workers.size()) {
            add_worker(nums_threads - m_workers.size());
        } else if (nums_threads < m_workers.size()) {
            delete_worker(m_workers.size() - nums_threads);
        }
    }

    void add_worker(std::size_t nums_threads) {
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
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        for (std::size_t i = 0; i < nums_threads; i++) {
            m_workers.back().join();
            m_workers.pop_back();
        }
    }

    ~ThreadPool() {
        this->stop();
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_queue_mutex;
    std::condition_variable m_condition;

    bool m_stop = false;

};



} // namespace utils