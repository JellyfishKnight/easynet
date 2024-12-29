#include "logger.hpp"

#include <iostream>
#include <gtest/gtest.h>


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
    LOG_DEBUG(logger, "This is a debug message");
    LOG_INFO(logger, "This is an info message");
    LOG_WARNING(logger, "This is a warning message");
    LOG_ERROR(logger, "This is an error message");
    LOG_FATAL(logger, "This is a fatal message");
}

TEST_F(LoggerTest, TestAsyncLogger) {
    utils::LoggerManager::get_instance().enable_async_logging();
    LOG_DEBUG(logger, "This is a debug message");
    LOG_INFO(logger, "This is an info message");
    LOG_WARNING(logger, "This is a warning message");
    LOG_ERROR(logger, "This is an error message");
    LOG_FATAL(logger, "This is a fatal message");
    utils::LoggerManager::get_instance().disable_async_logging();
}

TEST_F(LoggerTest, TestLoggerWithDifferentLoggers) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger1.log");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2", "logger2.log");

    LOG_DEBUG(logger1, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");

    LOG_DEBUG(logger2, "This is a debug message");
    LOG_INFO(logger2, "This is an info message");
    LOG_WARNING(logger2, "This is a warning message");
    LOG_ERROR(logger2, "This is an error message");
    LOG_FATAL(logger2, "This is a fatal message");
}

TEST_F(LoggerTest, TestLoggerWithDifferentPaths) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger1.log");

    LOG_DEBUG(logger1, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");

    utils::LoggerManager::get_instance().set_log_path(logger1, "logger2.log");

    LOG_DEBUG(logger1, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");
}

TEST_F(LoggerTest, TestLoggerWithSamePaths) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1", "logger.log");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2", "logger.log");

    LOG_DEBUG(logger1, "This is a debug message");
    LOG_DEBUG(logger2, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_INFO(logger2, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_WARNING(logger2, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_ERROR(logger2, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");
    LOG_FATAL(logger2, "This is a fatal message");

    utils::LoggerManager::get_instance().enable_async_logging();
    LOG_DEBUG(logger1, "This is a debug message");
    LOG_DEBUG(logger2, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_INFO(logger2, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_WARNING(logger2, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_ERROR(logger2, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");
    LOG_FATAL(logger2, "This is a fatal message");
    utils::LoggerManager::get_instance().disable_async_logging();
}

TEST_F(LoggerTest, TestLoggerOutputToConsole) {
    auto logger1 = utils::LoggerManager::get_instance().get_logger("logger1");
    auto logger2 = utils::LoggerManager::get_instance().get_logger("logger2");

    LOG_DEBUG(logger1, "This is a debug message");
    LOG_DEBUG(logger2, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_INFO(logger2, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_WARNING(logger2, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_ERROR(logger2, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");
    LOG_FATAL(logger2, "This is a fatal message");

    utils::LoggerManager::get_instance().enable_async_logging();
    LOG_DEBUG(logger1, "This is a debug message");
    LOG_DEBUG(logger2, "This is a debug message");
    LOG_INFO(logger1, "This is an info message");
    LOG_INFO(logger2, "This is an info message");
    LOG_WARNING(logger1, "This is a warning message");
    LOG_WARNING(logger2, "This is a warning message");
    LOG_ERROR(logger1, "This is an error message");
    LOG_ERROR(logger2, "This is an error message");
    LOG_FATAL(logger1, "This is a fatal message");
    LOG_FATAL(logger2, "This is a fatal message");
    utils::LoggerManager::get_instance().disable_async_logging();
}



int main() {
    ::testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}