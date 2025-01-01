#pragma once

#include <iostream>
#include <concepts>
#include <ranges>
#include <type_traits>

namespace utils {

template<typename BasicType>
    requires std::is_fundamental_v<BasicType>
void print(BasicType value) {
    std::cout << value << std::endl;
}

template<typename BasicType>
    requires std::is_fundamental_v<BasicType>
void print(const BasicType* array, size_t size) {
    std::cout << "[";
    for (size_t i = 0; i < size; ++i) {
        if (i == size - 1) [[unlikely]] {
            std::cout << array[i];
        } else {
            std::cout << array[i] << " ";
        }
    }
    std::cout << "]" <<std::endl;
}

// print container type
template<typename ContainerType>
requires requires {
    typename ContainerType::iterator;
}
void print(const ContainerType& container) {
    if constexpr (std::is_same_v<ContainerType, std::string>) {
        std::cout << container << std::endl;
        return;
    }
    std::cout << "[";
    for (const auto it = container.begin(); it != container.end(); ++it) {
        // last one do not print " "
        if (it == container.end() - 1) [[unlikely]] {
            std::cout << *it;
        } else {
            std::cout << *it << " ";
        }
    }
    std::cout << "]" <<std::endl;
}

// print string type
inline void print(const char* value) {
    std::cout << value << std::endl;
}


template<char format, typename T>
struct TypeChecker;

template<typename T>
struct TypeChecker<'d', T>: std::is_same<std::decay_t<T>, int> {};

template<typename T>
struct TypeChecker<'f', T>: std::is_same<std::decay_t<T>, double> {};

template<typename T>
struct TypeChecker<'s', T>: std::is_same<std::decay_t<T>, std::string> {};

template<typename T>
struct TypeChecker<'p', T>: std::is_pointer<T> {};

template<typename T>
struct TypeChecker<'c', T>: std::is_same<std::decay_t<T>, char> {};

template<typename T>
struct TypeChecker<'b', T>: std::is_same<std::decay_t<T>, bool> {};

template<typename T>
struct TypeChecker<'x', T>: std::is_integral<std::decay_t<T>> {};

template<typename T>
struct TypeChecker<'X', T>: std::is_integral<std::decay_t<T>> {};

template<typename T>
struct TypeChecker<'o', T>: std::is_integral<std::decay_t<T>> {};

template<typename T>
struct TypeChecker<'u', T>: std::is_integral<std::decay_t<T>> {};

template<typename T>
void check_type(T) {
    static_assert(T::value, "Type mismatch.");
}


template <typename... Args>
void handle_printf(const std::string& format, Args&&... args) {
    const char* fmt = format.c_str();
    std::tuple<Args...> arguments(std::forward<Args>(args)...);
    size_t index = 0;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // 跳过 '%'
            if (*fmt == '\0') break;

            char specifier = *fmt++;
            if (index >= sizeof...(Args)) {
                throw std::runtime_error("Too few arguments provided.");
            }

            std::apply([&](auto&&...) {
                auto arg = std::get<index>(arguments);
                if constexpr (index < sizeof...(Args)) {
                    check_type<decltype(arg)>(TypeChecker<specifier, args>); // 检查类型
                }
            }, arguments);

            index++;
        } else {
            std::cout << *fmt++;
        }
    }

    if (index != sizeof...(Args)) {
        throw std::runtime_error("Too many arguments provided.");
    }
}

template <typename... Args>
void print(const std::string& format, Args&&... args) {
    if (format.find("{}") != std::string::npos || format.find('%') != std::string::npos) {
        handle_printf(format, std::forward<Args>(args)...);
    } else {
        // 普通字符串
        std::cout << format << std::endl;
    }
}




} // namespace utils