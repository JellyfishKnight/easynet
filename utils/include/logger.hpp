#pragma once

#include <cassert>
#include <format>
#include <fstream>
#include <memory>
#include <source_location>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <queue>
#include <source_location>
#include <iostream>



#define LOG_DEBUG(logger, fmt, ...) utils::LoggerManager::get_instance().log(logger, utils::LogLevel::DEBUG, fmt, ##__VA_ARGS__)



namespace utils {

enum class LogLevel : int { DEBUG = 0, INFO, WARNING, ERROR, FATAL };

template <class T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template <class U> requires std::constructible_from<T, U>
    consteval with_source_location(U &&inner, std::source_location loc = std::source_location::current())
        : inner(std::forward<U>(inner)), loc(std::move(loc)) {}

    constexpr T const& format() const {
        return inner;
    }

    constexpr std::source_location const& location() const {
        return loc;
    }
};



class LoggerManager {
public:
    struct Logger {
        friend class LoggerManager;

        Logger() = default;
    private:
        std::string logger_name;
        std::string path;
    };

    void enable_async_logging() {
        assert(instance != nullptr);
        // if thread is already running, return
        if (m_log_thread.joinable()) {
            return;
        }
        m_async_logging_enabled = true;
        m_log_thread = std::thread([&] {
            while (instance != nullptr && m_async_logging_enabled) {
                if (!m_log_queue.empty()) {
                    auto [log_level, logger, message] = m_log_queue.front();
                    // log(logger, log_level, message);
                    m_log_queue.pop();
                }
            }
        });

    }

    void disable_async_logging() {
        m_async_logging_enabled = false;
        m_log_thread.join();
    }

    Logger& get_logger(std::string logger_name, std::string path = "") {
        if (m_loggers.find(logger_name) == m_loggers.end()) {
            m_loggers[logger_name].logger_name = logger_name;
            m_loggers[logger_name].path = path;
        } else {
            if (path != m_loggers[logger_name].path) {
                set_log_path(m_loggers[logger_name], path);
            }
        }
        return m_loggers[logger_name];
    }

    static LoggerManager& get_instance() {
        if (instance == nullptr) {
            instance = new LoggerManager();
        } 
        return *instance;
    }

    template<typename... Args>
    void log(const Logger& logger,
        LogLevel log_level,
        with_source_location<std::format_string<Args...>> fmt,
        Args&&... args) {
        const auto& loc = fmt.location();
        auto msg = std::vformat(fmt.format().get(), std::make_format_args(args...));
        log(logger, log_level, msg, loc);
    }

    void log(const Logger& logger, LogLevel log_level, std::string message, const std::source_location &loc) {
        const static std::unordered_map<LogLevel, std::string> log_level_map = {
            { LogLevel::DEBUG, "DEBUG" },
            { LogLevel::INFO, "INFO" },
            { LogLevel::WARNING, "WARNING" },
            { LogLevel::ERROR, "ERROR" },
            { LogLevel::FATAL, "FATAL" }
        };

        std::lock_guard<std::mutex> lock(m_file_mutex);
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss_time;
        oss_time << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

        auto msg = std::format("{} {}:{} [{}][{}]:{}", oss_time.str(), loc.file_name(), loc.line(), log_level_map.at(log_level), logger.logger_name, message);

        if (logger.path.empty()) {
            // log to console
            std::cout << msg << std::endl;
            return;
        }
        if (m_files.find(logger.logger_name) == m_files.end()) {
            m_files[logger.logger_name].open(logger.path, std::ios::out | std::ios::app);
        }

        m_files[logger.logger_name] << msg << std::endl;
    }


    void async_log(LogLevel log_level, const Logger& logger, std::string message) {
        m_log_queue.push(std::make_tuple(log_level, logger, message));
    }

    void set_log_path(Logger& logger, const std::string& path) {
        logger.path = path;
    }

    ~LoggerManager() {
        for (auto& [logger_name, file] : m_files) {
            file.close();
        }
        delete instance;

        m_log_thread.join();
    }

private:
    LoggerManager() {};
    
    inline static std::queue<std::tuple<LogLevel, Logger, std::string>> m_log_queue = {};

    inline static std::unordered_map<std::string, Logger> m_loggers = {};

    inline static std::unordered_map<std::string, std::fstream> m_files = {};
    
    inline static std::mutex m_file_mutex = {};

    inline static LoggerManager* instance = nullptr;

    inline static std::thread m_log_thread = {};

    inline static bool m_async_logging_enabled = false;
};


} // namespace utils
