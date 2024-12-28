#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <queue>

#include "print.hpp"

#define LOG_DEBUG(logger, message) utils::LoggerManager::log(LogLevel::DEBUG, logger, (message))
#define LOG_INFO(logger, message) utils::LoggerManager::log(LogLevel::INFO, logger, (message))
#define LOG_WARNING(logger, message) utils::LoggerManager::log(LogLevel::WARNING, logger, (message))
#define LOG_ERROR(logger, message) utils::LoggerManager::log(LogLevel::ERROR, logger, (message))
#define LOG_FATAL(logger, message) utils::LoggerManager::log(LogLevel::FATAL, logger, (message))


namespace utils {

enum class LogLevel : int { DEBUG = 0, INFO, WARNING, ERROR, FATAL };


class LoggerManager {
private:
    struct Logger {
        friend class LoggerManager;

        Logger() = default;
    private:
        std::string logger_name;
        std::string path;
    };
public:
    Logger& get_logger(std::string logger_name, std::string path = "");

    static LoggerManager& get_instance();

    void log(const Logger& logger, LogLevel log_level,  std::string message);

    void async_log(LogLevel log_level, const Logger& logger, std::string message);

    void set_log_path(Logger& logger, const std::string& path);

    ~LoggerManager() = default;

private:
    LoggerManager() = default;
    
    static std::queue<std::tuple<LogLevel, Logger, std::string>> m_log_queue;

    static std::unordered_map<std::string, Logger> m_loggers;

    static std::unordered_map<Logger, std::fstream> m_files;

    static std::mutex m_file_mutex;

    static std::shared_ptr<LoggerManager> instance;
};


} // namespace utils
