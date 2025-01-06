#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
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


#define LOG_FOREACH_LOG_LEVEL(f) \
    f(DEBUG) \
    f(INFO) \
    f(WARN) \
    f(ERROR) \
    f(FATAL)

#if defined(__linux__)
#define _IF_HAS_ANSI_COLORS(x) x
#endif


namespace utils {

enum class LogLevel : int {
#define _FUNCTION(name) name,
    LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
};

template <class T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template <class U> requires std::constructible_from<T, U>
    consteval with_source_location(U &&inner, std::source_location loc)
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
                    const auto& [log_level, logger, message, loc] = m_log_queue.front();
                    generate_log(logger, log_level, message, loc);
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
        if (m_loggers.find(logger_name) == m_loggers.end()) [[likely]] {
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
        if (instance == nullptr) [[unlikely]] {
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
        generate_log(logger, log_level, msg, loc);
    }

    template<typename... Args>
    void async_log(
        LogLevel log_level,
        const Logger& logger,
        with_source_location<std::format_string<Args...>> fmt,
        Args&&... args
    ) {
        auto message = std::vformat(fmt.format().get(), std::make_format_args(args...));
        m_log_queue.push(std::make_tuple(log_level, logger, message, fmt.location()));
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

    void generate_log(const Logger& logger, LogLevel log_level, std::string message, const std::source_location &loc) {
        if (log_level < min_level) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_file_mutex);
        std::chrono::zoned_time now{std::chrono::current_zone(), std::chrono::system_clock::now()};

        auto msg = std::format("{} {}:{} [{}][{}]:{}", now, loc.file_name(), loc.line(), get_log_level_name(log_level), logger.logger_name, message);
        if (logger.path.empty()) {
            // log to console
            std::cout << _IF_HAS_ANSI_COLORS(k_level_ansi_colors[(std::uint8_t)log_level]) +
                    msg + _IF_HAS_ANSI_COLORS(k_reset_ansi_color) << std::endl;
            return;
        }
        if (m_files.find(logger.logger_name) == m_files.end()) {
            m_files[logger.logger_name].open(logger.path, std::ios::out | std::ios::app);
        }

        m_files[logger.logger_name]
            << _IF_HAS_ANSI_COLORS(k_level_ansi_colors[(std::uint8_t)log_level] +)
                    msg _IF_HAS_ANSI_COLORS(+ k_reset_ansi_color) << std::endl;
        std::cout << msg << std::endl;
    }

    std::string get_log_level_name(LogLevel log_level) {
        const static std::unordered_map<LogLevel, std::string> log_level_map = {
#define _FUNCTION(name) \
            { LogLevel::name, #name },
LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
        };
        return log_level_map.at(log_level);
    }
    
    inline static std::queue<std::tuple<LogLevel, Logger, std::string, std::source_location>> m_log_queue = {};

    inline static std::unordered_map<std::string, Logger> m_loggers = {};

    inline static std::unordered_map<std::string, std::fstream> m_files = {};
    
    inline static std::mutex m_file_mutex = {};

    inline static LoggerManager* instance = nullptr;

    inline static std::thread m_log_thread = {};

    inline static bool m_async_logging_enabled = false;

    inline static LogLevel min_level = std::getenv("LOG_LEVEL") ? static_cast<LogLevel>(std::stoi(std::getenv("LOG_LEVEL"))) : LogLevel::INFO;

#if defined(__linux__)
    inline static constexpr char k_level_ansi_colors[static_cast<uint8_t>(LogLevel::FATAL) + 1][8] = {
        "\033[32m",
        "\033[34m",
        "\033[33m",
        "\033[31m",
        "\033[31;1m",
    };
    inline static constexpr char k_reset_ansi_color[4] = "\033[m";
#else 
#define _IF_HAS_ANSI_COLORS(x)
    inline static constexpr char k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][1] = {
        "",
        "",
        "",
        "",
        "",
    };
    inline static constexpr char k_reset_ansi_color[1] = "";
#endif


};

} // namespace utils


#define _FUNCTION(name) \
template <typename... Args> \
void net_log_##name(const ::utils::LoggerManager::Logger& logger, utils::with_source_location<std::format_string<Args...>> fmt, Args &&...args) { \
    return ::utils::LoggerManager::get_instance().log(logger, utils::LogLevel::name, std::move(fmt), std::forward<Args>(args)...); \
}
LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION

#define _FUNCTION(name) \
template <typename... Args> \
void net_async_log_##name(const utils::LoggerManager::Logger& logger, utils::with_source_location<std::format_string<Args...>> fmt, Args &&...args) { \
    return ::utils::LoggerManager::get_instance().async_log(logger, utils::LogLevel::name, std::move(fmt), std::forward<Args>(args)...); \
}
LOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION


#define NET_LOG_DEBUG(logger, format, ...) net_log_DEBUG(logger, {format, std::source_location::current()} __VA_OPT__(,) ##__VA_ARGS__)
#define NET_LOG_INFO(logger, format, ...) net_log_INFO(logger,  {format, std::source_location::current()} __VA_OPT__(,) ##__VA_ARGS__)
#define NET_LOG_WARN(logger, format, ...) net_log_WARN(logger,  {format, std::source_location::current()} __VA_OPT__(,) ##__VA_ARGS__)
#define NET_LOG_ERROR(logger, format, ...) net_log_ERROR(logger,  {format, std::source_location::current()} __VA_OPT__(,) ##__VA_ARGS__)
#define NET_LOG_FATAL(logger, format, ...) net_log_FATAL(logger,  {format, std::source_location::current()} __VA_OPT__(,) ##__VA_ARGS__)

#define NET_ENABLE_ASYNC_LOGGING() utils::LoggerManager::get_instance().enable_async_logging()
#define NET_DISABLE_ASYNC_LOGGING() utils::LoggerManager::get_instance().disable_async_logging()

#define NET_LOG_DEBUG_ASYNC(logger, format, ...) net_async_net_log_DEBUG(logger, { format, std::source_location::current() } __VA_OPT__(, )##__VA_ARGS__)
#define NET_LOG_INFO_ASYNC(logger, format, ...) net_async_net_log_INFO(logger, { format, std::source_location::current() } __VA_OPT__(, )##__VA_ARGS__)
#define NET_LOG_WARN_ASYNC(logger, format, ...) net_async_net_log_WARN(logger, { format, std::source_location::current() } __VA_OPT__(, )##__VA_ARGS__)
#define NET_LOG_ERROR_ASYNC(logger, format, ...) net_async_net_log_ERROR(logger, { format, std::source_location::current() } __VA_OPT__(, )##__VA_ARGS__)
#define NET_LOG_FATAL_ASYNC(logger, format, ...) net_async_net_log_FATAL(logger, { format, std::source_location::current() } __VA_OPT__(, )##__VA_ARGS__)
