#include "thread_pool.hpp"
#include <gtest/gtest.h>

class ThreadPoolTest: public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<utils::ThreadPool>(4);
    }

    void TearDown() override {
        pool.reset();
    }

    std::unique_ptr<utils::ThreadPool> pool;
};

int main() {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}