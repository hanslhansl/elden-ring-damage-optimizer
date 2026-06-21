module;
// #include <meta>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>
export module erdo:calculator;
import :meta;
import :witchy;

import std;
import BS.thread_pool;
// import nlohmann.json;

using json = nlohmann::json;

template <typename Map, typename Key, typename Default>
auto map_get(Map &&m, Key &&key, Default &&default_)
{
    using result_type = std::common_reference_t<typename std::remove_cvref_t<Map>::mapped_type, Default &&>;

    auto it = m.find(std::forward<Key>(key));
    if (it == m.end())
        return result_type(std::forward<Default>(default_));
    return result_type(it->second);
}

template <typename T>
std::pair<std::invoke_result_t<T &&>, std::chrono::nanoseconds> TimeFunctionExecution(T &&func)
{
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

    auto &&result = func();

    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

    // Getting number of milliseconds as a double
    return {std::forward<decltype(result)>(result), t2 - t1};
}

template <>
struct std::formatter<nlohmann::json, char> : std::formatter<std::string_view, char>
{
    auto format(const nlohmann::json &j, std::format_context &ctx) const
    {
        auto s = j.dump();
        return std::formatter<std::string_view, char>::format(s, ctx);
    }
};

export template <typename T>
constexpr T assert_floating_is(double f)
{
    if (f != (T)f)
        throw std::runtime_error("floating is not T");
    return f;
}

namespace calculator
{
    struct Weapon;
    using UpgradeLevels = std::array<int, 3>; // free handed, normal, somber

    
    constexpr bool isVanilla = true;

    enum class Attribute
    {
        STRENGTH,
        DEXTERITY,
        INTELLIGENCE,
        FAITH,
        ARCAINE
    };

    enum class Class
    {
        HERO,
        BANDIT,
        ASTROLOGER,
        WARRIOR,
        PRISONER,
        CONFESSOR,
        WRETCH,
        VAGABOND,
        PROPHET,
        SAMURAI,
    };

    enum class AttackPowerType
    {
        PHYSICAL = 0,
        MAGIC = 1,
        FIRE = 2,
        LIGHTNING = 3,
        HOLY = 4,
        POISON = 5,
        SCARLET_ROT = 6,
        BLEED = 7,
        FROST = 8,
        SLEEP = 9,
        MADNESS = 10,
        DEATH_BLIGHT = 11
    };

    enum class DamageType
    {
        PHYSICAL = std::to_underlying(AttackPowerType::PHYSICAL),
        MAGIC = std::to_underlying(AttackPowerType::MAGIC),
        FIRE = std::to_underlying(AttackPowerType::FIRE),
        LIGHTNING = std::to_underlying(AttackPowerType::LIGHTNING),
        HOLY = std::to_underlying(AttackPowerType::HOLY)
    };

    enum class StatusType
    {
        POISON = std::to_underlying(AttackPowerType::POISON),
        SCARLET_ROT = std::to_underlying(AttackPowerType::SCARLET_ROT),
        BLEED = std::to_underlying(AttackPowerType::BLEED),
        FROST = std::to_underlying(AttackPowerType::FROST),
        SLEEP = std::to_underlying(AttackPowerType::SLEEP),
        MADNESS = std::to_underlying(AttackPowerType::MADNESS),
        DEATH_BLIGHT = std::to_underlying(AttackPowerType::DEATH_BLIGHT)
    };

    enum class Affinity_
    {
        STANDARD = 0,
        HEAVY = 1,
        KEEN = 2,
        QUALITY = 3,
        FIRE = 4,
        FLAME_ART = 5,
        LIGHTNING = 6,
        SACRED = 7,
        MAGIC = 8,
        COLD = 9,
        POISON = 10,
        BLOOD = 11,
        OCCULT = 12,
        UNIQUE = -1
    };

    enum class Type_
    {
        DAGGER = 1,
        STRAIGHT_SWORD = 3,
        GREATSWORD = 5,
        COLOSSAL_SWORD = 7,
        CURVED_SWORD = 9,
        CURVED_GREATSWORD = 11,
        KATANA = 13,
        TWINBLADE = 14,
        THRUSTING_SWORD = 15,
        HEAVY_THRUSTING_SWORD = 16,
        AXE = 17,
        GREATAXE = 19,
        HAMMER = 21,
        GREAT_HAMMER = 23,
        FLAIL = 24,
        SPEAR = 25,
        GREAT_SPEAR = 28,
        HALBERD = 29,
        REAPER = 31,
        FIST = 35,
        CLAW = 37,
        WHIP = 39,
        COLOSSAL_WEAPON = 41,
        LIGHT_BOW = 50,
        BOW = 51,
        GREATBOW = 53,
        CROSSBOW = 55,
        BALLISTA = 56,
        GLINTSTONE_STAFF = 57,
        DUAL_CATALYST = 59,
        SACRED_SEAL = 61,
        SMALL_SHIELD = 65,
        MEDIUM_SHIELD = 67,
        GREATSHIELD = 69,
        TORCH = 87,
        HAND_TO_HAND = 88,
        PERFUME_BOTTLE = 89,
        THRUSTING_SHIELD = 90,
        THROWING_BLADE = 91,
        BACKHAND_BLADE = 92,
        LIGHT_GREATSWORD = 93,
        GREAT_KATANA = 94,
        BEAST_CLAW = 95
    };

} // namespace calculator

template<>
constexpr std::array<std::pair<calculator::Attribute, std::string_view>, 5> enum_string_mapping<calculator::Attribute> = {
    std::pair{calculator::Attribute::STRENGTH, "STRENGTH"},
    std::pair{calculator::Attribute::DEXTERITY, "DEXTERITY"},
    std::pair{calculator::Attribute::INTELLIGENCE, "INTELLIGENCE"},
    std::pair{calculator::Attribute::FAITH, "FAITH"},
    std::pair{calculator::Attribute::ARCAINE, "ARCAINE"}
};
template<>
constexpr std::array<std::pair<calculator::AttackPowerType, std::string_view>, 12> enum_string_mapping<calculator::AttackPowerType> = {
    std::pair{calculator::AttackPowerType::PHYSICAL, "PHYSICAL"},
    std::pair{calculator::AttackPowerType::MAGIC, "MAGIC"},
    std::pair{calculator::AttackPowerType::FIRE, "FIRE"},
    std::pair{calculator::AttackPowerType::LIGHTNING, "LIGHTNING"},
    std::pair{calculator::AttackPowerType::HOLY, "HOLY"},
    std::pair{calculator::AttackPowerType::POISON, "POISON"},
    std::pair{calculator::AttackPowerType::SCARLET_ROT, "SCARLET_ROT"},
    std::pair{calculator::AttackPowerType::BLEED, "BLEED"},
    std::pair{calculator::AttackPowerType::FROST, "FROST"},
    std::pair{calculator::AttackPowerType::SLEEP, "SLEEP"},
    std::pair{calculator::AttackPowerType::MADNESS, "MADNESS"},
    std::pair{calculator::AttackPowerType::DEATH_BLIGHT, "DEATH_BLIGHT"}
};
template<>
constexpr std::array<std::pair<calculator::DamageType, std::string_view>, 5> enum_string_mapping<calculator::DamageType> = {
    std::pair{calculator::DamageType::PHYSICAL, "PHYSICAL"},
    std::pair{calculator::DamageType::MAGIC, "MAGIC"},
    std::pair{calculator::DamageType::FIRE, "FIRE"},
    std::pair{calculator::DamageType::LIGHTNING, "LIGHTNING"},
    std::pair{calculator::DamageType::HOLY, "HOLY"}
};
template<>
constexpr std::array<std::pair<calculator::StatusType, std::string_view>, 7> enum_string_mapping<calculator::StatusType> = {
    std::pair{calculator::StatusType::POISON, "POISON"},
    std::pair{calculator::StatusType::SCARLET_ROT, "SCARLET_ROT"},
    std::pair{calculator::StatusType::BLEED, "BLEED"},
    std::pair{calculator::StatusType::FROST, "FROST"},
    std::pair{calculator::StatusType::SLEEP, "SLEEP"},
    std::pair{calculator::StatusType::MADNESS, "MADNESS"},
    std::pair{calculator::StatusType::DEATH_BLIGHT, "DEATH_BLIGHT"}
};
template<>
constexpr std::array<std::pair<calculator::Class, std::string_view>, 10> enum_string_mapping<calculator::Class> = {
    std::pair{calculator::Class::HERO, "HERO"},
    std::pair{calculator::Class::BANDIT, "BANDIT"},
    std::pair{calculator::Class::ASTROLOGER, "ASTROLOGER"},
    std::pair{calculator::Class::WARRIOR, "WARRIOR"},
    std::pair{calculator::Class::PRISONER, "PRISONER"},
    std::pair{calculator::Class::CONFESSOR, "CONFESSOR"},
    std::pair{calculator::Class::WRETCH, "WRETCH"},
    std::pair{calculator::Class::VAGABOND, "VAGABOND"},
    std::pair{calculator::Class::PROPHET, "PROPHET"},
    std::pair{calculator::Class::SAMURAI, "SAMURAI"}
};
template<>
constexpr std::array<std::pair<calculator::Affinity_, std::string_view>, 14> enum_string_mapping<calculator::Affinity_> = {
    std::pair{calculator::Affinity_::STANDARD, "STANDARD"},
    std::pair{calculator::Affinity_::HEAVY, "HEAVY"},
    std::pair{calculator::Affinity_::KEEN, "KEEN"},
    std::pair{calculator::Affinity_::QUALITY, "QUALITY"},
    std::pair{calculator::Affinity_::FIRE, "FIRE"},
    std::pair{calculator::Affinity_::FLAME_ART, "FLAME_ART"},
    std::pair{calculator::Affinity_::LIGHTNING, "LIGHTNING"},
    std::pair{calculator::Affinity_::SACRED, "SACRED"},
    std::pair{calculator::Affinity_::MAGIC, "MAGIC"},
    std::pair{calculator::Affinity_::COLD, "COLD"},
    std::pair{calculator::Affinity_::POISON, "POISON"},
    std::pair{calculator::Affinity_::BLOOD, "BLOOD"},
    std::pair{calculator::Affinity_::OCCULT, "OCCULT"},
    std::pair{calculator::Affinity_::UNIQUE, "UNIQUE"}
};
template<>
constexpr std::array<std::pair<calculator::Type_, std::string_view>, 47> enum_string_mapping<calculator::Type_> = {
    std::pair{calculator::Type_::DAGGER, "DAGGER"},
    std::pair{calculator::Type_::STRAIGHT_SWORD, "STRAIGHT_SWORD"},
    std::pair{calculator::Type_::GREATSWORD, "GREATSWORD"},
    std::pair{calculator::Type_::COLOSSAL_SWORD, "COLOSSAL_SWORD"},
    std::pair{calculator::Type_::CURVED_SWORD, "CURVED_SWORD"},
    std::pair{calculator::Type_::CURVED_GREATSWORD, "CURVED_GREATSWORD"},
    std::pair{calculator::Type_::KATANA, "KATANA"},
    std::pair{calculator::Type_::TWINBLADE, "TWINBLADE"},
    std::pair{calculator::Type_::THRUSTING_SWORD, "THRUSTING_SWORD"},
    std::pair{calculator::Type_::HEAVY_THRUSTING_SWORD, "HEAVY_THRUSTING_SWORD"},
    std::pair{calculator::Type_::AXE, "AXE"},
    std::pair{calculator::Type_::GREATAXE, "GREATAXE"},
    std::pair{calculator::Type_::HAMMER, "HAMMER"},
    std::pair{calculator::Type_::GREAT_HAMMER, "GREAT_HAMMER"},
    std::pair{calculator::Type_::FLAIL, "FLAIL"},
    std::pair{calculator::Type_::SPEAR, "SPEAR"},
    std::pair{calculator::Type_::GREAT_SPEAR, "GREAT_SPEAR"},
    std::pair{calculator::Type_::HALBERD, "HALBERD"},
    std::pair{calculator::Type_::REAPER, "REAPER"},
    std::pair{calculator::Type_::FIST, "FIST"},
    std::pair{calculator::Type_::CLAW, "CLAW"},
    std::pair{calculator::Type_::WHIP, "WHIP"},
    std::pair{calculator::Type_::COLOSSAL_WEAPON, "COLOSSAL_WEAPON"},
    std::pair{calculator::Type_::LIGHT_BOW, "LIGHT_BOW"},
    std::pair{calculator::Type_::BOW, "BOW"},
    std::pair{calculator::Type_::GREATBOW, "GREATBOW"},
    std::pair{calculator::Type_::CROSSBOW, "CROSSBOW"},
    std::pair{calculator::Type_::BALLISTA, "BALLISTA"},
    std::pair{calculator::Type_::GLINTSTONE_STAFF, "GLINTSTONE_STAFF"},
    std::pair{calculator::Type_::DUAL_CATALYST, "DUAL_CATALYST"},
    std::pair{calculator::Type_::SACRED_SEAL, "SACRED_SEAL"},
    std::pair{calculator::Type_::SMALL_SHIELD, "SMALL_SHIELD"},
    std::pair{calculator::Type_::MEDIUM_SHIELD, "MEDIUM_SHIELD"},
    std::pair{calculator::Type_::GREATSHIELD, "GREATSHIELD"},
    std::pair{calculator::Type_::TORCH, "TORCH"},
    std::pair{calculator::Type_::HAND_TO_HAND, "HAND_TO_HAND"},
    std::pair{calculator::Type_::PERFUME_BOTTLE, "PERFUME_BOTTLE"},
    std::pair{calculator::Type_::THRUSTING_SHIELD, "THRUSTING_SHIELD"},
    std::pair{calculator::Type_::THROWING_BLADE, "THROWING_BLADE"},
    std::pair{calculator::Type_::BACKHAND_BLADE, "BACKHAND_BLADE"},
    std::pair{calculator::Type_::LIGHT_GREATSWORD, "LIGHT_GREATSWORD"},
    std::pair{calculator::Type_::GREAT_KATANA, "GREAT_KATANA"},
    std::pair{calculator::Type_::BEAST_CLAW, "BEAST_CLAW"}
};

namespace calculator
{
    constexpr std::string_view attribute_to_json_string(Attribute at)
    {
        return enum_to_string(at);
        // switch (at)
        // {
        // case Attribute::STRENGTH:
        //     return "str";
        // case Attribute::DEXTERITY:
        //     return "dex";
        // case Attribute::INTELLIGENCE:
        //     return "int";
        // case Attribute::FAITH:
        //     return "fai";
        // case Attribute::ARCAINE:
        //     return "arc";
        // }

        // throw std::invalid_argument("invalid attribute");
    }

    using Stats = std::array<int, enumerators_of<Attribute>().size()>;
    using FullStats = std::array<int, 8>;
    constexpr Stats full_stats_to_stats(const FullStats &full_stats)
    {
        return {full_stats.at(3), full_stats.at(4), full_stats.at(5), full_stats.at(6), full_stats.at(7)};
    }
    constexpr FullStats merge_stats_and_full_stats(const Stats &stats, const FullStats &full_stats)
    {
        return {full_stats.at(0), full_stats.at(1), full_stats.at(2), stats.at(0), stats.at(1), stats.at(2), stats.at(3), stats.at(4)};
    }
    const std::map<Class, Stats> ALL_CLASS_STATS{{Class::HERO, {16, 9, 7, 8, 11}}, {Class::BANDIT, {9, 13, 9, 8, 14}}, {Class::ASTROLOGER, {8, 12, 16, 7, 9}}, {Class::WARRIOR, {10, 16, 10, 8, 9}}, {Class::PRISONER, {11, 14, 14, 6, 9}}, {Class::CONFESSOR, {12, 12, 9, 14, 9}}, {Class::WRETCH, {10, 10, 10, 10, 10}}, {Class::VAGABOND, {14, 13, 9, 9, 7}}, {Class::PROPHET, {11, 10, 7, 16, 10}}, {Class::SAMURAI, {12, 15, 9, 8, 8}}};

    using ScalingCurve = std::array<double, 149>;
    using AttributeScaling = std::array<double, enumerators_of<Attribute>().size()>;
    using AttackElementCorrects = std::array<AttributeScaling, enumerators_of<AttackPowerType>().size()>;
    using AttackElementCorrectsById = std::map<int, AttackElementCorrects>;
}


namespace nlohmann
{
    template <>
    struct adl_serializer<calculator::Attribute>
    {
        static calculator::Attribute from_json(const json &j)
        {
            return string_to_enum<calculator::Attribute>(j.get<std::string_view>());
        }
        static void to_json(json &j, calculator::Attribute opt)
        {
            j = enum_to_string(opt);
        }
    };

    template <>
    struct adl_serializer<calculator::Stats>
    {
        static void from_json(const json &j, calculator::Stats &s)
        {
            for (const auto &[attr_str, val] : j.items())
            {
                auto attr = string_to_enum<calculator::Attribute>(attr_str);
                s[std::to_underlying(attr)] = val.get<int>();
            }
        }
        static void to_json(json &j, const calculator::Stats &s)
        {
            j = json::object();
            for (const auto &[attr_index, val] : std::views::enumerate(s))
            {
                if (val != 0)
                {
                    auto attr_str = enum_to_string(integral_to_enum<calculator::Attribute>(attr_index));
                    j[attr_str] = val;
                }
            }
        }
    };

    template <>
    struct adl_serializer<calculator::AttackPowerType>
    {
        static calculator::AttackPowerType from_json(const json &j)
        {
            return integral_to_enum<calculator::AttackPowerType>(j.get<int>());
        }
        static void to_json(json &j, calculator::AttackPowerType opt)
        {
            j = std::to_underlying(opt);
        }
    };

    template <typename T>
    struct adl_serializer<std::map<calculator::AttackPowerType, T>>
    {
        static void from_json(const json &j, std::map<calculator::AttackPowerType, T> &m)
        {
            for (const auto &[apt_str, val] : j.items())
            {
                auto apt = integral_to_enum<calculator::AttackPowerType>(std::stoi(apt_str));

                m[apt] = val.get<T>();
            }
        }
        static void to_json(json &j, const std::map<calculator::AttackPowerType, T> &m)
        {
            j = json::object();
            for (const auto &[apt, val] : m)
                j[std::to_string((std::to_underlying(apt)))] = val;
        }
    };

    template <>
    struct adl_serializer<calculator::AttributeScaling>
    {
        static void from_json(const json &j, calculator::AttributeScaling &ac)
        {
            for (auto &&[attr_str, val] : j.items())
            {
                auto attr = string_to_enum<calculator::Attribute>(attr_str);

                if (val.is_boolean())
                    ac[std::to_underlying(attr)] = val.get<bool>();
                else if (val.is_number())
                    ac[std::to_underlying(attr)] = val.get<double>();
                else
                    throw std::invalid_argument(std::string("invalid json type") + val.type_name());
            }
        }
    };

    template <>
    struct adl_serializer<calculator::AttackElementCorrects>
    {
        static void from_json(const json &j, calculator::AttackElementCorrects &aec)
        {
            for (auto &&[apt_str, ac_j] : j.items())
            {
                auto apt_int = std::stoi(apt_str);
                auto apt = integral_to_enum<calculator::AttackPowerType>(apt_int);

                aec[apt_int] = ac_j.get<calculator::AttributeScaling>();
            }
        }
    };

    template <>
    struct adl_serializer<calculator::Affinity_>
    {
        static calculator::Affinity_ from_json(const json &j)
        {
            return integral_to_enum<calculator::Affinity_>(j.get<int>());
        }
    };

    template <>
    struct adl_serializer<calculator::Type_>
    {
        static calculator::Type_ from_json(const json &j)
        {
            return integral_to_enum<calculator::Type_>(j.get<int>());
        }
    };
} // namespace nlohmann

namespace calculator
{

    constexpr auto ineffective_attribute_penalty = 0.4;
    constexpr auto defaultDamageCalcCorrectGraphId = 0;
    constexpr auto defaultStatusCalcCorrectGraphId = 6;

    struct AttackOptions
    {
        UpgradeLevels upgrade_levels; // free handed, normal, somber
        bool two_handing;
        static const bool disable_two_handing_attack_power_bonus = false;
    };

    namespace AttackRating
    {
        namespace detail
        {
            struct base
            {
                static constexpr bool is_total_attack_power = false;
                static constexpr bool is_individual_attack_power = false;
                static constexpr bool is_individual_status_effect = false;
                static constexpr bool is_spell_scaling = false;
                static constexpr bool is_full = false;

                Stats stats{};
            };

            struct total_attack_power : base
            {
                static constexpr bool is_total_attack_power = true;

                double total_attack_power;
                constexpr auto value() const
                {
                    return total_attack_power;
                }
            };
            template <AttackPowerType I>
            struct individual_attack_power : base
            {
                static constexpr bool is_individual_attack_power = true;
                static constexpr AttackPowerType attack_power_type = I;

                double individual_attack_power;
                constexpr auto value() const
                {
                    return individual_attack_power;
                }
            };
            template <AttackPowerType I>
            struct individual_status_effect : base
            {
                static constexpr bool is_individual_status_effect = true;
                static constexpr AttackPowerType attack_power_type = I;

                double individual_status_effect;
                constexpr auto value() const
                {
                    return individual_status_effect;
                }
            };
            struct spell_scaling : base
            {
                static constexpr bool is_spell_scaling = true;

                double spell_scaling;
                constexpr auto value() const
                {
                    return spell_scaling;
                }
            };
            struct full : base
            {
                static constexpr bool is_full = true;

                const Weapon *weapon;
                UpgradeLevels upgrade_levels;
                bool two_handing;

                std::array<double, 3> total_attack_power;                                                        // a + b = c
                std::array<std::array<double, 3>, enumerators_of<DamageType>().size()> attack_power;  // a + b = c
                std::array<std::array<double, 3>, enumerators_of<StatusType>().size()> status_effect; // a + b = c
                double spell_scaling;
                std::vector<AttackPowerType> ineffective_attack_power_types;
                std::vector<Attribute> ineffective_attributes;
            };

            template <typename B>
            struct sparse_attack_rating : B
            {
            };
        } // namespace detail

        using total = detail::sparse_attack_rating<detail::total_attack_power>;

        using physical = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::PHYSICAL>>;
        using magic = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::MAGIC>>;
        using fire = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::FIRE>>;
        using lightning = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::LIGHTNING>>;
        using holy = detail::sparse_attack_rating<detail::individual_attack_power<AttackPowerType::HOLY>>;

        using poison_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::POISON>>;
        using scarlet_rot_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::SCARLET_ROT>>;
        using bleed_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::BLEED>>;
        using frost_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::FROST>>;
        using sleep_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::SLEEP>>;
        using madness_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::MADNESS>>;
        using death_blight_status = detail::sparse_attack_rating<detail::individual_status_effect<AttackPowerType::DEATH_BLIGHT>>;

        using spell_scaling = detail::sparse_attack_rating<detail::spell_scaling>;

        using full = detail::sparse_attack_rating<detail::full>;
    } // namespace AttackRating

    struct Weapon
    {
      private:
        inline static thread_local std::vector<Attribute> get_attack_rating_ineffective_attributes = []() {
            std::vector<Attribute> v{};
            constexpr auto size = enumerators_of<Attribute>().size();
            v.reserve(size);
            return v; }();

      public:
        using Affinity = Affinity_;
        using Type = Type_;

        struct Filter
        {
            std::set<bool> dlc;
            std::set<Type> types;
            std::set<Affinity> affinities;
            std::set<std::string> base_names;

            bool operator()(const Weapon &weapon) const
            {
                auto satisfies = [](const auto &set, const auto &val) { return set.empty() or set.contains(val); };

                return satisfies(this->base_names, weapon.base_name) and satisfies(this->dlc, weapon.dlc) and satisfies(this->types, weapon.type) and satisfies(this->affinities, weapon.affinity);
            }
        };

        struct AllFilterOptions
        {
            std::vector<bool> dlc;
            // std::vector<bool> sorcery_tools;
            // std::vector<bool> incantation_tools;
            std::vector<Type> types;
            std::vector<Affinity> affinities;
            std::vector<std::string> base_names;
        };

        // the full unique name of the weapon, e.g. "Heavy Nightrider Glaive"
        const std::string full_name;
        // the base weapon name without an affinity specified, e.g. "Nightrider
        // Glaive"
        const std::string base_name;
        // a wiki link for the weapon
        const std::string url;
        // true if the weapon was introduced with SOTE
        const bool dlc;
        // true if the weapon doesn't get a strength bonus when two-handing
        const bool paired;
        // true if this weapon can cast glintstone sorceries
        const bool sorcery_tool;
        // true if this weapon can cast incantations
        const bool incantation_tool;
        // the category of the weapon, e.g. Type.STRAIGHT_SWORD
        const Type type;
        // the affinity of the weapon, e.g. Affinity.HEAVY
        const Affinity affinity;
        // stat requirements necessary to use the weapon effectively (without an
        // attack rating penalty)
        const Stats requirements;
        // scaling amount at each upgrade level (0-10 or 0-25) for each player
        // attribute (e.g. Attribute.STRENGTH)
        const std::vector<AttributeScaling> attribute_scaling;
        // base attack power at each upgrade level for each attack power type
        const std::vector<std::array<double, enumerators_of<AttackPowerType>().size()>> base_attack_power;
        // map indicating which attack power types scale with which player
        // attributes
        const AttackElementCorrectsById::mapped_type attack_power_attribute_scaling;
        // map indicating which scaling curve is used for each attack power type
        const std::array<ScalingCurve, enumerators_of<AttackPowerType>().size()> attack_power_scaling_curves;
        // thresholds and labels for each scaling grade (S, A, B, etc.) for this
        // weapon. This isn't hardcoded for all weapons because it can be
        // changed by mods.
        const std::array<std::pair<double, std::string>, 6> scaling_tiers;

        // the index of the upgrade level for this weapon
        const int upgrade_level_index = [&]() {
            if (this->base_attack_power.size() == 1)
                return 0;
            else if (this->base_attack_power.size() == 11)
                return 2;
            else if (this->base_attack_power.size() == 26)
                return 1;
            else
                throw std::runtime_error("invalid base attack power size"); }();

        Stats adjust_stats_for_two_handing(bool two_handing, Stats stats) const
        {
            // Paired weapons do not get the two handing bonus
            if (this->paired)
                two_handing = false;

            // Bows and ballistae can only be two handed
            constexpr std::array<Weapon::Type, 4> bow_types = {Weapon::Type::LIGHT_BOW, Weapon::Type::BOW, Weapon::Type::GREATBOW, Weapon::Type::BALLISTA};
            if (std::ranges::contains(bow_types, this->type))
                two_handing = true;

            if (two_handing)
                stats.at(std::to_underlying(Attribute::STRENGTH)) *= 1.5;

            return stats;
        }

        template <typename T>
        void get_attack_rating(const AttackOptions &attack_options_, const Stats &stats, T &result) const
        {
            auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options_.two_handing, stats);

            result.stats = stats;
            if constexpr (T::is_total_attack_power)
                result.total_attack_power = 0.;
            if constexpr (T::is_full)
            {
                result.weapon = this;
                result.upgrade_levels = attack_options_.upgrade_levels;
                result.two_handing = attack_options_.two_handing;

                result.total_attack_power.fill({});
                constexpr auto size = enumerators_of<AttackPowerType>().size();
                result.ineffective_attack_power_types.reserve(size);
            }

            for (auto attribute : enumerators_of<Attribute>())
                if (adjusted_stats[std::to_underlying(attribute)] < this->requirements[std::to_underlying(attribute)])
                    Weapon::get_attack_rating_ineffective_attributes.push_back(attribute);

            auto upgrade_level = attack_options_.upgrade_levels.at(upgrade_level_index);
            auto &base_attack_power_at_upgrade_level = this->base_attack_power.at(upgrade_level);

            bool is_sorcery_or_incantation_tool = this->sorcery_tool || this->incantation_tool;

            auto loop_cycle = [&](const AttackPowerType &attack_power_type) {
                auto temp_index = std::to_underlying(attack_power_type);
                auto base_attack_power = base_attack_power_at_upgrade_level[temp_index];

                if (base_attack_power != 0 || is_sorcery_or_incantation_tool)
                {
                    auto is_damage_type = std::to_underlying(attack_power_type) <= std::to_underlying(AttackPowerType::HOLY);
                    auto &&scaling_attributes =
                        this->attack_power_attribute_scaling.at(std::to_underlying(attack_power_type));
                    double total_scaling = 1.;

                    if (std::ranges::any_of(
                            Weapon::get_attack_rating_ineffective_attributes,
                            [&](Attribute ineffective_attribute)
                            {
                                return scaling_attributes[std::to_underlying(ineffective_attribute)] != 0;
                            }))
                    {
                        total_scaling = 1. - ineffective_attribute_penalty;
                        if constexpr (T::is_full)
                            result.ineffective_attack_power_types.push_back(attack_power_type);
                    }
                    else
                    {
                        auto &effective_stats =
                            (!attack_options_.disable_two_handing_attack_power_bonus &&
                             is_damage_type)
                                ? adjusted_stats
                                : stats;

                        for (auto &&attribute : enumerators_of<Attribute>())
                        {
                            auto &&attribute_correct =
                                scaling_attributes.at(std::to_underlying(attribute));
                            double scaling{};

                            if (attribute_correct != 0)
                            {
                                if (attribute_correct == 1)
                                    scaling = this->attribute_scaling.at(upgrade_level)
                                                  .at(std::to_underlying(attribute));
                                else
                                    scaling =
                                        attribute_correct *
                                        this->attribute_scaling.at(upgrade_level)
                                            .at(std::to_underlying(attribute)) /
                                        this->attribute_scaling.at(0).at(std::to_underlying(attribute));

                                if (scaling != 0.)
                                    total_scaling +=
                                        this->attack_power_scaling_curves[std::to_underlying(attack_power_type)][effective_stats[std::to_underlying(attribute)]] * scaling;
                            }
                        }
                    }

                    if (base_attack_power != 0)
                    {
                        auto res = base_attack_power * total_scaling;

                        if constexpr (T::is_total_attack_power)
                            if (is_damage_type)
                                result.total_attack_power += res;

                        if constexpr (T::is_individual_attack_power)
                            result.individual_attack_power = res;

                        if constexpr (T::is_individual_status_effect)
                            result.individual_status_effect = res;

                        if constexpr (T::is_full)
                        {
                            if (is_damage_type) // attack_power_type._to_integral()
                                                // <= AttackPowerType::HOLY
                            {
                                auto &&att_pwr =
                                    result.attack_power[std::to_underlying(attack_power_type)];
                                att_pwr[0] = base_attack_power;
                                att_pwr[1] = res - base_attack_power;
                                att_pwr[2] = res;
                                result.total_attack_power[0] += base_attack_power;
                                result.total_attack_power[1] += res - base_attack_power;
                                result.total_attack_power[2] += res;
                            }
                            else // attack_power_type._to__integral() >
                                 // AttackPowerType::HOLY
                            {
                                auto &&att_pwr =
                                    result.status_effect[std::to_underlying(attack_power_type) - std::to_underlying(AttackPowerType::POISON)];
                                att_pwr[0] = base_attack_power;
                                att_pwr[1] = res - base_attack_power;
                                att_pwr[2] = res;
                            }
                        }
                    }

                    if constexpr (T::is_spell_scaling)
                        if (is_sorcery_or_incantation_tool)
                            result.spell_scaling = 100. * total_scaling;

                    if constexpr (T::is_full)
                        if (attack_power_type == AttackPowerType::PHYSICAL && is_sorcery_or_incantation_tool)
                            result.spell_scaling = 100. * total_scaling;
                } };

            if constexpr (T::is_total_attack_power)
                for (auto &&attack_power_type : enumerators_of<DamageType>())
                    loop_cycle(integral_to_enum<AttackPowerType>(std::to_underlying(attack_power_type)));

            if constexpr (T::is_individual_attack_power)
                loop_cycle(T::attack_power_type);

            if constexpr (T::is_individual_status_effect)
                loop_cycle(T::attack_power_type);

            if constexpr (T::is_spell_scaling)
                loop_cycle(AttackPowerType::PHYSICAL);

            if constexpr (T::is_full)
                for (auto &&attack_power_type : enumerators_of<AttackPowerType>())
                    loop_cycle(attack_power_type);

            if constexpr (T::is_full)
            {
                result.ineffective_attributes = std::move(Weapon::get_attack_rating_ineffective_attributes);
                constexpr auto size = enumerators_of<Attribute>().size();
                Weapon::get_attack_rating_ineffective_attributes.reserve(size);
            }
            Weapon::get_attack_rating_ineffective_attributes.clear();
        }

        bool operator==(const Weapon&) const = default;
    };

    struct CalcCorrectGraphEntry
    {
        int maxVal;
        double maxGrowVal, adjPt;

        friend void from_json(const json &j, CalcCorrectGraphEntry &c)
        {
            j.at("maxVal").get_to(c.maxVal);
            j.at("maxGrowVal").get_to(c.maxGrowVal);
            j.at("adjPt").get_to(c.adjPt);
        }
        friend void to_json(json &j, const CalcCorrectGraphEntry &c)
        {
            j = json{{"maxVal", c.maxVal}, {"maxGrowVal", c.maxGrowVal}, {"adjPt", c.adjPt}};
        }
    };
    using CalcCorrectGraph = std::array<CalcCorrectGraphEntry, 5>;
    struct ReinforceTypesDict
    {
        AttributeScaling attack;             // index: AttackPowerType (if in ALL_DAMAGE_TYPES)
        AttributeScaling attributeScaling;   // index: Attribute
        std::array<int, 3> statusSpEffectId; // statusSpEffectId1,
                                             // statusSpEffectId2,/*  */
                                             // statusSpEffectId3

        friend void from_json(const json &j, ReinforceTypesDict &r)
        {
            auto &&attack_json = j.at("attack");
            for (auto dmg_type : enumerators_of<DamageType>())
                r.attack.at(std::to_underlying(dmg_type)) = attack_json.value(std::to_string(std::to_underlying(dmg_type)), double(0));

            r.attributeScaling = j.at("attributeScaling").get<AttributeScaling>();

            r.statusSpEffectId.at(0) = j.value("statusSpEffectId1", 0);
            r.statusSpEffectId.at(1) = j.value("statusSpEffectId2", 0);
            r.statusSpEffectId.at(2) = j.value("statusSpEffectId3", 0);
        }

        bool operator==(const ReinforceTypesDict&) const = default;
    };

    constexpr size_t get_stat_variation_count(const int attribute_points, const Stats &min_stats)
    {
        constexpr auto N = Stats{}.size();
        constexpr auto UPPER = 99;
        const auto SUM = attribute_points;
        size_t count = 0;

        if (attribute_points > UPPER * min_stats.size())
            return 0;
        // throw std::invalid_argument("attribute_points must be <= " +
        // std::to_string(UPPER) + " * " + std::to_string(N));

        if (std::ranges::any_of(min_stats, [](int v) { return v > UPPER; }))
            throw std::invalid_argument("min_stats must be "
                                        "<= " + std::to_string(UPPER));

        for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
        {
            auto SUM_i = SUM - i;
            for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
            {
                auto SUM_i_j = SUM_i - j;

                if (0ll == min_stats[4])
                {
                    auto a1 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
                    auto b1 = std::min(UPPER, SUM_i_j - min_stats[3]);
                    auto b1_a1_1 = b1 - a1 + 1;
                    if (b1_a1_1 > 0)
                        count += (1 - min_stats[3]) * b1_a1_1;

                    auto a2 = a1;
                    auto b2 = std::min(UPPER, SUM_i_j - UPPER - 1);
                    auto b2_a2_1 = b2 - a2 + 1;
                    if (b2_a2_1 > 0)
                        count += UPPER * b2_a2_1;

                    auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
                    auto b3 = b1;
                    auto b3_a3_1 = b3 - a3 + 1;
                    if (b3_a3_1 > 0)
                        count += SUM_i_j * b3_a3_1 - (a3 + b3) * b3_a3_1 / 2;

                    auto a4 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
                    auto b4 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
                    auto b4_a4_1 = b4 - a4 + 1;
                    if (b4_a4_1 > 0)
                        count += (UPPER + 1 - SUM_i_j + UPPER) * b4_a4_1 + (a4 + b4) * b4_a4_1 / 2;

                    auto a5 = std::max(min_stats[2], SUM_i_j - UPPER);
                    auto b5 = b4;
                    auto b5_a5_1 = b5 - a5 + 1;
                    if (b5_a5_1 > 0)
                        count += SUM_i_j * b5_a5_1;
                }
                else
                {
                    auto a2 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
                    auto b2 = std::min({UPPER, SUM_i_j - UPPER - min_stats[4], SUM_i_j - UPPER - 1});
                    auto b2_a2_1 = b2 - a2 + 1;
                    if (b2_a2_1 > 0)
                        count += (1 + UPPER - min_stats[3]) * b2_a2_1;

                    auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
                    auto b3 = std::min(UPPER, SUM_i_j - UPPER - min_stats[4]);
                    auto b3_a3_1 = b3 - a3 + 1;
                    if (b3_a3_1 > 0)
                        count += (1 + SUM_i_j - min_stats[3]) * b3_a3_1 - (a3 + b3) * b3_a3_1 / 1;

                    auto a4 = std::max({min_stats[2], SUM_i_j - min_stats[3] - UPPER, SUM_i_j - min_stats[4] - UPPER + 1});
                    auto b4 = std::min(UPPER, SUM_i_j - min_stats[4] - min_stats[3]);
                    auto b4_a4_1 = b4 - a4 + 1;
                    if (b4_a4_1 > 0)
                        count += (SUM_i_j - min_stats[4] - min_stats[3] + 1) * b4_a4_1 - (a4 + b4) * b4_a4_1 / 2;

                    auto a5 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
                    auto b5 = std::min({UPPER, SUM_i_j - UPPER - 1 - min_stats[3], SUM_i_j - min_stats[4] - UPPER});
                    auto b5_a5_1 = b5 - a5 + 1;
                    if (b5_a5_1 > 0)
                        count += (UPPER - SUM_i_j + UPPER + 1) * b5_a5_1 + (a5 + b5) * b5_a5_1 / 2;

                    auto a7 = std::max(min_stats[2], SUM_i_j - UPPER);
                    auto b7 = b5;
                    auto b7_a7_1 = b7 - a7 + 1;
                    if (b7_a7_1 > 0)
                        count += (UPPER + 1) * b7_a7_1;

                    auto a8 = std::max(min_stats[2], SUM_i_j - UPPER - min_stats[4] + 1);
                    auto b8 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
                    auto b8_a8_1 = b8 - a8 + 1;
                    if (b8_a8_1 > 0)
                        count += (UPPER - min_stats[4] + 1) * b8_a8_1;
                }
            }
        }

        return count;
    }
    constexpr std::vector<Stats> get_stat_variations(const int attribute_points, const Stats &min_stats)
    {
        constexpr auto N = Stats{}.size();
        constexpr auto UPPER = 99;
        const auto SUM = attribute_points;

        auto possible_occurances = get_stat_variation_count(attribute_points, min_stats);
        if (possible_occurances == 0)
            return {};
        std::vector<Stats> stat_variations{};
        stat_variations.reserve(possible_occurances);

        for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
        {
            auto SUM_i = SUM - i;
            for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
            {
                auto SUM_i_j = SUM_i - j;
                for (auto k = min_stats[2]; k <= std::min(UPPER, SUM_i_j); ++k)
                {
                    auto SUM_i_j_k = SUM_i_j - k;
                    for (auto l = min_stats[3]; l <= std::min(UPPER, SUM_i_j_k); ++l)
                    {
                        auto SUM_i_j_k_l = SUM_i_j_k - l;
                        auto m = SUM_i_j_k_l;
                        if (min_stats[4] <= m && m <= UPPER)
                        {
                            stat_variations.push_back({i, j, k, l, m});
                        }
                    }
                }
            }
        }

        return stat_variations;
    }

    struct OptimizationContext
    {
        using var_vec = std::variant<std::vector<AttackRating::total>, std::vector<AttackRating::physical>, std::vector<AttackRating::magic>, std::vector<AttackRating::fire>, std::vector<AttackRating::lightning>, std::vector<AttackRating::holy>, std::vector<AttackRating::poison_status>, std::vector<AttackRating::scarlet_rot_status>, std::vector<AttackRating::bleed_status>, std::vector<AttackRating::frost_status>, std::vector<AttackRating::sleep_status>, std::vector<AttackRating::madness_status>, std::vector<AttackRating::death_blight_status>, std::vector<AttackRating::spell_scaling>>;

        var_vec optional_results;
        // BS::thread_pool<> pool;
        const std::vector<const Weapon *> &weapons;
        AttackOptions attack_options;

        template <typename T>
            requires requires(T t) { t.value(); }
        OptimizationContext(int threads, const std::vector<Stats> &stat_variations, const std::vector<const Weapon *> &filtered_weapons_, AttackOptions attack_options_, std::type_identity<T>)
            : optional_results{}, /*pool(threads),*/ weapons(filtered_weapons_), attack_options(attack_options_)
        {
            // create a vector of optional results for each weapon
            auto &optional_results = this->optional_results.emplace<std::vector<T>>();
            optional_results.resize(this->weapons.size());

            // process one weapon
            auto do_weapon = [&](size_t i) {
                auto &weapon = *this->weapons.at(i);

                T intermediate_attack_rating{};
                T &best_attack_rating = optional_results[i];

                // loop through all stat variations and find the one resulting
                // in the best attack rating
                for (auto &&stats : stat_variations)
                {
                    weapon.get_attack_rating(this->attack_options, stats, intermediate_attack_rating);

                    if (best_attack_rating.value() < intermediate_attack_rating.value())
                        best_attack_rating = std::move(intermediate_attack_rating);
                }

                std::println("completed {}: {}", i, weapon.full_name); };

            // loop through all weapons and get the best attack rating each
            // asynchronously
            for (size_t i = 0; i < this->weapons.size(); i++)
                do_weapon(i);
            // this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);
        }

        AttackRating::full wait_and_get_result()
        {
            // wait for all threads to finish
            // this->pool.wait();

            std::println();

            // loop through all optional attack ratings and get the best one
            auto loop_lambda = [](auto &vec) {
                auto sparse_result_it = std::ranges::max_element(
                    vec, {}, &std::remove_reference_t<decltype(vec)>::value_type::value);
                if (sparse_result_it == vec.end())
                    return std::pair{-1ll, Stats{}};
                auto index = std::distance(vec.begin(), sparse_result_it);
                return std::pair{index, sparse_result_it->stats}; };

            auto [index, stats] = std::visit(loop_lambda, this->optional_results);

            if (index == -1)
                return AttackRating::full{};

            AttackRating::full best_attack_rating{};
            this->weapons[index]->get_attack_rating(this->attack_options, stats, best_attack_rating);
            return best_attack_rating;
        }
    };

    using FilteredWeapons = std::vector<const Weapon *>;

    class WeaponContainer
    {
    protected:
        using WeaponDict = json;

        std::map<int, ScalingCurve> calcCorrectGraphsById{};
        AttackElementCorrectsById attackElementCorrectsById{};
        std::array<std::pair<double, std::string>, 6> scalingTiers{};

        static ScalingCurve evaluate_CalcCorrectGraph(const CalcCorrectGraph &calcCorrectGraph)
        {
            ScalingCurve arr{};

            for (size_t i = 1; i < calcCorrectGraph.size(); i++)
            {
                auto &prevStage = calcCorrectGraph.at(i - 1);
                auto &stage = calcCorrectGraph.at(i);

                auto minAttributeValue = i == 1 ? 1 : prevStage.maxVal + 1;
                auto maxAttributeValue = (i == calcCorrectGraph.size() - 1) ? 148 : stage.maxVal;

                auto attributeValue = minAttributeValue;
                while (attributeValue <= maxAttributeValue)
                {
                    if (not arr.at(attributeValue))
                    {
                        auto ratio = double(attributeValue - prevStage.maxVal) / double(stage.maxVal - prevStage.maxVal);

                        if (prevStage.adjPt > 0)
                            ratio = std::pow(ratio, prevStage.adjPt);
                        else if (prevStage.adjPt < 0)
                            ratio = 1 - std::pow((1 - ratio), -prevStage.adjPt);

                        arr[attributeValue] = prevStage.maxGrowVal + (stage.maxGrowVal - prevStage.maxGrowVal) * ratio;
                    }
                    attributeValue += 1;
                }
            }

            return arr;
        }

      public:
        std::vector<Weapon> weapons{};

        WeaponContainer() = default;
        WeaponContainer(const std::filesystem::path &file_path)
        {
            json data = json::parse(std::ifstream(file_path));

            for (const auto &[id_, calcCorrectGraph] : data.at("calcCorrectGraphs").items())
                this->calcCorrectGraphsById.emplace(std::stoi(id_), evaluate_CalcCorrectGraph(calcCorrectGraph.get<CalcCorrectGraph>()));

            for (auto &&[id, attackElementCorrect] : data.at("attackElementCorrects").items())
            {
                auto &&[inserted, success] = this->attackElementCorrectsById.emplace(std::stoi(id), attackElementCorrect);
                constexpr auto default_ = AttributeScaling{false, false, false, false, true}; // default value
                inserted->second[std::to_underlying(AttackPowerType::POISON)] = default_;
                inserted->second[std::to_underlying(AttackPowerType::BLEED)] = default_;
                inserted->second[std::to_underlying(AttackPowerType::MADNESS)] = default_;
                inserted->second[std::to_underlying(AttackPowerType::SLEEP)] = default_;
            }

            std::map<int, std::vector<ReinforceTypesDict>> reinforceTypes{};
            for (auto &&[id, reinforceType] : data.at("reinforceTypes").items())
                reinforceTypes.emplace(std::stoi(id), reinforceType);

            std::map<int, std::map<AttackPowerType, int>> statusSpEffectParams{};
            for (auto &&[key, val] : data.at("statusSpEffectParams").items())
                statusSpEffectParams.try_emplace(std::stoi(key), val.get<std::map<AttackPowerType, int>>());

            this->scalingTiers = data.at("scalingTiers").get<decltype(this->scalingTiers)>();

            auto create_weapon = [&](const json &weapon_data) {
                auto &&attackElementCorrect = this->attackElementCorrectsById.at(weapon_data.at("attackElementCorrectId").get<int>());

                const auto &reinforceParams = reinforceTypes.at(weapon_data.at("reinforceTypeId").get<int>());

                auto calcCorrectGraphIds = weapon_data.at("calcCorrectGraphIds").get<std::map<AttackPowerType, int>>();
                std::array<ScalingCurve, enumerators_of<AttackPowerType>().size()> weaponCalcCorrectGraphs{};
                for (auto damage_type : enumerators_of<DamageType>())
                    weaponCalcCorrectGraphs.at(std::to_underlying(damage_type)) = this->calcCorrectGraphsById.at(map_get(calcCorrectGraphIds,
                        integral_to_enum<AttackPowerType>(std::to_underlying(damage_type)),
                        defaultDamageCalcCorrectGraphId)
                    );
                for (auto status_type : enumerators_of<StatusType>())
                    weaponCalcCorrectGraphs.at(std::to_underlying(status_type)) = this->calcCorrectGraphsById.at(map_get(calcCorrectGraphIds,
                        integral_to_enum<AttackPowerType>(std::to_underlying(status_type)),
                        defaultStatusCalcCorrectGraphId)
                    );

                auto unupgradedAttack = weapon_data.at("attack").get<std::vector<std::pair<AttackPowerType, int>>>();
                auto statusSpEffectParamIds = weapon_data.value("statusSpEffectParamIds", std::array<int, 3>{});
                std::vector<std::array<double, enumerators_of<AttackPowerType>().size()>> attack{};
                for (const auto &reinforceParam : reinforceParams)
                {
                    auto &attack_at_upgrade_level = attack.emplace_back();
                    for (const auto &[attackPowerType, unupgradedAttackPower] : unupgradedAttack)
                        attack_at_upgrade_level.at(std::to_underlying(attackPowerType)) = unupgradedAttackPower * reinforceParam.attack.at(std::to_underlying(attackPowerType));

                    int i = 0;
                    for (const auto &spEffectParamId : statusSpEffectParamIds)
                    {
                        if (spEffectParamId)
                        {
                            const auto &statusSpEffectParam = statusSpEffectParams.at(spEffectParamId + reinforceParam.statusSpEffectId.at(i));
                            for (const auto &[apt, val] : statusSpEffectParam)
                                attack_at_upgrade_level.at(std::to_underlying(apt)) = val;
                        }
                        i++;
                    }
                }

                auto unupgradedAttributeScaling = weapon_data.at("attributeScaling").get<std::vector<std::pair<Attribute, double>>>();
                std::vector<AttributeScaling> attributeScaling{};
                for (const auto &reinforceParam : reinforceParams)
                {
                    auto &foo = attributeScaling.emplace_back();
                    for (const auto &[attribute, unupgradedScaling] : unupgradedAttributeScaling)
                        foo.at(std::to_underlying(attribute)) =
                            unupgradedScaling *
                            reinforceParam.attributeScaling.at(std::to_underlying(attribute));
                }

                std::string weaponName = weapon_data.at("weaponName").get<std::string>();
                auto url_part = weaponName;
                std::ranges::replace(url_part, ' ', '_');

                return Weapon{
                    .full_name = weapon_data.at("name").get<std::string>(),
                    .base_name = std::move(weaponName),
                    .url = weapon_data.value("url", "https://eldenring.fandom.com/wiki/" + url_part),
                    .dlc = weapon_data.at("dlc").get<bool>(),
                    .paired = weapon_data.value("paired", false),
                    .sorcery_tool = weapon_data.value("sorceryTool", false),
                    .incantation_tool = weapon_data.value("incantationTool", false),
                    .type = weapon_data.at("weaponType").get<Weapon::Type>(),
                    .affinity = weapon_data.at("affinityId").get<Weapon::Affinity>(),
                    .requirements = weapon_data.at("requirements").get<Stats>(),
                    .attribute_scaling = std::move(attributeScaling),
                    .base_attack_power = std::move(attack),
                    .attack_power_attribute_scaling = std::move(attackElementCorrect),
                    .attack_power_scaling_curves = std::move(weaponCalcCorrectGraphs),
                    .scaling_tiers = this->scalingTiers};
                };

            const auto &weapons_data = data.at("weapons");
            this->weapons.reserve(weapons_data.size());
            for (auto &&weapon_data : weapons_data)
            {
                this->weapons.emplace_back(create_weapon(weapon_data));
            }

            std::println("{} weapons\n", this->weapons.size());
        }

        Weapon::AllFilterOptions get_all_filter_options() const
        {
            Weapon::Filter filter{};
            for (const auto &weapon : this->weapons)
            {
                filter.dlc.insert(weapon.dlc);
                filter.types.insert(weapon.type);
                filter.affinities.insert(weapon.affinity);
                filter.base_names.insert(weapon.base_name);
            }

            Weapon::AllFilterOptions all_filter_options{
                {filter.dlc.begin(), filter.dlc.end()},
                {filter.types.begin(), filter.types.end()},
                {filter.affinities.begin(), filter.affinities.end()},
                {filter.base_names.begin(), filter.base_names.end()},
            };

            return all_filter_options;
        }

        FilteredWeapons apply_filter(const Weapon::Filter &weapon_filter) const
        {
            FilteredWeapons filtered{};
            filtered.reserve(this->weapons.size());

            for (const auto &weapon : this->weapons)
                if (weapon_filter(weapon))
                    filtered.push_back(&weapon);

            return filtered;
        }

        bool operator==(const WeaponContainer&) const = default;
    };

    class Parser
    {
        using ParamRow = std::map<std::string, double>;

        // msg/engus/menu.msgbnd.dcx
        inline static const std::vector<std::filesystem::path> needed_elden_ring_file_paths = {"regulation.bin", std::filesystem::path("msg") / "engus" / "menu.msgbnd.dcx", std::filesystem::path("msg") / "engus" / "menu_dlc01.msgbnd.dcx", std::filesystem::path("msg") / "engus" / "menu_dlc02.msgbnd.dcx", std::filesystem::path("msg") / "engus" / "item.msgbnd.dcx", std::filesystem::path("msg") / "engus" / "item_dlc01.msgbnd.dcx", std::filesystem::path("msg") / "engus" / "item_dlc02.msgbnd.dcx"};

        inline static const std::filesystem::path attackElementCorrectFile = "AttackElementCorrectParam.param";
        inline static const std::filesystem::path calcCorrectGraphFile = "CalcCorrectGraph.param";
        inline static const std::filesystem::path equipParamWeaponFile = "EquipParamWeapon.param";
        inline static const std::filesystem::path reinforceParamWeaponFile = "ReinforceParamWeapon.param";
        inline static const std::filesystem::path spEffectFile = "SpEffectParam.param";
        inline static const std::filesystem::path menuValueTableFile = "MenuValueTableParam.param";
        inline static const std::filesystem::path weaponNameFmgFile = "WeaponName.fmg";
        inline static const std::filesystem::path dlcWeaponNameFmgFile = "WeaponName_dlc01.fmg";
        inline static const std::filesystem::path menuTextFmgFile = "GR_MenuText.fmg";

        // AttackElementCorrectParam.param
        inline static const std::set needed_unpacked_files = {attackElementCorrectFile, calcCorrectGraphFile, equipParamWeaponFile, reinforceParamWeaponFile, spEffectFile, menuValueTableFile, weaponNameFmgFile, dlcWeaponNameFmgFile, menuTextFmgFile};

        inline static const std::map<size_t, calculator::Weapon::Type> wepTypeOverrides = {{110000, calculator::Weapon::Type::FIST}};

        static std::vector<std::filesystem::path> copy_elden_ring_files(const std::filesystem::path &elden_ring, const std::filesystem::path &to)
        {
            std::filesystem::create_directory(to);
            std::vector<std::filesystem::path> ret{};

            for (auto &elden_ring_file_path : Parser::needed_elden_ring_file_paths)
            {
                auto elden_ring_file_path_string = elden_ring_file_path.generic_string();
                std::ranges::replace(elden_ring_file_path_string, '/', '-');

                auto new_file = to / elden_ring_file_path_string;
                std::println("copy {} to {}", elden_ring / elden_ring_file_path, new_file);
                std::filesystem::copy_file(elden_ring / elden_ring_file_path, new_file, std::filesystem::copy_options::overwrite_existing);
                ret.push_back(new_file);
            }

            std::println("copy er files to {}\n", to.string());
            return ret;
        }
        static void witchy(const std::filesystem::path &witchy_exe, const std::vector<std::filesystem::path> &files)
        {
            std::stringstream ss{};
            ss << "\"" << witchy_exe << " --passive --parallel";
            for (auto &file : files)
                ss << " " << file;
            ss << "\"";

            std::system(ss.str().c_str());
        }
        static std::vector<std::filesystem::path> witchy_unpack_files(const std::filesystem::path &witchy_exe, const std::vector<std::filesystem::path> &files_to_unpack)
        {
            witchy(witchy_exe, files_to_unpack);

            std::vector<std::filesystem::path> ret{};
            for (auto &unpacked_file : files_to_unpack)
            {
                auto unpacked_filename = unpacked_file.filename().generic_string();
                std::ranges::replace(unpacked_filename, '.', '-');
                ret.push_back(unpacked_file.parent_path() / unpacked_filename);
            }
            return ret;
        }
        static std::vector<std::filesystem::path> witchy_to_xml(const std::filesystem::path &witchy_exe, const std::vector<std::filesystem::path> &files_to_xml)
        {
            witchy(witchy_exe, files_to_xml);

            std::vector<std::filesystem::path> ret{};
            for (auto &file_to_xml : files_to_xml)
            {
                auto xml_filename = file_to_xml.filename().generic_string() + ".xml";
                ret.push_back(file_to_xml.parent_path() / xml_filename);
            }
            return ret;
        }
        static std::vector<std::filesystem::path> get_needed_unpacked_files(const std::vector<std::filesystem::path> &unpacked_directories)
        {
            std::vector<std::filesystem::path> ret{};
            for (auto &unpacked_directory : unpacked_directories)
            {
                for (auto &unpacked_file : std::filesystem::directory_iterator(unpacked_directory))
                {
                    if (Parser::needed_unpacked_files.contains(unpacked_file.path().filename()))
                    {
                        ret.push_back(unpacked_file.path());
                    }
                }
            }

            return ret;
        }
        static std::filesystem::path copy_xml_files(const std::filesystem::path &parent_path, const std::vector<std::filesystem::path> &xml_files)
        {
            auto xml_directory = parent_path / "xml_files";
            std::filesystem::create_directory(xml_directory);

            for (auto &xml_file : xml_files)
            {
                auto new_file_path = xml_directory / xml_file.filename();
                std::filesystem::copy_file(xml_file, new_file_path, std::filesystem::copy_options::overwrite_existing);
            }

            std::println("copy xml files to {}\n", xml_directory.string());
            return xml_directory;
        }
        static std::map<long long, ParamRow> read_param_xml(const std::filesystem::path &file_path)
        {
            auto actual_path = file_path.parent_path() / (file_path.filename().generic_string() + ".xml");

            pugi::xml_document data;
            auto result = data.load_file(actual_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
            if (!result)
                throw std::runtime_error("could not load xml "
                                         "file: " + std::string(result.description()));

            auto field_nodes = data.child("param").child("fields").children("field");

            ParamRow default_values{};
            for (auto &&field_node : field_nodes)
            {
                auto name = field_node.attribute("name").as_string();
                auto defaultValue = field_node.attribute("defaultValue").as_double(std::numeric_limits<double>::max());

                if (defaultValue != std::numeric_limits<double>::max())
                    default_values.emplace(name, defaultValue);
            }

            auto row_nodes = data.child("param").child("rows").children("row");

            std::map<long long, ParamRow> ret{};
            for (auto &&row_node : row_nodes)
            {
                auto name = row_node.attribute("name").as_string();

                ParamRow row_data = default_values;
                for (auto &&row_attribute : row_node.attributes())
                {
                    row_data[row_attribute.name()] = row_attribute.as_double();
                }

                auto id = row_node.attribute("id").as_llong();
                ret.emplace(id, std::move(row_data));
            }

            return ret;
        }
        static std::map<long long, std::string> read_fmg_xml(const std::filesystem::path &file_path)
        {
            auto actual_path = file_path.parent_path() / (file_path.filename().generic_string() + ".xml");

            pugi::xml_document data;
            auto result = data.load_file(actual_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
            if (!result)
                throw std::runtime_error("could not load xml file: " + std::string(result.description()));

            auto text_nodes = data.child("fmg").child("entries").children("text");

            std::map<long long, std::string> ret{};
            for (auto &&text_node : text_nodes)
            {
                auto id = text_node.attribute("id").as_llong();
                auto text = text_node.child_value();
                ret.emplace(id, text);
            }

            return ret;
        }

        static bool is_unique_weapon(const ParamRow &row)
        {
            return row.at("gemMountType") == 0 || row.at("disableGemAttr") == 1;
        }

        std::map<long long, ParamRow> attackElementCorrectParams;
        std::map<long long, ParamRow> calcCorrectGraphs;
        std::map<long long, ParamRow> equipParamWeapons;
        std::map<long long, ParamRow> reinforceParamWeapons;
        std::map<long long, ParamRow> spEffectParams;
        std::map<long long, ParamRow> menuValueTableParams;
        std::map<long long, std::string> menuText;
        std::map<long long, std::string> weaponNames;
        std::map<long long, std::string> dlcWeaponNames;

        std::map<calculator::AttackPowerType, double> parse_status_sp_effect_params(long long statusSpEffectParamId) const
        {
            if (!this->spEffectParams.contains(statusSpEffectParamId))
                return {};
            auto &&spEffectRow = this->spEffectParams.at(statusSpEffectParamId);

            std::map<calculator::AttackPowerType, double> statuses = {{AttackPowerType::POISON, spEffectRow.at("poizonAttackPowe"
                                                                                                                 "r")},
                                                                        {AttackPowerType::SCARLET_ROT, spEffectRow.at("diseaseAttackPowe"
                                                                                                                      "r")},
                                                                        {AttackPowerType::BLEED, spEffectRow.at("bloodAttackPowe"
                                                                                                                "r")},
                                                                        {AttackPowerType::FROST, spEffectRow.at("freezeAttackPowe"
                                                                                                                "r")},
                                                                        {AttackPowerType::SLEEP, spEffectRow.at("sleepAttackPowe"
                                                                                                                "r")},
                                                                        {AttackPowerType::MADNESS, spEffectRow.at("madnessAttackPowe"
                                                                                                                  "r")},
                                                                        {AttackPowerType::DEATH_BLIGHT, spEffectRow.at("curseAttackPower")}};

            if (std::ranges::any_of(statuses, [](auto &&v) { return v.second != 0; }))
                return statuses;

            return {};
        }
        CalcCorrectGraph parse_calc_correct_graph(const ParamRow &row) const
        {
            return CalcCorrectGraph{
                CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal0")), row.at("stageMaxGrowVal0") / 100., row.at("adjPt_maxGrowVal0")},
                CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal1")), row.at("stageMaxGrowVal1") / 100., row.at("adjPt_maxGrowVal1")},
                CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal2")), row.at("stageMaxGrowVal2") / 100., row.at("adjPt_maxGrowVal2")},
                CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal3")), row.at("stageMaxGrowVal3") / 100., row.at("adjPt_maxGrowVal3")},
                CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal4")), row.at("stageMaxGrowVal4") / 100., row.at("adjPt_maxGrowVal4")},
            };
        }
        json parse_attack_element_correct(const ParamRow &row) const
        {
            auto buildAttackElementCorrect = [](std::vector<std::tuple<Attribute, bool, double>> v) {
                json entries = json::object();
                for (auto &&elem : v)
                {
                    auto &&[attribute, isCorrect, overwriteCorrect] = elem;
                    if (isCorrect)
                    {
                        if (overwriteCorrect == -1)
                            entries.emplace(
                                enum_to_string(attribute), true);
                        else
                            entries.emplace(enum_to_string(attribute), overwriteCorrect / 100.);
                    }
                }
                return entries; };

            return json{{std::to_string(std::to_underlying(AttackPowerType::PHYSICAL)), buildAttackElementCorrect({{Attribute::STRENGTH, row.at("isStrengthCorrect_byPhysics"), row.at("overwriteStrengthCorrectRate_byPhysics")}, {Attribute::DEXTERITY, row.at("isDexterityCorrect_byPhysics"), row.at("overwriteDexterityCorrectRate_byPhysics")}, {Attribute::FAITH, row.at("isFaithCorrect_byPhysics"), row.at("overwriteFaithCorrectRate_byPhysics")}, {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byPhysics"), row.at("overwriteMagicCorrectRate_byPhysics")}, {Attribute::ARCAINE, row.at("isLuckCorrect_byPhysics"), row.at("overwriteLuckCorrectRate_byPhysics")}})}, {std::to_string(std::to_underlying(AttackPowerType::MAGIC)), buildAttackElementCorrect({{Attribute::STRENGTH, row.at("isStrengthCorrect_byMagic"), row.at("overwriteStrengthCorrectRate_byMagic")}, {Attribute::DEXTERITY, row.at("isDexterityCorrect_byMagic"), row.at("overwriteDexterityCorrectRate_byMagic")}, {Attribute::FAITH, row.at("isFaithCorrect_byMagic"), row.at("overwriteFaithCorrectRate_byMagic")}, {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byMagic"), row.at("overwriteMagicCorrectRate_byMagic")}, {Attribute::ARCAINE, row.at("isLuckCorrect_byMagic"), row.at("overwriteLuckCorrectRate_byMagic")}})}, {std::to_string(std::to_underlying(AttackPowerType::FIRE)), buildAttackElementCorrect({{Attribute::STRENGTH, row.at("isStrengthCorrect_byFire"), row.at("overwriteStrengthCorrectRate_byFire")}, {Attribute::DEXTERITY, row.at("isDexterityCorrect_byFire"), row.at("overwriteDexterityCorrectRate_byFire")}, {Attribute::FAITH, row.at("isFaithCorrect_byFire"), row.at("overwriteFaithCorrectRate_byFire")}, {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byFire"), row.at("overwriteMagicCorrectRate_byFire")}, {Attribute::ARCAINE, row.at("isLuckCorrect_byFire"), row.at("overwriteLuckCorrectRate_byFire")}})}, {std::to_string(std::to_underlying(AttackPowerType::LIGHTNING)), buildAttackElementCorrect({{Attribute::STRENGTH, row.at("isStrengthCorrect_byThunder"), row.at("overwriteStrengthCorrectRate_byThunder")}, {Attribute::DEXTERITY, row.at("isDexterityCorrect_byThunder"), row.at("overwriteDexterityCorrectRate_byThunder")}, {Attribute::FAITH, row.at("isFaithCorrect_byThunder"), row.at("overwriteFaithCorrectRate_byThunder")}, {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byThunder"), row.at("overwriteMagicCorrectRate_byThunder")}, {Attribute::ARCAINE, row.at("isLuckCorrect_byThunder"), row.at("overwriteLuckCorrectRate_byThunder")}})}, {std::to_string(std::to_underlying(AttackPowerType::HOLY)), buildAttackElementCorrect({{Attribute::STRENGTH, row.at("isStrengthCorrect_byDark"), row.at("overwriteStrengthCorrectRate_byDark")}, {Attribute::DEXTERITY, row.at("isDexterityCorrect_byDark"), row.at("overwriteDexterityCorrectRate_byDark")}, {Attribute::FAITH, row.at("isFaithCorrect_byDark"), row.at("overwriteFaithCorrectRate_byDark")}, {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byDark"), row.at("overwriteMagicCorrectRate_byDark")}, {Attribute::ARCAINE, row.at("isLuckCorrect_byDark"), row.at("overwriteLuckCorrectRate_byDark")}})}};
        }
        json parse_reinforce_param_weapon(const ParamRow &row) const
        {
            auto cut_dec = [](double f) -> json {
                if (f == (long long)f)
                    return (long long)f;
                return f; };

            json ret = {{"attack", {{std::to_string(std::to_underlying(AttackPowerType::PHYSICAL)), cut_dec(row.at("physicsAtkRate"))}, {std::to_string(std::to_underlying(AttackPowerType::MAGIC)), cut_dec(row.at("magicAtkRate"))}, {std::to_string(std::to_underlying(AttackPowerType::FIRE)), cut_dec(row.at("fireAtkRate"))}, {std::to_string(std::to_underlying(AttackPowerType::LIGHTNING)), cut_dec(row.at("thunderAtkRate"))}, {std::to_string(std::to_underlying(AttackPowerType::HOLY)), cut_dec(row.at("darkAtkRate"))}}}, {"attributeScaling", {{calculator::attribute_to_json_string(Attribute::STRENGTH), row.at("correctStrengthRate")}, {calculator::attribute_to_json_string(Attribute::DEXTERITY), row.at("correctAgilityRate")}, {calculator::attribute_to_json_string(Attribute::INTELLIGENCE), row.at("correctMagicRate")}, {calculator::attribute_to_json_string(Attribute::FAITH), row.at("correctFaithRate")}, {calculator::attribute_to_json_string(Attribute::ARCAINE), row.at("correctLuckRate")}}}};

            if (row.contains("spEffectId1"))
                if (row.at("spEffectId1") != 0)
                    ret["statusSpEffectId1"] = row.at("spEffectId1");
            if (row.contains("spEffectId2"))
                if (row.at("spEffectId2") != 0)
                    ret["statusSpEffectId2"] = row.at("spEffectId2");
            if (row.contains("spEffectId3"))
                if (row.at("spEffectId3") != 0)
                    ret["statusSpEffectId3"] = row.at("spEffectId3");

            return ret;
        }

        json parse_weapon(const ParamRow &row) const
        {
            auto row_id = assert_floating_is<long long>(row.at("id"));

            std::string name{};
            bool dlc{};
            if (this->weaponNames.contains(row_id))
                name = this->weaponNames.at(row_id);
            else if (this->dlcWeaponNames.contains(row_id))
            {
                name = this->dlcWeaponNames.at(row_id);
                dlc = isVanilla;
            }
            else
            {
                std::println("ignoring: could not find weapon name for id: {}", row_id);
                return {};
            }

            if (name.find("[ERROR]") != std::string::npos || name.find("%null%") != std::string::npos)
            {
                std::println("ignoring: weapon name: {}, id: {}", name, row_id);
                return {};
            }

            const auto weaponType = wepTypeOverrides.contains(row_id) ? std::to_underlying(wepTypeOverrides.at(row_id)) : assert_floating_is<long long>(row.at("wepType"));
            if (!is_valid_enum_integral<calculator::Weapon::Type>(weaponType))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weaponType))
                    std::println("ignoring: weapon {} because no real weapon", name);
                else
                {
                    std::println("ignoring: Unknown weapon type {} for weapon {}", weaponType, name);
                    throw std::runtime_error("unknown weapon type");
                }
                return {};
            }

            if (!this->reinforceParamWeapons.contains(row.at("reinforceTypeId")))
                throw std::runtime_error("could not find reinforce param weapon for "
                                         "reinforceTypeId: " + std::to_string(row.at("reinforceTypeId")));

            if (!this->attackElementCorrectParams.contains(row.at("attackElementCorrect"
                                                                  "Id")))
                throw std::runtime_error("could not find attack element correct param for "
                                         "attackElementCorrectId: " + std::to_string(row.at("attackElementCorrectId")));

            const auto affinityId = assert_floating_is<long long>((row_id % 10000) / 100.);

            const auto equipParamWeaponsId = row_id - 100 * affinityId;
            if (!this->equipParamWeapons.contains(equipParamWeaponsId))
                throw std::runtime_error("could not find equip param weapon for "
                                         "id: " + std::to_string(equipParamWeaponsId));
            const auto &uninfusedWeapon = this->equipParamWeapons.at(equipParamWeaponsId);

            if (affinityId != 0 && is_unique_weapon(uninfusedWeapon))
                throw std::runtime_error("unique weapon cannot have an affinity");

            std::set<calculator::AttackPowerType> attackPowerTypes{};
            std::vector<long long> statusSpEffectParamIds{};
            for (auto &&spEffectParamId : std::vector<long long>{
                     assert_floating_is<long long>(row.at("spEffectBehaviorId0")),
                     assert_floating_is<long long>(row.at("spEffectBehaviorId1")),
                     assert_floating_is<long long>(row.at("spEffectBehaviorId2")),
                 })
            {
                auto statusSpEffectParams = parse_status_sp_effect_params(spEffectParamId);
                if (!statusSpEffectParams.empty())
                {
                    for (auto &&[k, v] : statusSpEffectParams)
                        attackPowerTypes.insert(k);

                    statusSpEffectParamIds.emplace_back(spEffectParamId);
                }
                else
                    statusSpEffectParamIds.emplace_back(0);
            }

            if (std::ranges::all_of(statusSpEffectParamIds, [](long long id) { return id == 0; }))
                statusSpEffectParamIds.clear();

            if (isVanilla && row_id == 32131200)
                statusSpEffectParamIds = {0, 0, 0};

            std::vector<std::pair<AttackPowerType, double>> attack{};
            for (auto &&pair : std::vector<std::pair<AttackPowerType, double>>{{AttackPowerType::PHYSICAL, row.at("attackBasePhysics")}, {AttackPowerType::MAGIC, row.at("attackBaseMagic")}, {AttackPowerType::FIRE, row.at("attackBaseFire")}, {AttackPowerType::LIGHTNING, row.at("attackBaseThunder")}, {AttackPowerType::HOLY, row.at("attackBaseDark")}})
            {
                auto &&[attackPowerType, attackPower] = pair;

                if (attackPower != 0)
                {
                    attackPowerTypes.insert(attackPowerType);
                    attack.emplace_back(pair);
                }
            }

            if (row.at("enableMagic") || row.at("enableMiracle"))
                for (auto &&damageType : enumerators_of<DamageType>())
                    attackPowerTypes.insert(integral_to_enum<AttackPowerType>(std::to_underlying(damageType)));

            std::map<AttackPowerType, long long> calcCorrectGraphIds{};
            if (attackPowerTypes.contains(AttackPowerType::PHYSICAL))
                if (row.contains("correctType_Physics"))
                    if (row.at("correctType_"
                               "Physics") != defaultDamageCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::PHYSICAL] = assert_floating_is<long long>(row.at("correctType_Physics"));
            if (attackPowerTypes.contains(AttackPowerType::MAGIC))
                if (row.contains("correctType_Magic"))
                    if (row.at("correctType_"
                               "Magic") != defaultDamageCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::MAGIC] = assert_floating_is<long long>(row.at("correctType_Magic"));
            if (attackPowerTypes.contains(AttackPowerType::FIRE))
                if (row.contains("correctType_Fire"))
                    if (row.at("correctType_"
                               "Fire") != defaultDamageCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::FIRE] = assert_floating_is<long long>(row.at("correctType_Fire"));
            if (attackPowerTypes.contains(AttackPowerType::LIGHTNING))
                if (row.contains("correctType_Thunder"))
                    if (row.at("correctType_"
                               "Thunder") != defaultDamageCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::LIGHTNING] = assert_floating_is<long long>(row.at("correctType_Thunder"));
            if (attackPowerTypes.contains(AttackPowerType::HOLY))
                if (row.contains("correctType_Dark"))
                    if (row.at("correctType_"
                               "Dark") != defaultDamageCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::HOLY] = assert_floating_is<long long>(row.at("correctType_Dark"));

            if (attackPowerTypes.contains(AttackPowerType::POISON))
                if (row.contains("correctType_Poison"))
                    if (row.at("correctType_"
                               "Poison") != defaultStatusCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::POISON] = assert_floating_is<long long>(row.at("correctType_Poison"));
            if (attackPowerTypes.contains(AttackPowerType::BLEED))
                if (row.contains("correctType_Bleed"))
                    if (row.at("correctType_"
                               "Bleed") != defaultStatusCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::BLEED] = assert_floating_is<long long>(row.at("correctType_Bleed"));
            if (attackPowerTypes.contains(AttackPowerType::SLEEP))
                if (row.contains("correctType_Sleep"))
                    if (row.at("correctType_"
                               "Sleep") != defaultStatusCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::SLEEP] = assert_floating_is<long long>(row.at("correctType_Sleep"));
            if (attackPowerTypes.contains(AttackPowerType::MADNESS))
                if (row.contains("correctType_Madness"))
                    if (row.at("correctType_"
                               "Madness") != defaultStatusCalcCorrectGraphId)
                        calcCorrectGraphIds[AttackPowerType::MADNESS] = assert_floating_is<long long>(row.at("correctType_Madness"));

            for (auto &&[apt, calcCorrectGraphId] : calcCorrectGraphIds)
            {
                if (is_valid_enum_integral<DamageType>(std::to_underlying(apt)))
                    if (calcCorrectGraphId != defaultDamageCalcCorrectGraphId && !this->calcCorrectGraphs.contains(calcCorrectGraphId))
                        throw std::runtime_error("could not find calc correct graph for "
                                                 "id: " + std::to_string(calcCorrectGraphId));
                if (is_valid_enum_integral<StatusType>(std::to_underlying(apt)))
                    if (calcCorrectGraphId != defaultStatusCalcCorrectGraphId && !this->calcCorrectGraphs.contains(calcCorrectGraphId))
                        throw std::runtime_error("could not find calc correct graph for "
                                                 "id: " + std::to_string(calcCorrectGraphId));
            }

            std::vector<std::pair<Attribute, double>> attributeScaling{};
            if (row.at("correctStrength"))
                attributeScaling.emplace_back(Attribute::STRENGTH, row.at("correctStrengt"
                                                                          "h") / 100.);
            if (row.at("correctAgility"))
                attributeScaling.emplace_back(Attribute::DEXTERITY, row.at("correctAgilit"
                                                                           "y") / 100.);
            if (row.at("correctMagic"))
                attributeScaling.emplace_back(Attribute::INTELLIGENCE, row.at("correctMag"
                                                                              "i"
                                                                              "c") / 100.);
            if (row.at("correctFaith"))
                attributeScaling.emplace_back(Attribute::FAITH, row.at("correctFait"
                                                                       "h") / 100.);
            if (row.at("correctLuck"))
                attributeScaling.emplace_back(Attribute::ARCAINE, row.at("correctLuc"
                                                                         "k") / 100.);

            json ret{};
            ret["name"] = name;
            ret["weaponName"] = (weaponNames.contains(uninfusedWeapon.at("id")) ? weaponNames.at(uninfusedWeapon.at("id")) : dlcWeaponNames.at(uninfusedWeapon.at("id")));
            // ret["url"] = "";
            ret["affinityId"] = is_unique_weapon(row) ? -1 : affinityId;
            ret["weaponType"] = weaponType;
            ret["requirements"] = Stats{
                assert_floating_is<int>(row.at("properStrength")),
                assert_floating_is<int>(row.at("properAgility")),
                assert_floating_is<int>(row.at("properMagic")),
                assert_floating_is<int>(row.at("properFaith")),
                assert_floating_is<int>(row.at("properLuck")),
            };
            ret["attack"] = attack;
            ret["attributeScaling"] = attributeScaling;
            if (!statusSpEffectParamIds.empty())
                ret["statusSpEffectParamIds"] = statusSpEffectParamIds;
            ret["reinforceTypeId"] = assert_floating_is<long long>(row.at("reinforceTyp"
                                                                          "eId"));
            ret["attackElementCorrectId"] = assert_floating_is<long long>(row.at("attac"
                                                                                 "kElem"
                                                                                 "entCo"
                                                                                 "rrect"
                                                                                 "Id"));
            ret["calcCorrectGraphIds"] = calcCorrectGraphIds;
            if (row.at("isDualBlade") == 1)
                ret["paired"] = true;
            if (row.at("enableMagic") == 1)
                ret["sorceryTool"] = true;
            if (row.at("enableMiracle") == 1)
                ret["incantationTool"] = true;
            ret["dlc"] = dlc;

            std::println("weapon: {}, type: {}", name, enum_to_string(integral_to_enum<Weapon::Type>(weaponType)));
            return ret;
        }

      public:
        Parser(const std::filesystem::path &witchy_exe_path, const std::filesystem::path &uxm_target_directory)
        {
            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files
            auto copy_files_to_path = std::filesystem::current_path() / "elden_ring_files";

            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation.bin
            auto files_to_unpack = this->copy_elden_ring_files(uxm_target_directory, copy_files_to_path);

            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin
            auto unpacked_directories = this->witchy_unpack_files(witchy_exe_path, files_to_unpack);

            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin\AttackElementCorrectParam.param
            auto needed_unpacked_file_paths = this->get_needed_unpacked_files(unpacked_directories);

            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin\AttackElementCorrectParam.param.xml
            auto xml_files = this->witchy_to_xml(witchy_exe_path, needed_unpacked_file_paths);

            // D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\xml_files
            auto xml_directory = this->copy_xml_files(copy_files_to_path, xml_files);

            this->attackElementCorrectParams = read_param_xml(xml_directory / attackElementCorrectFile);
            this->calcCorrectGraphs = read_param_xml(xml_directory / calcCorrectGraphFile);
            this->equipParamWeapons = read_param_xml(xml_directory / equipParamWeaponFile);
            this->reinforceParamWeapons = read_param_xml(xml_directory / reinforceParamWeaponFile);
            this->spEffectParams = read_param_xml(xml_directory / spEffectFile);
            this->menuValueTableParams = read_param_xml(xml_directory / menuValueTableFile);
            this->menuText = read_fmg_xml(xml_directory / menuTextFmgFile);
            this->weaponNames = read_fmg_xml(xml_directory / weaponNameFmgFile);
            this->dlcWeaponNames = read_fmg_xml(xml_directory / dlcWeaponNameFmgFile);
        }

        json get_regulation_data_json()
        {
            json weapons_json = json::array();
            for (auto &&[k, param_row] : this->equipParamWeapons)
            {
                auto weapon_json = this->parse_weapon(param_row);
                if (!weapon_json.empty())
                    weapons_json.push_back(weapon_json);
            }

            std::set<long long> calc_correct_graph_ids{defaultDamageCalcCorrectGraphId, defaultStatusCalcCorrectGraphId};
            for (auto &&weapon_json : weapons_json)
                for (auto &&[apt, calcCorrectGraphId] : weapon_json.at("calcCorrectGraphIds").items())
                    calc_correct_graph_ids.insert(calcCorrectGraphId.get<long long>());
            json calc_correct_graphs_json{};
            for (auto &&[id, calc_correct_graph] : this->calcCorrectGraphs)
                if (calc_correct_graph_ids.contains(id))
                    calc_correct_graphs_json[std::to_string(id)] = parse_calc_correct_graph(calc_correct_graph);

            std::set<long long> attackElementCorrectIds{};
            for (auto &&weapon_json : weapons_json)
                attackElementCorrectIds.insert(weapon_json.at("attackElementCorrectId").get<long long>());
            json attack_element_corrects_json{};
            for (auto &&[id, row] : this->attackElementCorrectParams)
                if (attackElementCorrectIds.contains(id))
                    attack_element_corrects_json[std::to_string(id)] = parse_attack_element_correct(row);

            std::set<long long> reinforceTypeIds{};
            for (auto &&weapon_json : weapons_json)
                reinforceTypeIds.insert(weapon_json.at("reinforceTypeId").get<long long>());
            json reinforce_types_json{};
            for (auto &&[reinforceParamId, reinforceParamWeapon] : this->reinforceParamWeapons)
            {
                auto reinforceLevel = reinforceParamId % 50;
                auto reinforceTypeId = reinforceParamId - reinforceLevel;
                auto reinforceTypeId_string = std::to_string(reinforceTypeId);

                if (reinforceTypeIds.contains(reinforceTypeId))
                {
                    if (!reinforce_types_json.contains(reinforceTypeId_string))
                        reinforce_types_json[reinforceTypeId_string] = json::array();

                    auto &&reinforceTypeJson = reinforce_types_json[reinforceTypeId_string];
                    if (reinforceTypeJson.size() == reinforceLevel)
                        reinforceTypeJson.push_back(parse_reinforce_param_weapon(reinforceParamWeapon));
                }
            }

            std::set<long long> statusSpEffectParamIds{};
            for (auto &&weapon_json : weapons_json)
            {
                auto reinforceTypeId_string = std::to_string(weapon_json.at("reinforceTypeId").get<long long>());
                auto &&reinforceParamWeapons = reinforce_types_json.at(reinforceTypeId_string);
                for (auto &&reinforceParamWeapon : reinforceParamWeapons)
                {
                    if (weapon_json.contains("statusSpEffectParamIds"))
                    {
                        int i = 1;
                        for (auto &&spEffectParamId : weapon_json.at("statusSpEffectParamIds").get<std::array<long long, 3>>())
                        {
                            if (spEffectParamId)
                            {
                                auto statusSpEffectId_string = "statusSpEffectId" + std::to_string(i);
                                auto offset = reinforceParamWeapon.value(statusSpEffectId_string, 0ll);
                                statusSpEffectParamIds.insert(spEffectParamId + offset);
                            }
                            ++i;
                        }
                    }
                }
            }
            json status_sp_effect_params_json{};
            for (auto &&[spEffectParamId, _] : this->spEffectParams)
            {
                if (statusSpEffectParamIds.contains(spEffectParamId))
                {
                    auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId);
                    std::erase_if(status_sp_effect_params, [](auto &&v) { return v.second == 0; });
                    status_sp_effect_params_json[std::to_string(spEffectParamId)] = status_sp_effect_params;
                }
            }

            json scaling_tiers_tson = json::array();
            for (auto &&[id, row] : this->menuValueTableParams)
                if (row.at("compareType") == 1 && id >= 100)
                    scaling_tiers_tson.push_back(json::array({row.at("value") / 100., this->menuText.at(row.at("textId"))}));

            json regulation_data_json{
                {"calcCorrectGraphs", calc_correct_graphs_json},
                {"attackElementCorrects", attack_element_corrects_json},
                {"reinforceTypes", reinforce_types_json},
                {"statusSpEffectParams", status_sp_effect_params_json},
                {"scalingTiers", scaling_tiers_tson},
                {"weapons", weapons_json}
            };

            std::println("\nsuccessfully generated regulation data file");

            return regulation_data_json;
        }
    };
} // namespace calculator



void test1()
{
    auto regulation_file = std::filesystem::current_path().parent_path() / "regulation_data_current_game_current_erdo.json";
    auto &&[weap_contain, weap_contain_time] = TimeFunctionExecution([&]() { return calculator::WeaponContainer(regulation_file); });


    calculator::AttackOptions atk_options = {{0, 25, 10}, true};
    auto [stat_variations, stat_variations_time] = TimeFunctionExecution([&]() { return calculator::get_stat_variations(1 + 79, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH)); });

    auto [filtered_weaps, filtered_weapons_time] = TimeFunctionExecution([&]() { return weap_contain.apply_filter(calculator::Weapon::Filter{{}, {}, {}}); });
    std::println();

    auto [attack_rating, attack_rating_time] = TimeFunctionExecution([&]() { return calculator::OptimizationContext(10, stat_variations, filtered_weaps, atk_options, std::type_identity<calculator::AttackRating::total>{}).wait_and_get_result(); });
    std::println();

    std::print("stats: ");
    for (auto stat : attack_rating.stats)
        std::print("{} ", stat);
    std::println("\n");

    std::println("{}: {}\n", attack_rating.weapon->full_name, attack_rating.total_attack_power.at(2));

    for (int i = 0; i < attack_rating.attack_power.size(); i++)
        if (attack_rating.attack_power.at(i).at(2) != 0)
            std::println("{}: {} + {} = {}", enum_to_string(integral_to_enum<calculator::DamageType>(i)), attack_rating.attack_power.at(i).at(0), attack_rating.attack_power.at(i).at(1), attack_rating.attack_power.at(i).at(2));
    std::println();

    for (int i = 0; i < attack_rating.status_effect.size(); i++)
        if (attack_rating.status_effect.at(i).at(2) != 0)
            std::println("{}: {} + {} = {}", enum_to_string(index_to_enum<calculator::StatusType>(i)), attack_rating.status_effect.at(i).at(0), attack_rating.status_effect.at(i).at(1), attack_rating.status_effect.at(i).at(2));
    std::println();

    std::println("spell_scaling: {}%\n", attack_rating.spell_scaling);

    std::println("load weapon container: {}", weap_contain_time);
    std::println("get stat variations: {}", stat_variations_time);
    std::println("filter weapons: {}", filtered_weapons_time);
    std::println("query best stats: {}", attack_rating_time);

    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
}

void test2()
{
    // Madding Hand & Poisoned Hand differ slightly

    auto parser = calculator::Parser(std::filesystem::path("C:/Users/Paul/Downloads/WitchyBND-v3.0.0.1-win-x64/WitchyBND.exe"), std::filesystem::path("F:/Programme/Steam/steamapps/common/ELDEN RING/Game", std::filesystem::path::format::native_format)
                                     // std::filesystem::path("C:/Users/Paul/Desktop/Neuer Ordner") //
    );
    auto regulation_data_json = parser.get_regulation_data_json();


    auto weapons_json = regulation_data_json["weapons"];
    auto calc_correct_graphs_json = regulation_data_json["calcCorrectGraphs"];
    auto attack_element_corrects_json = regulation_data_json["attackElementCorrec"
                                                             "ts"];
    auto reinforce_types_json = regulation_data_json["reinforceTypes"];
    auto status_sp_effect_params_json = regulation_data_json["statusSpEffectParam"
                                                             "s"];
    auto scaling_tiers_tson = regulation_data_json["scalingTiers"];

    auto regulation_file = std::filesystem::current_path().parent_path() / "regulation_data.json";
    auto regulation_data = json::parse(std::ifstream(regulation_file));
    auto weaponJson = regulation_data["weapons"];
    auto calcCorrectGraphs = regulation_data["calcCorrectGraphs"];
    auto attackElementCorrects = regulation_data["attackElementCorrects"];
    auto reinforceTypes = regulation_data["reinforceTypes"];
    auto statusSpEffectParams = regulation_data["statusSpEffectParams"];
    auto scalingTiers = regulation_data["scalingTiers"];

    std::println("my weaponJson: {}", weapons_json.size());
    std::println("weaponJson: {}", weaponJson.size());

    std::println("my calcCorrectGraphs: {}", calc_correct_graphs_json.size());
    std::println("calcCorrectGraphs: {}", calcCorrectGraphs.size());

    std::println("my attackElementCorrects: {}", attack_element_corrects_json.size());
    std::println("attackElementCorrects: {}", attackElementCorrects.size());
    std::println();

    std::println("my reinforceTypes: {}", reinforce_types_json.size());
    std::println("reinforceTypes: {}", reinforceTypes.size());
    std::println();

    std::println("my statusSpEffectParams: {}", status_sp_effect_params_json.size());
    std::println("statusSpEffectParams: {}", statusSpEffectParams.size());
    std::println();

    std::println("my scalingTiers: {}", scaling_tiers_tson.size());
    std::println("scalingTiers: {}", scalingTiers.size());
    std::println();

    if (weapons_json != weaponJson)
    {
        std::println("weapons");
        for (auto &&[myw, w] : std::views::zip(weapons_json, weaponJson))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    if (calc_correct_graphs_json != calcCorrectGraphs)
    {
        std::println("calcCorrectGraphs");
        for (auto &&[myw, w] : std::views::zip(calc_correct_graphs_json, calcCorrectGraphs))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    if (attack_element_corrects_json != attackElementCorrects)
    {
        std::println("attackElementCorrects");
        for (auto &&[myw, w] : std::views::zip(attack_element_corrects_json, attackElementCorrects))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    if (reinforce_types_json != reinforceTypes)
    {
        std::println("reinforceTypes");
        for (auto &&[myw, w] : std::views::zip(reinforce_types_json, reinforceTypes))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    if (status_sp_effect_params_json != statusSpEffectParams)
    {
        std::println("statusSpEffectParams");
        for (auto &&[myw, w] : std::views::zip(status_sp_effect_params_json, statusSpEffectParams))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    if (scaling_tiers_tson != scalingTiers)
    {
        std::println("scalingTiers");
        for (auto &&[myw, w] : std::views::zip(scaling_tiers_tson, scalingTiers))
        {
            if (myw != w)
            {
                std::println("my: {}", myw);
                std::println("other: {}", w);
            }
        }
        std::println();
    }

    std::println("\n\ndone");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 1000));
}

