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
