#pragma once

#include <bits/types/FILE.h>
#include <fstream>
#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <queue>

#include "print.hpp"

#define LOG_DEBUG(logger, message) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::DEBUG, (message))
#define LOG_INFO(logger, message) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::INFO, (message))
#define LOG_WARNING(logger, message) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::WARNING, (message))
#define LOG_ERROR(logger, message) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::ERROR, (message))
#define LOG_FATAL(logger, message) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::FATAL, (message))


namespace utils {

enum class LogLevel : int { DEBUG = 0, INFO, WARNING, ERROR, FATAL };


class LoggerManager {
public:
    struct Logger {
        friend class LoggerManager;

        Logger() = default;
    private:
        std::string logger_name;
        std::string path;
    };

    void enable_async_logging();

    void disable_async_logging();

    Logger& get_logger(std::string logger_name, std::string path = "");

    static LoggerManager& get_instance();

    void log(const Logger& logger, LogLevel log_level,  std::string message);

    void async_log(LogLevel log_level, const Logger& logger, std::string message);

    void set_log_path(Logger& logger, const std::string& path);

    ~LoggerManager();

private:
    LoggerManager();
    
    static std::queue<std::tuple<LogLevel, Logger, std::string>> m_log_queue;

    static std::unordered_map<std::string, Logger> m_loggers;

    static std::unordered_map<std::string, std::fstream> m_files;
    
    static std::mutex m_file_mutex;

    static std::shared_ptr<LoggerManager> instance;

    std::thread m_log_thread;
};


} // namespace utils
