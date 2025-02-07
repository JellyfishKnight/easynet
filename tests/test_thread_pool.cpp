#include "thread_pool.hpp"
#include <chrono>
#include <format>
#include <future>
#include <gtest/gtest.h>
#include <ratio>
#include <string>
#include <thread>
#include <vector>

class ThreadPoolTest: public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<utils::ThreadPool>(1);
    }

    void TearDown() override {
        pool.reset();
    }

    std::unique_ptr<utils::ThreadPool> pool;
};

TEST_F(ThreadPoolTest, TestThreadPool) {
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        auto f = [](int a, int b) { return a + b; };
        auto opt = pool->submit(f, i, i + 1);
        if (opt.has_value()) {
            try {
                futures.emplace_back(std::move(opt.value()));
            } catch (const std::future_error& e) {
                FAIL() << e.what();
            }
        } else {
            FAIL() << "Failed to submit task";
        }
    }
    for (int i = 0; i < 100; ++i) {
        try {
            ASSERT_EQ(futures[i].get(), i + i + 1);
        } catch (const std::future_error& e) {
            FAIL() << e.what();
        }
    }
}

TEST_F(ThreadPoolTest, AddWorkerTest) {
    pool->add_worker(4);
    ASSERT_EQ(pool->worker_num(), 5);
}

TEST_F(ThreadPoolTest, DeleteWorkerTest) {
    pool->delete_worker(2);
    ASSERT_EQ(pool->worker_num(), 3);
}

int main() {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}
