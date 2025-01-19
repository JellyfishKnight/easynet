#pragma once

#include <bits/utility.h>
#include <cstddef>
#include <string_view>
#include <utility>

/*************from string to enum***************/

/**
 * @brief Extracts the value of the key "CrItMaGiC" from a string.
 */
constexpr std::string_view try_extract_value(std::string_view str) {
    auto critpos = str.find("CrItMaGiC =  ") + 12;
    auto endpos = str.find_first_of(";]");
    auto slice = str.substr(critpos, endpos - critpos);
    return slice;
}

/**
 * @brief Removes the prefix "CrItMaGiC =  " from a string.
 */
constexpr std::string_view try_remove_prefix(std::string_view str, std::string_view prefix) {
    if (!str.empty()) {
        if (str.front() == '(') {
            return {};
        }
        if (str.find(prefix) == 0 && str.find("::", prefix.size(), 2) == prefix.size()) {
            return str.substr(prefix.size() + 2);
        }
    }
    return str;
}

template<typename CrItMaGiC>
constexpr std::string_view enum_type_name() {
    constexpr std::string_view name = try_extract_value(__PRETTY_FUNCTION__);
    return name;
}

template<auto CrItMaGiC>
constexpr std::string_view enum_value_name() {
    constexpr auto type = enum_type_name<decltype(CrItMaGiC)>();
    constexpr std::string_view name =
        try_remove_prefix(try_extract_value(__PRETTY_FUNCTION__), type);
    return name;
}

#if __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#endif

template<typename Enum, std::size_t I0, std::size_t I1>
constexpr std::size_t enum_range_impl() {
    if constexpr (I0 + 1 == I1) {
        return I1;
    } else {
        constexpr std::size_t I = (I0 + I1) / 2;
        if constexpr (!enum_value_name<static_cast<Enum>(I)>().empty()) {
            return enum_range_impl<Enum, I, I1>();
        } else {
            return enum_range_impl<Enum, I0, I>();
        }
    }
}

template<typename E>
constexpr std::size_t enum_range() {
    return enum_range_impl<E, 0, 256>();
}

#if __clang__
    #pragma clang diagnostic pop
#endif

template<typename Enum, std::size_t... Is>
constexpr std::string_view dump_enum_impl(Enum value, std::index_sequence<Is...>) {
    std::string_view ret;
    (void
    )((value == static_cast<Enum>(Is) && ((ret = enum_value_name<static_cast<Enum>(Is)>()), false))
      || ...);
    return ret;
}

template<typename Enum>
constexpr std::string_view dump_enum(Enum value) {
    return dump_enum_impl(value, std::make_index_sequence<enum_range<Enum>()>());
}

template<typename Enum, std::size_t... Is>
constexpr Enum parse_enum_impl(std::string_view name, std::index_sequence<Is...>) {
    std::size_t ret;
    (void)((name == enum_value_name<static_cast<Enum>(Is)>() && ((ret = Is), false)) || ...);
    return static_cast<Enum>(ret);
}

template<typename Enum>
constexpr Enum parse_enum(std::string_view name) {
    return parse_enum_impl<Enum>(name, std::make_index_sequence<enum_range<Enum>()>());
}

/*************from enum to string***************/