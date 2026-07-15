export module erdo:calculator;
import :meta;

import std;


template <typename Map, typename Key, typename Default>
auto map_get(Map &&m, Key &&key, Default &&default_) {
    using result_type = std::common_reference_t<typename std::remove_cvref_t<Map>::mapped_type, Default &&>;

    auto it = m.find(std::forward<Key>(key));
    if (it == m.end())
        return result_type(std::forward<Default>(default_));
    return result_type(it->second);
}

template <typename T>
std::pair<std::invoke_result_t<T &&>, std::chrono::nanoseconds> TimeFunctionExecution(T &&func) {
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

    auto &&result = func();

    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

    // Getting number of milliseconds as a double
    return {std::forward<decltype(result)>(result), t2 - t1};
}


export template <typename T>
constexpr T assert_floating_is(double f) {
    if (f != (T)f)
        throw std::runtime_error("floating is not T");
    return f;
}

export namespace calculator
{
    struct Weapon;
    using UpgradeLevels = std::array<int, 3>; // free handed, normal, somber

    constexpr bool isVanilla = true;

    enum class RelevantAttribute {
        STRENGTH,
        DEXTERITY,
        INTELLIGENCE,
        FAITH,
        ARCAINE,
    };

    enum class Attribute {
        VIGOR = -3,
        MIND = -2,
        ENDURANCE = -1,
        
        STRENGTH = std::to_underlying(RelevantAttribute::STRENGTH),
        DEXTERITY = std::to_underlying(RelevantAttribute::DEXTERITY),
        INTELLIGENCE = std::to_underlying(RelevantAttribute::INTELLIGENCE),
        FAITH = std::to_underlying(RelevantAttribute::FAITH),
        ARCAINE = std::to_underlying(RelevantAttribute::ARCAINE),
    };

    enum class AttackPowerType {
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

    enum class DamageType {
        PHYSICAL = std::to_underlying(AttackPowerType::PHYSICAL),
        MAGIC = std::to_underlying(AttackPowerType::MAGIC),
        FIRE = std::to_underlying(AttackPowerType::FIRE),
        LIGHTNING = std::to_underlying(AttackPowerType::LIGHTNING),
        HOLY = std::to_underlying(AttackPowerType::HOLY)
    };

    enum class StatusType {
        POISON = std::to_underlying(AttackPowerType::POISON),
        SCARLET_ROT = std::to_underlying(AttackPowerType::SCARLET_ROT),
        BLEED = std::to_underlying(AttackPowerType::BLEED),
        FROST = std::to_underlying(AttackPowerType::FROST),
        SLEEP = std::to_underlying(AttackPowerType::SLEEP),
        MADNESS = std::to_underlying(AttackPowerType::MADNESS),
        DEATH_BLIGHT = std::to_underlying(AttackPowerType::DEATH_BLIGHT)
    };
} // namespace calculator

template<>
constexpr std::array<std::pair<calculator::RelevantAttribute, std::string_view>, 5> enum_string_mapping<calculator::RelevantAttribute> = {
    std::pair{calculator::RelevantAttribute::STRENGTH, "STRENGTH"},
    std::pair{calculator::RelevantAttribute::DEXTERITY, "DEXTERITY"},
    std::pair{calculator::RelevantAttribute::INTELLIGENCE, "INTELLIGENCE"},
    std::pair{calculator::RelevantAttribute::FAITH, "FAITH"},
    std::pair{calculator::RelevantAttribute::ARCAINE, "ARCAINE"}
};
template<>
constexpr std::array<std::pair<calculator::Attribute, std::string_view>, 8> enum_string_mapping<calculator::Attribute> = {
    std::pair{calculator::Attribute::VIGOR, "VIGOR"},
    std::pair{calculator::Attribute::MIND, "MIND"},
    std::pair{calculator::Attribute::ENDURANCE, "ENDURANCE"},
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

export namespace calculator
{
    constexpr auto irrelevant_attribute_count = enumerators_of<Attribute>().size() - enumerators_of<RelevantAttribute>().size();
    struct Stats : std::array<int, enumerators_of<RelevantAttribute>().size()> {

    };
    struct FullStats : std::array<int, enumerators_of<Attribute>().size()> {
        constexpr Stats to_stats() const {
            Stats stats{};
            for (auto attribute : enumerators_of<RelevantAttribute>())
                stats.at(std::to_underlying(attribute)) = this->at(std::to_underlying(attribute) + irrelevant_attribute_count);
            return stats;
        }
    };
    const std::map<std::string, FullStats> character_class_stats{
        {"hero", {14, 9, 9, 16, 9, 7, 8, 11}},
        {"bandit", {10, 13, 11, 9, 13, 9, 8, 14}},
        {"astrologer", {9, 12, 15, 8, 12, 16, 7, 9}},
        {"warrior", {11, 16, 12, 10, 16, 10, 8, 9}},
        {"prisoner", {11, 14, 12, 11, 14, 14, 6, 9}},
        {"confessor", {10, 12, 13, 12, 12, 9, 14, 9}},
        {"wretch", {10, 10, 10, 10, 10, 10, 10, 10}},
        {"vagabond", {15, 13, 10, 14, 13, 9, 9, 7}},
        {"prophet", {10, 10, 14, 11, 10, 7, 16, 10}},
        {"samurai", {12, 15, 11, 12, 15, 9, 8, 8}},
        {"heavy knight", {14, 11, 8, 7, 17, 8, 15, 9}},
        {"idus knight", {10, 15, 12, 8, 11, 11, 13, 6}},
    };

    using ScalingCurve = std::array<double, 149>;
    using AttributeScaling = std::array<double, enumerators_of<RelevantAttribute>().size()>;
    using AttackElementCorrects = std::array<AttributeScaling, enumerators_of<AttackPowerType>().size()>;
    using AttackElementCorrectsById = std::map<int, AttackElementCorrects>;


    constexpr auto ineffective_attribute_penalty = 0.4;
    constexpr auto defaultDamageCalcCorrectGraphId = 0;
    constexpr auto defaultStatusCalcCorrectGraphId = 6;

    struct AttackOptions {
        UpgradeLevels upgrade_levels; // free handed, normal, somber
        bool two_handing;
        static constexpr bool disable_two_handing_attack_power_bonus = false;
    };

    struct AttackRating {
        Stats stats;
        AttackOptions attack_options;
        std::reference_wrapper<const Weapon> weapon;

        std::array<double, 3> total_attack_power;                                             // a + b = c
        std::array<std::array<double, 3>, enumerators_of<DamageType>().size()> attack_power;  // a + b = c
        std::array<std::array<double, 3>, enumerators_of<StatusType>().size()> status_effect; // a + b = c
        double spell_scaling;
        std::array<bool, enumerators_of<AttackPowerType>().size()> ineffective_attack_power_types;
        std::array<bool, enumerators_of<RelevantAttribute>().size()> ineffective_attributes;
    };

    auto calculate_upgrade_level_index(const auto& base_attack_power) {
        if (base_attack_power.size() == 1)
            return 0;
        else if (base_attack_power.size() == 11)
            return 2;
        else if (base_attack_power.size() == 26)
            return 1;
        else
            throw std::runtime_error("invalid base attack power size");
    }

    struct Weapon {
        enum class Affinity {
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

        enum class Type {
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

        // the full unique name of the weapon, e.g. "Heavy Nightrider Glaive"
        std::string full_name;
        // the base weapon name without an affinity specified, e.g. "Nightrider Glaive"
        std::string base_name;
        // a wiki link for the weapon
        std::string url;
        // true if the weapon was introduced with SOTE
        bool dlc;
        // true if the weapon doesn't get a strength bonus when two-handing
        bool paired;
        // true if this weapon can cast glintstone sorceries
        bool sorcery_tool;
        // true if this weapon can cast incantations
        bool incantation_tool;
        // the category of the weapon, e.g. Type.STRAIGHT_SWORD
        Type type;
        // the affinity of the weapon, e.g. Affinity.HEAVY
        Affinity affinity;
        // stat requirements necessary to use the weapon effectively (without an attack rating penalty)
        Stats requirements;
        // scaling amount at each upgrade level (0-10 or 0-25) for each player attribute (e.g. Attribute.STRENGTH)
        std::vector<AttributeScaling> attribute_scaling;
        // base attack power at each upgrade level for each attack power type
        std::vector<std::array<double, enumerators_of<AttackPowerType>().size()>> base_attack_power;
        // map indicating which attack power types scale with which player attributes
        AttackElementCorrectsById::mapped_type attack_power_attribute_scaling;
        // map indicating which scaling curve is used for each attack power type
        std::array<ScalingCurve, enumerators_of<AttackPowerType>().size()> attack_power_scaling_curves;
        // thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
        std::array<std::pair<double, std::string>, 6> scaling_tiers;

        // the index of the upgrade level for this weapon
        int upgrade_level_index = calculate_upgrade_level_index(base_attack_power);

        Stats adjust_stats_for_two_handing(bool two_handing, Stats stats) const {
            // Paired weapons do not get the two handing bonus
            if (this->paired)
                two_handing = false;

            // Bows and ballistae can only be two handed
            constexpr std::array<Weapon::Type, 4> bow_types = {Weapon::Type::LIGHT_BOW, Weapon::Type::BOW, Weapon::Type::GREATBOW, Weapon::Type::BALLISTA};
            if (std::ranges::contains(bow_types, this->type))
                two_handing = true;

            if (two_handing)
                stats.at(std::to_underlying(RelevantAttribute::STRENGTH)) *= 1.5;

            return stats;
        }

        AttackRating get_attack_rating(const AttackOptions &attack_options, const Stats &stats) const {
            auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options.two_handing, stats);

            AttackRating attack_rating{ stats, attack_options, *this };

            for (auto attribute : enumerators_of<RelevantAttribute>())
                if (adjusted_stats[std::to_underlying(attribute)] < this->requirements[std::to_underlying(attribute)])
                    attack_rating.ineffective_attributes.at(std::to_underlying(attribute)) = true;

            auto upgrade_level = attack_options.upgrade_levels.at(upgrade_level_index);
            auto &base_attack_power_at_upgrade_level = this->base_attack_power.at(upgrade_level);

            bool is_sorcery_or_incantation_tool = this->sorcery_tool || this->incantation_tool;

            auto loop_cycle = [&](const AttackPowerType &attack_power_type) {
                auto temp_index = std::to_underlying(attack_power_type);
                auto base_attack_power = base_attack_power_at_upgrade_level[temp_index];

                if (base_attack_power != 0 || is_sorcery_or_incantation_tool)
                {
                    auto is_damage_type = std::to_underlying(attack_power_type) <= std::to_underlying(AttackPowerType::HOLY);
                    auto &&scaling_attributes = this->attack_power_attribute_scaling.at(std::to_underlying(attack_power_type));
                    double total_scaling = 1.;

                    if (std::ranges::any_of(
                            enumerators_of<RelevantAttribute>(),
                            [&](RelevantAttribute attribute)
                            {
                                return attack_rating.ineffective_attributes.at(std::to_underlying(attribute)) && scaling_attributes[std::to_underlying(attribute)] != 0;
                            }))
                    {
                        total_scaling = 1. - ineffective_attribute_penalty;
                        attack_rating.ineffective_attack_power_types.at(std::to_underlying(attack_power_type)) = true;
                    }
                    else
                    {
                        auto &effective_stats =
                            (!attack_options.disable_two_handing_attack_power_bonus &&
                             is_damage_type)
                                ? adjusted_stats
                                : stats;

                        for (auto &&attribute : enumerators_of<RelevantAttribute>())
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

                        if (is_damage_type) // attack_power_type._to_integral() <= AttackPowerType::HOLY
                        {
                            auto &&att_pwr =
                                attack_rating.attack_power[std::to_underlying(attack_power_type)];
                            att_pwr[0] = base_attack_power;
                            att_pwr[1] = res - base_attack_power;
                            att_pwr[2] = res;
                            attack_rating.total_attack_power[0] += base_attack_power;
                            attack_rating.total_attack_power[1] += res - base_attack_power;
                            attack_rating.total_attack_power[2] += res;
                        }
                        else // attack_power_type._to__integral() > AttackPowerType::HOLY
                        {
                            auto &&att_pwr = attack_rating.status_effect[std::to_underlying(attack_power_type) - std::to_underlying(AttackPowerType::POISON)];
                            att_pwr[0] = base_attack_power;
                            att_pwr[1] = res - base_attack_power;
                            att_pwr[2] = res;
                        }
                        
                    }

                    if (attack_power_type == AttackPowerType::PHYSICAL && is_sorcery_or_incantation_tool)
                        attack_rating.spell_scaling = 100. * total_scaling;
                } 
            };


            for (auto &&attack_power_type : enumerators_of<AttackPowerType>())
                loop_cycle(attack_power_type);

            return attack_rating;
        }
    };

    struct CalcCorrectGraphEntry {
        long long maxVal;
        double maxGrowVal, adjPt;
    };
    using CalcCorrectGraph = std::array<CalcCorrectGraphEntry, 5>;
    struct ReinforceTypesDict {
        AttributeScaling attack;             // index: AttackPowerType (if in ALL_DAMAGE_TYPES)
        AttributeScaling attributeScaling;   // index: Attribute
        std::array<int, 3> statusSpEffectId; // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3
    };

    constexpr std::size_t get_stat_variation_count(const int attribute_points, const Stats &min_stats) {
        constexpr auto N = Stats{}.size();
        constexpr auto UPPER = 99;
        const auto SUM = attribute_points;
        std::size_t count = 0;

        if (attribute_points > UPPER * min_stats.size())
            return 0;
        // throw std::invalid_argument("attribute_points must be <= " +
        // std::to_string(UPPER) + " * " + std::to_string(N));

        if (std::ranges::any_of(min_stats, [](int v) { return v > UPPER; }))
            throw std::invalid_argument("min_stats must be <= " + std::to_string(UPPER));

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
    std::vector<Stats> get_stat_variations(const int attribute_points, const Stats &min_stats) {
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

} // namespace calculator


// template<>
// constexpr std::array<std::pair<calculator::Class, std::string_view>, 10> enum_string_mapping<calculator::Class> = {
//     std::pair{calculator::Class::HERO, "HERO"},
//     std::pair{calculator::Class::BANDIT, "BANDIT"},
//     std::pair{calculator::Class::ASTROLOGER, "ASTROLOGER"},
//     std::pair{calculator::Class::WARRIOR, "WARRIOR"},
//     std::pair{calculator::Class::PRISONER, "PRISONER"},
//     std::pair{calculator::Class::CONFESSOR, "CONFESSOR"},
//     std::pair{calculator::Class::WRETCH, "WRETCH"},
//     std::pair{calculator::Class::VAGABOND, "VAGABOND"},
//     std::pair{calculator::Class::PROPHET, "PROPHET"},
//     std::pair{calculator::Class::SAMURAI, "SAMURAI"}
// };
template<>
constexpr std::array<std::pair<calculator::Weapon::Affinity, std::string_view>, 14> enum_string_mapping<calculator::Weapon::Affinity> = {
    std::pair{calculator::Weapon::Affinity::STANDARD, "STANDARD"},
    std::pair{calculator::Weapon::Affinity::HEAVY, "HEAVY"},
    std::pair{calculator::Weapon::Affinity::KEEN, "KEEN"},
    std::pair{calculator::Weapon::Affinity::QUALITY, "QUALITY"},
    std::pair{calculator::Weapon::Affinity::FIRE, "FIRE"},
    std::pair{calculator::Weapon::Affinity::FLAME_ART, "FLAME_ART"},
    std::pair{calculator::Weapon::Affinity::LIGHTNING, "LIGHTNING"},
    std::pair{calculator::Weapon::Affinity::SACRED, "SACRED"},
    std::pair{calculator::Weapon::Affinity::MAGIC, "MAGIC"},
    std::pair{calculator::Weapon::Affinity::COLD, "COLD"},
    std::pair{calculator::Weapon::Affinity::POISON, "POISON"},
    std::pair{calculator::Weapon::Affinity::BLOOD, "BLOOD"},
    std::pair{calculator::Weapon::Affinity::OCCULT, "OCCULT"},
    std::pair{calculator::Weapon::Affinity::UNIQUE, "UNIQUE"}
};
template<>
constexpr std::array<std::pair<calculator::Weapon::Type, std::string_view>, 47> enum_string_mapping<calculator::Weapon::Type> = {
    std::pair{calculator::Weapon::Type::DAGGER, "DAGGER"},
    std::pair{calculator::Weapon::Type::STRAIGHT_SWORD, "STRAIGHT_SWORD"},
    std::pair{calculator::Weapon::Type::GREATSWORD, "GREATSWORD"},
    std::pair{calculator::Weapon::Type::COLOSSAL_SWORD, "COLOSSAL_SWORD"},
    std::pair{calculator::Weapon::Type::CURVED_SWORD, "CURVED_SWORD"},
    std::pair{calculator::Weapon::Type::CURVED_GREATSWORD, "CURVED_GREATSWORD"},
    std::pair{calculator::Weapon::Type::KATANA, "KATANA"},
    std::pair{calculator::Weapon::Type::TWINBLADE, "TWINBLADE"},
    std::pair{calculator::Weapon::Type::THRUSTING_SWORD, "THRUSTING_SWORD"},
    std::pair{calculator::Weapon::Type::HEAVY_THRUSTING_SWORD, "HEAVY_THRUSTING_SWORD"},
    std::pair{calculator::Weapon::Type::AXE, "AXE"},
    std::pair{calculator::Weapon::Type::GREATAXE, "GREATAXE"},
    std::pair{calculator::Weapon::Type::HAMMER, "HAMMER"},
    std::pair{calculator::Weapon::Type::GREAT_HAMMER, "GREAT_HAMMER"},
    std::pair{calculator::Weapon::Type::FLAIL, "FLAIL"},
    std::pair{calculator::Weapon::Type::SPEAR, "SPEAR"},
    std::pair{calculator::Weapon::Type::GREAT_SPEAR, "GREAT_SPEAR"},
    std::pair{calculator::Weapon::Type::HALBERD, "HALBERD"},
    std::pair{calculator::Weapon::Type::REAPER, "REAPER"},
    std::pair{calculator::Weapon::Type::FIST, "FIST"},
    std::pair{calculator::Weapon::Type::CLAW, "CLAW"},
    std::pair{calculator::Weapon::Type::WHIP, "WHIP"},
    std::pair{calculator::Weapon::Type::COLOSSAL_WEAPON, "COLOSSAL_WEAPON"},
    std::pair{calculator::Weapon::Type::LIGHT_BOW, "LIGHT_BOW"},
    std::pair{calculator::Weapon::Type::BOW, "BOW"},
    std::pair{calculator::Weapon::Type::GREATBOW, "GREATBOW"},
    std::pair{calculator::Weapon::Type::CROSSBOW, "CROSSBOW"},
    std::pair{calculator::Weapon::Type::BALLISTA, "BALLISTA"},
    std::pair{calculator::Weapon::Type::GLINTSTONE_STAFF, "GLINTSTONE_STAFF"},
    std::pair{calculator::Weapon::Type::DUAL_CATALYST, "DUAL_CATALYST"},
    std::pair{calculator::Weapon::Type::SACRED_SEAL, "SACRED_SEAL"},
    std::pair{calculator::Weapon::Type::SMALL_SHIELD, "SMALL_SHIELD"},
    std::pair{calculator::Weapon::Type::MEDIUM_SHIELD, "MEDIUM_SHIELD"},
    std::pair{calculator::Weapon::Type::GREATSHIELD, "GREATSHIELD"},
    std::pair{calculator::Weapon::Type::TORCH, "TORCH"},
    std::pair{calculator::Weapon::Type::HAND_TO_HAND, "HAND_TO_HAND"},
    std::pair{calculator::Weapon::Type::PERFUME_BOTTLE, "PERFUME_BOTTLE"},
    std::pair{calculator::Weapon::Type::THRUSTING_SHIELD, "THRUSTING_SHIELD"},
    std::pair{calculator::Weapon::Type::THROWING_BLADE, "THROWING_BLADE"},
    std::pair{calculator::Weapon::Type::BACKHAND_BLADE, "BACKHAND_BLADE"},
    std::pair{calculator::Weapon::Type::LIGHT_GREATSWORD, "LIGHT_GREATSWORD"},
    std::pair{calculator::Weapon::Type::GREAT_KATANA, "GREAT_KATANA"},
    std::pair{calculator::Weapon::Type::BEAST_CLAW, "BEAST_CLAW"}
};