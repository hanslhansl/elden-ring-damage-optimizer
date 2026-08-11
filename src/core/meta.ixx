module;
// #include <meta>
export module erdo:meta;

import std;

export namespace erdo
{
    template <typename E>
        requires std::is_enum_v<E>
    constexpr auto enum_string_mapping = nullptr;

    template <typename E>
        requires std::is_enum_v<E>
    consteval auto enumerators_of()
    {
        std::array<E, enum_string_mapping<E>.size()> r{};
        for (auto&& [from, to] : std::views::zip(enum_string_mapping<E>, r))
            to = from.first;

        return r;
    }
    /*template <typename E>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E))
    consteval auto enumerators_of()
    {
        std::array<E, std::meta::enumerators_of(^^E).size()> r{};
        std::size_t i = 0;
        template for (constexpr auto info : std::define_static_array(std::meta::enumerators_of(^^E)))
            r[i++] = [:info:];
        
        return r;
    }*/

    template <typename E>
        requires (std::is_enum_v<E> /*&& std::meta::is_enumerable_type(^^E)*/)
    consteval auto enumerator_integrals_of()
    {
        std::array<std::underlying_type_t<E>, enumerators_of<E>().size()> r{};
        for (auto&& [enumerator, integral] : std::views::zip(enumerators_of<E>(), r))
            integral = std::to_underlying(enumerator);

        return r;
    }

    template <typename E>
        requires (std::is_enum_v<E> /*&& std::meta::is_enumerable_type(^^E)*/)
    consteval auto enumerator_strings_of()
    {
        std::array<std::string_view, enumerators_of<E>().size()> r{};
        std::ranges::copy(enum_string_mapping<E> | std::views::values, r.begin());
        return r;
    }

    template <typename E>
        requires std::is_enum_v<E>
    constexpr std::string_view enum_to_string(E value)
    {
        for (auto e : enum_string_mapping<E>)
            if (value == e.first)
                return e.second;

        throw std::invalid_argument(std::format("'{}' is not a valid enumerator", std::to_underlying(value)));
    }
    /*template <typename E>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E))
    constexpr std::string_view enum_to_string(E value)
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) if (value == [:e:])
            return std::meta::identifier_of(e);

        throw std::invalid_argument(std::format("'{}' is not a valid enumerator of enum {}", std::to_underlying(value), std::meta::display_string_of(^^E)));
    }*/

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E string_to_enum(std::string_view str, D default_ = std::nullopt)
    {
        for (auto e : enum_string_mapping<E>)
            if (str == e.second)
                return e.first;

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("string '{}' does not correspond to any enumerator", str));
        else
            return default_;
    }
    /*template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> && std::meta::is_enumerable_type(^^E) && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E string_to_enum(std::string_view str, D default_ = std::nullopt)
    {
        template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) if (str == std::meta::identifier_of(e)) return [:e:];

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("string '{}' does not correspond to any enumerator of enum {}", str, std::meta::display_string_of(^^E)));
        else
            return default_;
    }*/

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> /* && std::meta::is_enumerable_type(^^E)*/ && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E index_to_enum(std::size_t index, D default_ = std::nullopt)
    {
        constexpr auto enumerators = enumerators_of<E>();

        if (index < enumerators.size())
            return enumerators[index];

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("index {} is out of range", index));
            // throw std::invalid_argument(std::format("index {} is out of range for enum {}", index, std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E, typename D = std::nullopt_t>
        requires(std::is_enum_v<E> /*&& std::meta::is_enumerable_type(^^E)*/ && (std::same_as<D, std::nullopt_t> || std::same_as<D, E>))
    constexpr E integral_to_enum(std::underlying_type_t<E> integral, D default_ = std::nullopt)
    {
        for (auto enumerator : enumerators_of<E>())
            if (integral == std::to_underlying(enumerator))
                return enumerator;

        if constexpr (std::same_as<D, std::nullopt_t>)
            throw std::invalid_argument(std::format("integral {} does not correspond to any enumerator", integral));
            // throw std::invalid_argument(std::format("integral {} does not correspond to any enumerator of enum {}", integral, std::meta::display_string_of(^^E)));
        else
            return default_;
    }

    template <typename E>
        requires (std::is_enum_v<E> /*&& std::meta::is_enumerable_type(^^E)*/)
    constexpr bool is_valid_enum_integral(std::underlying_type_t<E> integral)
    {
        return std::ranges::contains(enumerator_integrals_of<E>(), integral);
    }


    template<typename E, std::size_t I = 0, typename F>
        requires (std::is_enum_v<E> /*&& std::meta::is_enumerable_type(^^E)*/)
    constexpr decltype(auto) visit_enum(E e, F&& f)
    {
        if constexpr (I < enumerators_of<E>().size())
        {
            if (e == enumerators_of<E>()[I])
                return std::forward<F>(f)(std::integral_constant<E, enumerators_of<E>()[I]>{});
            else
                return visit_enum<E, I + 1>(e, std::forward<F>(f));
        }

        throw std::invalid_argument(std::format("integral {} is not a valid enumerator", std::to_underlying(e)));
    }


    template<typename T, typename Tuple, std::size_t... Is>
    constexpr std::size_t tuple_index_impl(std::index_sequence<Is...>)
    {
        constexpr bool matches[] = { std::same_as<T, std::tuple_element_t<Is, Tuple>>... };

        for (std::size_t i = 0; i < sizeof...(Is); ++i)
            if (matches[i])
                return i;

        throw std::out_of_range("Type not found in tuple");
    }
    template<typename T, typename Tuple>
    constexpr std::size_t tuple_index_v = tuple_index_impl<T, Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});


    template <typename Map, typename Key, typename Default>
    auto map_get(Map &&m, Key &&key, Default &&default_) {
        using result_type = std::common_reference_t<typename std::remove_cvref_t<Map>::mapped_type, Default &&>;

        auto it = m.find(std::forward<Key>(key));
        if (it == m.end())
            return result_type(std::forward<Default>(default_));
        return result_type(it->second);
    }
}