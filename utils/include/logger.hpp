#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <queue>

#include "print.hpp"

namespace utils {

enum class LogLevel : int { DEBUG = 0, INFO, WARNING, ERROR, FATAL };


class LoggerManager {
private:
    struct Logger {
        std::string logger_name;
        std::string path;
        LogLevel log_level;
    };


public:
    const Logger& get_logger(std::string logger_name, std::string path);

    void log(LogLevel log_level, Logger logger, std::string message);

    void async_log(LogLevel log_level, Logger logger, std::string message);

    void set_log_path(const Logger& logger, std::string path);

    void set_log_level(LogLevel log_level);

    ~LoggerManager() = default;

private:
    static std::queue<std::pair<LogLevel, std::string>> m_log_queue;

    static std::unordered_map<std::string, Logger> m_loggers;

    static std::unordered_map<Logger, std::fstream> m_files;
    
    std::mutex m_file_mutex;
    
    std::string m_log_path;
};


} // namespace utils
