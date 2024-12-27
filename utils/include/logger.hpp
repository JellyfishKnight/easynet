#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <queue>

#include "print.hpp"

namespace utils {

enum class LogLevel : int { DEBUG = 0, INFO, WARNING, ERROR, FATAL };

#define LOG_DEBUG(logger, message) LoggerManager::log(LogLevel::DEBUG, logger, (message))
#define LOG_INFO(logger, message) LoggerManager::log(LogLevel::INFO, logger, (message))
#define LOG_WARNING(logger, message) LoggerManager::log(LogLevel::WARNING, logger, (message))
#define LOG_ERROR(logger, message) LoggerManager::log(LogLevel::ERROR, logger, (message))
#define LOG_FATAL(logger, message) LoggerManager::log(LogLevel::FATAL, logger, (message))


class LoggerManager {
private:
    struct Logger {
        friend class LoggerManager;
    private:
        std::string logger_name;
        std::string path;
        LogLevel log_level;
    };
public:
    const Logger& get_logger(std::string logger_name, std::string path);

    static LoggerManager& get_instance();

    void log(LogLevel log_level, Logger logger, std::string message);

    void async_log(LogLevel log_level, Logger logger, std::string message);

    void set_log_path(const Logger& logger, std::string path);

    void set_log_level(LogLevel log_level);

    ~LoggerManager() = default;

private:
    LoggerManager() = default;
    
    static std::queue<std::tuple<LogLevel, Logger, std::string>> m_log_queue;

    static std::unordered_map<std::string, Logger> m_loggers;

    static std::unordered_map<Logger, std::fstream> m_files;

    static std::mutex m_file_mutex;

    static LoggerManager m_instance;
};


} // namespace utils
