module;
#include <meta>
export module erdo:meta;

import std;

export {
    template <typename E>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E))
    consteval auto enumerators_of()
    {
        std::array<E, std::meta::enumerators_of(^^E).size()> r{};
        std::size_t i = 0;
        template for (constexpr auto info : std::define_static_array(std::meta::enumerators_of(^^E))) r[i++] = [:info:];
        return r;
    }

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E) && (std::same_as<D, std::nullopt_t> || std::convertible_to<D, std::string_view>))
    constexpr std::string_view enum_to_string(E value, D default_ = std::nullopt)
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) if (value == [:e:]) return std::meta::identifier_of(e);

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("'{}' is not a valid enumerator of enum {}", std::to_underlying(value), std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E) && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E string_to_enum(std::string_view str, D default_ = std::nullopt)
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) if (str == std::meta::identifier_of(e)) return [:e:];

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("string '{}' does not correspond to any enumerator of enum {}", str, std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E) && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E index_to_enum(std::size_t index, D default_ = std::nullopt)
    {
        constexpr auto enumerators = enumerators_of<E>();

        if (index < enumerators.size())
            return enumerators[index];

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("index {} is out of range for enum {}", index, std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E) && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E integral_to_enum(std::underlying_type_t<E> integral, D default_ = std::nullopt)
    {
        for (auto [i, enumerator] : enumerators_of<E>() | std::views::enumerate)
            if (integral == std::to_underlying(enumerator))
                return enumerator;

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("integral {} does not correspond to any enumerator of enum {}", integral, std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E))
    constexpr bool is_valid_enum_integral(std::underlying_type_t<E> integral)
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) if (integral == std::to_underlying([:e:])) return true;

        return false;
    }
}