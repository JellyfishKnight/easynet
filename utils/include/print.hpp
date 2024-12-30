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

// template<typename... BasicType>
// void print(const std::string& format, BasicType... value) {

// }

template<typename BasicType>
    requires std::is_fundamental_v<BasicType>
void print(const BasicType* array, size_t size) {
    std::cout << "[";
    for (size_t i = 0; i < size; ++i) {
        std::cout << array[i] << " ";
    }
    std::cout << "]" <<std::endl;
}

// print container type
template<typename ContainerType>
    requires std::ranges::range<ContainerType>
void print(const ContainerType& container) {
    if constexpr (std::is_same_v<ContainerType, std::string>) {
        std::cout << container << std::endl;
        return;
    }
        
    for (const auto& element : container) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}




} // namespace utils