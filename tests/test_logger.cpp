#include "logger.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
#include <thread>

class LoggerTest: public ::testing::Test {
protected:
    void SetUp() override {
        logger = utils::LoggerManager::get_instance().get_logger("test_logger", "test.log");
    }

    void TearDown() override {
        // close the file
    }

    utils::LoggerManager::Logger logger;
};

TEST_F(LoggerTest, TestLogger) {
    NET_LOG_DEBUG(logger, "This is a debug message");
    NET_LOG_INFO(logger, "This is an info message");
    NET_LOG_WARN(logger, "This is a warning message");
    NET_LOG_ERROR(logger, "This is an error message");
    NET_LOG_FATAL(logger, "This is a fatal message");
}

TEST_F(LoggerTest, TestAsyncLogger) {
    NET_ENABLE_ASYNC_LOGGING();
    NET_LOG_DEBUG(logger, "This is a debug message");
    NET_LOG_INFO(logger, "This is an info message");
    NET_LOG_WARN(logger, "This is a warning message");
    NET_LOG_ERROR(logger, "This is an error message");
    NET_LOG_FATAL(logger, "This is a fatal message");
    NET_DISABLE_ASYNC_LOGGING();
}

TEST_F(LoggerTest, TestLoggerWithDifferentLoggers) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger1.log");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2", "logger2.log");

    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");

    NET_LOG_DEBUG(logger2, "This is a debug message");
    NET_LOG_INFO(logger2, "This is an info message");
    NET_LOG_WARN(logger2, "This is a warning message");
    NET_LOG_ERROR(logger2, "This is an error message");
    NET_LOG_FATAL(logger2, "This is a fatal message");
}

TEST_F(LoggerTest, TestLoggerWithDifferentPaths) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger1.log");

    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");

    utils::LoggerManager::get_instance().set_log_path(logger1, "logger2.log");

    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");
}

TEST_F(LoggerTest, TestLoggerWithSamePaths) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger.log");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2", "logger.log");

    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_DEBUG(logger2, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_INFO(logger2, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_WARN(logger2, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_ERROR(logger2, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");
    NET_LOG_FATAL(logger2, "This is a fatal message");

    utils::LoggerManager::get_instance().enable_async_logging();
    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_DEBUG(logger2, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_INFO(logger2, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_WARN(logger2, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_ERROR(logger2, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");
    NET_LOG_FATAL(logger2, "This is a fatal message");
    utils::LoggerManager::get_instance().disable_async_logging();
}

TEST_F(LoggerTest, TestLoggerOutputToConsole) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2");

    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_DEBUG(logger2, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_INFO(logger2, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_WARN(logger2, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_ERROR(logger2, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");
    NET_LOG_FATAL(logger2, "This is a fatal message");

    utils::LoggerManager::get_instance().enable_async_logging();
    NET_LOG_DEBUG(logger1, "This is a debug message");
    NET_LOG_DEBUG(logger2, "This is a debug message");
    NET_LOG_INFO(logger1, "This is an info message");
    NET_LOG_INFO(logger2, "This is an info message");
    NET_LOG_WARN(logger1, "This is a warning message");
    NET_LOG_WARN(logger2, "This is a warning message");
    NET_LOG_ERROR(logger1, "This is an error message");
    NET_LOG_ERROR(logger2, "This is an error message");
    NET_LOG_FATAL(logger1, "This is a fatal message");
    NET_LOG_FATAL(logger2, "This is a fatal message");
    utils::LoggerManager::get_instance().disable_async_logging();
}

TEST_F(LoggerTest, TestLoggerWithSourceLocation) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger_process_11111111", "multi_process_test.log");

    for (int i = 0; i < 10; ++i) {
        NET_LOG_WARN(logger1, "This is a warning message");
        // std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

int main() {
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}