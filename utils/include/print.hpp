#pragma once

#include <iostream>
#include <concepts>
#include <ostream>
#include <ranges>
#include <type_traits>
#include <format>
#include <source_location>

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
    std::cout << "]" << std::endl;
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
    
    for (auto it = container.begin(); it != container.end(); ++it) {
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

// print with format
template<typename... Args>
void print(const std::format_string<Args...>& fmt, Args&&... args) {
    std::cout << std::format(std::move(fmt), std::forward<Args>(std::move(args))...) << std::endl;
}


} // namespace utils