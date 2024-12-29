#pragma once

#include <iostream>
#include <concepts>
#include <ranges>


namespace utils {

template<typename BasicType>
void print(BasicType value) {
    std::cout << value << std::endl;
}

// print container type
template<typename ContainerType>
    requires std::ranges::range<ContainerType>
void print(const ContainerType& container) {
    for (const auto& element : container) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}






} // namespace utils