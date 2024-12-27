#include "logger.hpp"

namespace utils {

LoggerManager& LoggerManager::get_instance() {
    static LoggerManager instance;
    return instance;
}


} // namespace utils