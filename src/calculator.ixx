module;
#include <string>
export module erdo:calculator;
import :meta;

import std;


export namespace erdo::calculator
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

    enum class StatusEffectType {
        POISON = std::to_underlying(AttackPowerType::POISON),
        SCARLET_ROT = std::to_underlying(AttackPowerType::SCARLET_ROT),
        BLEED = std::to_underlying(AttackPowerType::BLEED),
        FROST = std::to_underlying(AttackPowerType::FROST),
        SLEEP = std::to_underlying(AttackPowerType::SLEEP),
        MADNESS = std::to_underlying(AttackPowerType::MADNESS),
        DEATH_BLIGHT = std::to_underlying(AttackPowerType::DEATH_BLIGHT)
    };
} // namespace erdo::calculator

using namespace erdo;

template<>
constexpr std::array<std::pair<calculator::RelevantAttribute, std::string_view>, 5> enum_string_mapping<erdo::calculator::RelevantAttribute> = {
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
constexpr std::array<std::pair<calculator::StatusEffectType, std::string_view>, 7> enum_string_mapping<calculator::StatusEffectType> = {
    std::pair{calculator::StatusEffectType::POISON, "POISON"},
    std::pair{calculator::StatusEffectType::SCARLET_ROT, "SCARLET_ROT"},
    std::pair{calculator::StatusEffectType::BLEED, "BLEED"},
    std::pair{calculator::StatusEffectType::FROST, "FROST"},
    std::pair{calculator::StatusEffectType::SLEEP, "SLEEP"},
    std::pair{calculator::StatusEffectType::MADNESS, "MADNESS"},
    std::pair{calculator::StatusEffectType::DEATH_BLIGHT, "DEATH_BLIGHT"}
};

export namespace erdo::calculator
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
    using IneffectiveAttackPowerTypes = std::array<bool, enumerators_of<AttackPowerType>().size()>;
    using IneffectiveAttributes = std::array<bool, enumerators_of<RelevantAttribute>().size()>;
    using BaseAttackPower = std::array<double, enumerators_of<AttackPowerType>().size()>;
    using AttackPower = std::array<double, 2>;  // base / full
    using AttackPowers = std::array<AttackPower, enumerators_of<AttackPowerType>().size()>;
    using AttributeScalings = std::array<double, enumerators_of<RelevantAttribute>().size()>;

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

        AttackPower total_attack_power;
        AttackPowers attack_powers;
        double spell_scaling;
        AttributeScalings attribute_scalings;
        IneffectiveAttackPowerTypes ineffective_attack_power_types;
        IneffectiveAttributes ineffective_attributes;
    };

    long long calculate_upgrade_level_index(const auto& base_attack_powers) {
        if (base_attack_powers.size() == 1)
            return 0;
        else if (base_attack_powers.size() == 11)
            return 2;
        else if (base_attack_powers.size() == 26)
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
        std::vector<AttributeScaling> attribute_scalings;
        // base attack power at each upgrade level for each attack power type
        std::vector<BaseAttackPower> base_attack_powers;
        // map indicating which attack power types scale with which player attributes
        AttackElementCorrects attack_power_attribute_scaling;
        // map indicating which scaling curve is used for each attack power type
        std::array<ScalingCurve, enumerators_of<AttackPowerType>().size()> attack_power_scaling_curves;
        // thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
        std::array<std::pair<double, std::string>, 6> scaling_tiers;

        // the index of the upgrade level for this weapon
        long long upgrade_level_index = calculate_upgrade_level_index(this->base_attack_powers);
        // 
        bool is_sorcery_or_incantation_tool = this->sorcery_tool || this->incantation_tool;


        std::string qualified_name(int upgrade_level) const
        {
            if (upgrade_level == 0)
                return this->full_name;
            return std::format("{} +{}", this->full_name, upgrade_level);
        }

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
                stats.at(std::to_underlying(RelevantAttribute::STRENGTH)) *= 1.5;

            return stats;
        }

        std::string calculate_scaling_tier(double scaling) const
        {
            for (auto&& [threshold, tier] : this->scaling_tiers)
                if (scaling >= threshold)
                    return tier;
            return {};
            throw std::invalid_argument(std::format("scaling value {} is below all thresholds", scaling));
        }

        IneffectiveAttributes calculate_ineffective_attributes(const Stats& adjusted_stats) const
        {
            IneffectiveAttributes ineffective_attributes{};
            for (auto&& [ineffective_attribute, adjusted_stat, requirement] : std::views::zip(
                ineffective_attributes,
                adjusted_stats,
                this->requirements
            ))
                if (adjusted_stat < requirement)
                    ineffective_attribute = true;
            return ineffective_attributes;
        }

        IneffectiveAttackPowerTypes calculate_ineffective_attack_power_types(const IneffectiveAttributes& ineffective_attributes) const
        {
            IneffectiveAttackPowerTypes ineffective_attack_power_types{};

            for(auto attack_power_type_index : integral_enumerators_of<AttackPowerType>())
            {
                auto &&scaling_attributes = this->attack_power_attribute_scaling[attack_power_type_index];
                if (std::ranges::any_of(
                        integral_enumerators_of<RelevantAttribute>(),
                        [&](auto attribute)
                        {
                            return ineffective_attributes[attribute] && scaling_attributes[attribute] != 0;
                        }))
                    ineffective_attack_power_types[attack_power_type_index] = true;
            }

            return ineffective_attack_power_types;
        }

        double calculate_total_scaling(
            bool ineffective_attack_power_type,
            const Stats& effective_stats,
            const AttributeScaling& scaling_attributes,
            const AttributeScaling& attribute_scaling_at_upgrade_level,
            const ScalingCurve& scaling_curve
        ) const
        {
            if (ineffective_attack_power_type)
            {
                return 1. - ineffective_attribute_penalty;
            }
            else
            {
                double total_scaling = 1.;

                for (auto &&attribute : integral_enumerators_of<RelevantAttribute>())
                {
                    auto &&attribute_correct = scaling_attributes[attribute];
                    double scaling{};

                    if (attribute_correct != 0)
                    {
                        if (attribute_correct == 1)
                            scaling = attribute_scaling_at_upgrade_level[attribute];
                        else
                            scaling = attribute_correct * attribute_scaling_at_upgrade_level[attribute] / this->attribute_scalings[0][attribute];

                        if (scaling != 0.)
                            total_scaling += scaling * scaling_curve[effective_stats[attribute]];
                    }
                }

                return total_scaling;
            }
        }

        auto calculate_attack_powers(
            const Stats& stats,
            const Stats& adjusted_stats,
            const bool disable_two_handing_attack_power_bonus,
            const AttackPowerType &attack_power_type,
            const IneffectiveAttributes& ineffective_attributes,
            const IneffectiveAttackPowerTypes& ineffective_attack_power_types,
            const BaseAttackPower& base_attack_powers,
            const AttributeScaling& attribute_scaling_at_upgrade_level,
            AttackPowers& attack_powers,
            double& spell_scaling
        ) const
        {
            auto attack_power_type_integral = std::to_underlying(attack_power_type);

            auto is_damage_type = attack_power_type_integral <= std::to_underlying(AttackPowerType::HOLY);
            auto total_scaling = this->calculate_total_scaling(
                ineffective_attack_power_types[attack_power_type_integral],
                (!disable_two_handing_attack_power_bonus && is_damage_type) ? adjusted_stats : stats,
                this->attack_power_attribute_scaling[attack_power_type_integral],
                attribute_scaling_at_upgrade_level,
                this->attack_power_scaling_curves[attack_power_type_integral]
            );

            auto base_attack_power = base_attack_powers[attack_power_type_integral];
            if (base_attack_power != 0)
            {
                auto full_attack_power = base_attack_power * total_scaling;

                auto &&attack_power = attack_powers[attack_power_type_integral];
                attack_power[0] = base_attack_power;
                attack_power[1] = full_attack_power;
            }

            if (attack_power_type == AttackPowerType::PHYSICAL && this->is_sorcery_or_incantation_tool)
                spell_scaling = total_scaling;

            return;
        }

        AttackPower calculate_total_attack_power(const AttackPowers attack_powers) const
        {
            AttackPower total_attack_power{};
            
            for (auto damage_type : integral_enumerators_of<DamageType>())
            {
                auto&& attack_power = attack_powers[damage_type];

                total_attack_power[0] += attack_power[0];
                total_attack_power[1] += attack_power[1];
            }

            return total_attack_power;
        }

        AttackRating calculate_attack_rating(const AttackOptions &attack_options, const Stats &stats) const
        {
            auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options.two_handing, stats);
            auto upgrade_level = attack_options.upgrade_levels[upgrade_level_index];

            AttackRating attack_rating{
                .stats=stats,
                .attack_options=attack_options,
                .weapon=*this,
                .attribute_scalings = this->attribute_scalings[upgrade_level],
                .ineffective_attributes=this->calculate_ineffective_attributes(adjusted_stats),
            };
            attack_rating.ineffective_attack_power_types = this->calculate_ineffective_attack_power_types(attack_rating.ineffective_attributes);

            auto&& base_attack_powers = this->base_attack_powers[upgrade_level];

            for (auto &&attack_power_type : enumerators_of<AttackPowerType>())
                this->calculate_attack_powers(
                    stats,
                    adjusted_stats,
                    attack_options.disable_two_handing_attack_power_bonus,
                    attack_power_type,
                    attack_rating.ineffective_attributes,
                    attack_rating.ineffective_attack_power_types,
                    base_attack_powers,
                    this->attribute_scalings[upgrade_level],
                    attack_rating.attack_powers,
                    attack_rating.spell_scaling
                );

            attack_rating.total_attack_power = this->calculate_total_attack_power(attack_rating.attack_powers);

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

} // namespace erdo::calculator

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