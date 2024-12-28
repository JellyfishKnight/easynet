#include "logger.hpp"
#include <iostream>
#include <fstream>


namespace utils {

LoggerManager& LoggerManager::get_instance() {
    if (instance == nullptr) {
        instance = std::make_shared<LoggerManager>();
    } 
    return *instance;
}

LoggerManager::Logger& LoggerManager::get_logger(std::string logger_name, std::string path) {
    if (m_loggers.find(logger_name) == m_loggers.end()) {
        m_loggers[logger_name].logger_name = logger_name;
        m_loggers[logger_name].path = path;
    }
    return m_loggers[logger_name];
}

void LoggerManager::log(const Logger& logger, LogLevel log_level, std::string message) {
    const static std::unordered_map<LogLevel, std::string> log_level_map = {
        { LogLevel::DEBUG, "DEBUG" },
        { LogLevel::INFO, "INFO" },
        { LogLevel::WARNING, "WARNING" },
        { LogLevel::ERROR, "ERROR" },
        { LogLevel::FATAL, "FATAL" }
    };

    std::lock_guard<std::mutex> lock(m_file_mutex);
    if (m_files.find(logger) == m_files.end()) {
        m_files[logger].open(logger.path, std::ios::out | std::ios::app);
    }
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    m_files[logger] << std::ctime(&now_c) << " [" << log_level_map.at(log_level) << "] " << message << std::endl;
}

void LoggerManager::async_log(LogLevel log_level, const Logger& logger, std::string message) {
    m_log_queue.push(std::make_tuple(log_level, logger, message));
}

void LoggerManager::set_log_path(Logger& logger, const std::string& path) {
    logger.path = path;
}

std::shared_ptr<LoggerManager> LoggerManager::instance = nullptr;

std::queue<std::tuple<LogLevel, LoggerManager::Logger, std::string>> LoggerManager::m_log_queue;

std::unordered_map<std::string, LoggerManager::Logger> LoggerManager::m_loggers;

std::unordered_map<LoggerManager::Logger, std::fstream> LoggerManager::m_files;

std::mutex LoggerManager::m_file_mutex;

} // namespace utils