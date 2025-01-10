#include "thread_pool.hpp"
#include <future>
#include <gtest/gtest.h>
#include <thread>

// class ThreadPoolTest: public ::testing::Test {
// protected:
//     void SetUp() override {
//         pool = std::make_unique<utils::ThreadPool>(4);
//     }

//     void TearDown() override {
//         pool.reset();
//     }

//     std::unique_ptr<utils::ThreadPool> pool;
// };


// TEST_F(ThreadPoolTest, TestThreadPool) {
//     std::vector<std::future<int>> futures(2);
//     for (int i = 0; i < 2; ++i) {
//         auto f = [](int a, int b) { return a + b; };
//         auto opt = pool->submit(f, i, i + 1);
//         if (opt.has_value()) {
//             // try {
//             // futures[i] = std::move(opt.value());
//             std::cout << opt->get() << std::endl;
//             // } catch (const std::future_error& e) {
//             //     FAIL() << e.what();
//             // }
//         } else {
//             FAIL() << "Failed to submit task";
//         }
//     }
//     // for (int i = 0; i < 2; ++i) {
//     //     try {
//     //         // ASSERT_EQ(futures[i].get(), i + i + 1);
//     //         std::cout << futures[i].get() << std::endl;
//     //     } catch (const std::future_error& e) {
//     //         FAIL() << e.what();
//     //     }
//     // }
// }

// // TEST_F(ThreadPoolTest, AddTaskWithName) {
// //     auto f = [](int a, int b) { return a + b; };
// //     std::vector<std::future<int>> futures;
// //     for (int i = 0; i < 10; ++i) {
// //         auto opt = pool->submit(std::to_string(i), f, i, i + 1);
// //         if (opt.has_value()) {
// //             futures.emplace_back(std::move(opt.value()));
// //         } else {
// //             FAIL() << "Failed to submit task";
// //         }
// //     }
// //     for (int i = 0; i < 10; ++i) {
// //         ASSERT_EQ(futures[i].get(), i + i + 1);
// //     }
// // }

// // TEST_F(ThreadPoolTest, GetStatusOfTask) {
// //     auto f = [](int a, int b) {
// //         std::this_thread::sleep_for(std::chrono::seconds(3));
// //         return a + b;
// //     };
// //     std::vector<std::future<int>> futures;
// //     for (int i = 0; i < 10; ++i) {
// //         futures.emplace_back(*pool->submit(std::to_string(i), f, i, i + 1));
// //     }
// //     for (int i = 0; i < 10; ++i) {
// //         auto status = pool->get_task_status(std::to_string(i));
// //         ASSERT_TRUE(status.has_value());
// //         ASSERT_EQ(std::get<0>(status.value()), utils::TaskStatus::FINISHED);
// //     }
// // }

// int main() {
//     testing::InitGoogleTest();

//     return RUN_ALL_TESTS();
// }

int main() {
    utils::ThreadPool pool(2);

    for (int i = 0; i < 10; ++i) {
        auto f = [](int a, int b) { std::cout << a << " " << b << std::endl; };
        auto opt = pool.submit(f, i, i + 1);
    }
}