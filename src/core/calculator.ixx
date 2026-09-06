module;
#include <cassert>
#include <string>
export module erdo:calculator;
import :meta;

import std;


export namespace erdo::calculator
{
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
    constexpr auto attribute_points_to_character_level(int attribute_points)
    {
        return attribute_points - 79;
    }
    constexpr auto character_level_to_attribute_points(int character_level)
    {
        return character_level + 79;
    }

    constexpr auto attribute_level_limit = 99;
    constexpr auto irrelevant_attribute_count = enumerators_of<Attribute>().size() - enumerators_of<RelevantAttribute>().size();
    using RelevantAttributeLevelsArray = std::array<unsigned int, enumerators_of<RelevantAttribute>().size()>;
    using RelevantAttributeLevels = std::span<const unsigned int, enumerators_of<RelevantAttribute>().size()>;
    using IrrelevantAttributeLevels = std::span<const unsigned int, irrelevant_attribute_count>;
    struct AttributeLevels : std::array<unsigned int, enumerators_of<Attribute>().size()>
    {
        constexpr RelevantAttributeLevels relevant_stats() const
        {
            return RelevantAttributeLevels{ this->begin() + irrelevant_attribute_count, this->end() };
        }
        constexpr IrrelevantAttributeLevels irrelevant_stats() const
        {
            return IrrelevantAttributeLevels{
                this->begin(),
                this->begin() + irrelevant_attribute_count
            };
        }

        constexpr int attribute_points() const
        {
            return std::ranges::fold_left(*this, 0, std::plus<int>{});
        }
        constexpr int character_level() const
        {
            return attribute_points_to_character_level(this->attribute_points());
        }
    };
    const std::array<std::pair<std::string, AttributeLevels>, 12> character_class_attributes{{
        {"Hero", {14, 9, 9, 16, 9, 7, 8, 11}},
        {"Bandit", {10, 13, 11, 9, 13, 9, 8, 14}},
        {"Astrologer", {9, 12, 15, 8, 12, 16, 7, 9}},
        {"Warrior", {11, 16, 12, 10, 16, 10, 8, 9}},
        {"Prisoner", {11, 14, 12, 11, 14, 14, 6, 9}},
        {"Confessor", {10, 12, 13, 12, 12, 9, 14, 9}},
        {"Wretch", {10, 10, 10, 10, 10, 10, 10, 10}},
        {"Vagabond", {15, 13, 10, 14, 13, 9, 9, 7}},
        {"Prophet", {10, 10, 14, 11, 10, 7, 16, 10}},
        {"Samurai", {12, 15, 11, 12, 15, 9, 8, 8}},
        {"Heavy Knight", {14, 11, 8, 7, 17, 8, 15, 9}},
        {"Idus Knight", {10, 15, 12, 8, 11, 11, 13, 6}},
    }};
    const AttributeLevels& get_character_class_attributes(const std::string& class_name)
    {
        auto it = std::ranges::find(character_class_attributes, class_name, &decltype(character_class_attributes)::value_type::first);
        if (it != character_class_attributes.end())
            return it->second;
        throw std::invalid_argument("Invalid character class name: " + class_name);
    }

    using UpgradeLevels = std::array<unsigned int, 3>; // free handed, normal, somber
    constexpr auto max_upgrade_levels = UpgradeLevels{ 0, 25, 10 };
    using ScalingCurve = std::array<double, 149>;
    using ScalingCurves = std::array<ScalingCurve, enumerators_of<AttackPowerType>().size()>;
    using AttributeScalings = std::array<double, enumerators_of<RelevantAttribute>().size()>;
    using AttackElementCorrects = std::array<AttributeScalings, enumerators_of<AttackPowerType>().size()>;
    using AttackElementCorrectsById = std::map<int, AttackElementCorrects>;
    using IneffectiveAttackPowerTypes = std::array<bool, enumerators_of<AttackPowerType>().size()>;
    using IneffectiveAttributes = std::array<bool, enumerators_of<RelevantAttribute>().size()>;
    using BaseAttackPowers = std::array<double, enumerators_of<AttackPowerType>().size()>;
    using BaseAttackPowersAtUpgradeLevels = std::vector<BaseAttackPowers>;
    using ScalingTiers = std::array<std::pair<double, std::string>, 6>;
    using AttributeScalingsAtUpgradeLevels = std::vector<AttributeScalings>;
    using NonscalingAttributes = IneffectiveAttributes;

    using TotalScalings = std::array<double, enumerators_of<AttackPowerType>().size()>;
    using AttackPower = std::array<double, 2>;  // base / full
    using AttackPowers = std::array<AttackPower, enumerators_of<AttackPowerType>().size()>;


    constexpr auto ineffective_attribute_penalty = 0.4;
    constexpr auto defaultDamageCalcCorrectGraphId = 0;
    constexpr auto defaultStatusCalcCorrectGraphId = 6;


    struct Weapon
    {
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
        RelevantAttributeLevelsArray requirements;
        // scaling amount at each upgrade level (0-10 or 0-25) for each player attribute (e.g. Attribute.STRENGTH)
        AttributeScalingsAtUpgradeLevels attribute_scalings_at_upgrade_levels;
        // base attack power at each upgrade level for each attack power type
        BaseAttackPowersAtUpgradeLevels base_attack_powers_at_upgrade_levels;
        // each attack power type's scaling with each character attribute
        AttackElementCorrects attack_power_types_attribute_scalings;
        // each attack power type's scaling curve
        ScalingCurves attack_power_scaling_curves;
        // thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
        ScalingTiers scaling_tiers;

        // the index of the upgrade level for this weapon
        int upgrade_level_index = [&]() {
            for (auto [i, max_upgrade_level] : max_upgrade_levels | std::views::enumerate)
                if (this->base_attack_powers_at_upgrade_levels.size() == max_upgrade_level + 1)
                    return i;
            
            throw std::runtime_error("invalid base attack power size");
        }();
        // whether the weapon is a catalyst
        bool is_sorcery_or_incantation_tool = this->sorcery_tool || this->incantation_tool;
        // attributes which this weapon does not scale with, i.e. which don't affect its attack rating in any way
        NonscalingAttributes nonscaling_attributes = [&](){
            NonscalingAttributes result{};
            for (auto attribute : enumerator_integrals_of<calculator::RelevantAttribute>())
            {
                result[attribute] = std::ranges::all_of(enumerator_integrals_of<calculator::AttackPowerType>(), [&](int apt){
                    auto&& attribute_correct = this->attack_power_types_attribute_scalings[apt][attribute];

                    // If attribute_correct is 0, this attribute is ignored for both scaling and penalty for this apt
                    if (attribute_correct == 0.)
                        return true;

                    auto can_trigger_penalty = this->requirements[attribute] > 0;
                    std::vector<bool> independant_at_upgrade_levels{};
                    for (auto upgrade_level = 0; upgrade_level < this->base_attack_powers_at_upgrade_levels.size(); ++upgrade_level)
                    {
                        // 1. Direct Scaling Dependency
                        if (this->attribute_scalings_at_upgrade_levels[upgrade_level][attribute] != 0.)
                        {
                            // return false;
                            independant_at_upgrade_levels.push_back(false);
                            continue;
                        }

                        // 2. Ineffectiveness Penalty Dependency
                        auto&& base_attack_power = this->base_attack_powers_at_upgrade_levels[upgrade_level][apt];
                        if ((base_attack_power != 0. || this->is_sorcery_or_incantation_tool) && can_trigger_penalty)
                        {
                            // return false;
                            independant_at_upgrade_levels.push_back(false);
                            continue;
                        }

                        independant_at_upgrade_levels.push_back(true);
                    }

                    if (std::ranges::all_of(independant_at_upgrade_levels, [&](auto x) {return x == true;}))
                        return true;
                    else if (std::ranges::all_of(independant_at_upgrade_levels, [&](auto x) {return x == false;}))
                        return false;

                    throw std::runtime_error(std::format("inconsistent independance of apt {} from attribute {} for {} at upgrade levels: {}",
                        enum_to_string((AttackPowerType)apt),
                        enum_to_string((Attribute)attribute),
                        this->full_name,
                        independant_at_upgrade_levels
                    ));
                });
            }
            return result;
        }();


        std::string fandom_url() const
        {
            auto url_part = this->base_name;
            std::ranges::replace(url_part, ' ', '_');
            return "https://eldenring.fandom.com/wiki/" + url_part;
        }
        std::string fextralife_url() const
        {
            auto url_part = this->base_name;
            std::ranges::replace(url_part, ' ', '+');
            return "https://eldenring.wiki.fextralife.com/" + url_part;
        }

        std::string qualified_name(int upgrade_level) const
        {
            if (upgrade_level == 0)
                return this->full_name;
            return std::format("{} +{}", this->full_name, upgrade_level);
        }

        int max_upgrade_level() const
        {
            return max_upgrade_levels.at(this->upgrade_level_index);
        }

        static const Weapon dummy;
    };
    const Weapon Weapon::dummy { .upgrade_level_index = 0 };


    struct AttackOptions
    {
        static constexpr bool disable_two_handing_attack_power_bonus = false;

        UpgradeLevels upgrade_levels; // free handed, normal, somber
        bool two_handing;
    };

    struct FullAttackOptions : AttackOptions
    {
        std::reference_wrapper<const Weapon> weapon;
        AttributeLevels stats;

        FullAttackOptions(const Weapon& weapon, const AttributeLevels& stats, const AttackOptions& attack_options)
            : AttackOptions{attack_options}, weapon{ weapon }, stats{ stats } { }

        RelevantAttributeLevelsArray adjust_stats_for_two_handing() const
        {
            RelevantAttributeLevelsArray adjusted_relevant_stats{};
            std::ranges::copy(this->stats.relevant_stats(), adjusted_relevant_stats.begin());

            auto effective_two_handing = this->two_handing;

            // Paired weapons do not get the two handing bonus
            if (this->weapon.get().paired)
                effective_two_handing = false;

            // Bows and ballistae can only be two handed
            constexpr std::array<Weapon::Type, 4> bow_types = {Weapon::Type::LIGHT_BOW, Weapon::Type::BOW, Weapon::Type::GREATBOW, Weapon::Type::BALLISTA};
            if (std::ranges::contains(bow_types, this->weapon.get().type))
                effective_two_handing = true;

            if (effective_two_handing && !this->disable_two_handing_attack_power_bonus)
                adjusted_relevant_stats.at(std::to_underlying(Attribute::STRENGTH)) *= 1.5;

            return adjusted_relevant_stats;
        }

        int upgrade_level() const
        {
            return this->upgrade_levels.at(this->weapon.get().upgrade_level_index);
        }

        const AttributeScalings& attribute_scalings_at_upgrade_level() const
        {
            return this->weapon.get().attribute_scalings_at_upgrade_levels.at(this->upgrade_level());
        }

        const BaseAttackPowers& base_attack_powers() const
        {
            return this->weapon.get().base_attack_powers_at_upgrade_levels.at(this->upgrade_level());
        }
    
        const AttributeScalings& attack_power_type_attribute_scalings(AttackPowerType apt) const
        {
            return this->weapon.get().attack_power_types_attribute_scalings.at(std::to_underlying(apt));
        }

        std::vector<std::string> calculate_scaling_tiers() const
        {
            auto&& attribute_scalings_at_upgrade_level = this->attribute_scalings_at_upgrade_level();
            std::vector<std::string> scaling_tiers{ attribute_scalings_at_upgrade_level.size() };
            for (auto&& [scaling, scaling_tier] : std::views::zip(attribute_scalings_at_upgrade_level, scaling_tiers))
            {
                for (auto&& [threshold, tier] : this->weapon.get().scaling_tiers)
                    if (scaling >= threshold)
                        scaling_tier = tier;
            }
            return scaling_tiers;
        }
    };

    struct AttackRating
    {
        AttackPower total_attack_power;
        AttackPowers attack_powers;
        double spell_scaling;
        TotalScalings total_scalings;
        IneffectiveAttackPowerTypes ineffective_attack_power_types;
        IneffectiveAttributes ineffective_attributes;
    };

    class Attack : public FullAttackOptions, public AttackRating
    {
        void calculate_ineffective_attributes_inplace(const RelevantAttributeLevels& adjusted_relevant_stats)
        {
            for (auto&& [ineffective_attribute, adjusted_stat, requirement] : std::views::zip(
                this->ineffective_attributes,
                adjusted_relevant_stats,
                this->weapon.get().requirements
            ))
                ineffective_attribute = adjusted_stat < requirement;
        }

        double calculate_total_scaling(
            const bool is_ineffective_attack_power_type,
            const RelevantAttributeLevels& effective_relevant_stats,
            const AttributeScalings& scaling_attributes,
            const AttributeScalings& attribute_scalings_at_upgrade_level,
            const ScalingCurve& scaling_curve
        ) const
        {
            auto&& weapon = this->weapon.get();

            if (is_ineffective_attack_power_type)
            {
                return 1. - ineffective_attribute_penalty;
            }
            else
            {
                double total_scaling = 1.;

                for (auto &&attribute : enumerator_integrals_of<RelevantAttribute>())
                {
                    auto &&attribute_correct = scaling_attributes[attribute];
                    double scaling{};

                    if (attribute_correct != 0)
                    {
                        if (attribute_correct == 1)
                            scaling = attribute_scalings_at_upgrade_level[attribute];
                        else
                            scaling = attribute_correct * attribute_scalings_at_upgrade_level[attribute] / weapon.attribute_scalings_at_upgrade_levels[0][attribute];

                        if (scaling != 0.)
                            total_scaling += scaling * scaling_curve[effective_relevant_stats[attribute]];
                    }
                }

                return total_scaling;
            }
        }

        void calculate_attack_power_inplace_impl(
            const AttackPowerType attack_power_type,
            const RelevantAttributeLevels& relevant_stats,
            const RelevantAttributeLevels& adjusted_relevant_stats,
            const IneffectiveAttributes& ineffective_attributes,
            const BaseAttackPowers& base_attack_powers,
            const AttributeScalings& attribute_scalings_at_upgrade_level
        )
        {
            const auto attack_power_type_integral = std::to_underlying(attack_power_type);
            const auto is_damage_type = attack_power_type <= AttackPowerType::HOLY;
            
            auto&& weapon = this->weapon.get();
            auto&& scaling_attributes = this->attack_power_type_attribute_scalings(attack_power_type);
            auto&& scaling_curve = weapon.attack_power_scaling_curves[attack_power_type_integral];
            auto&& base_attack_power = base_attack_powers[attack_power_type_integral];

            this->ineffective_attack_power_types[attack_power_type_integral] = false;
            if (base_attack_power != 0 || weapon.is_sorcery_or_incantation_tool)
            {
                for (auto&& [ineffective_attribute, scaling_attribute] : std::views::zip(ineffective_attributes, scaling_attributes))
                {
                    if (ineffective_attribute && scaling_attribute != 0)
                    {
                        this->ineffective_attack_power_types[attack_power_type_integral] = true;
                        break;
                    }
                }
            }

            this->total_scalings[attack_power_type_integral] = this->calculate_total_scaling(
                this->ineffective_attack_power_types[attack_power_type_integral],
                is_damage_type ? adjusted_relevant_stats : relevant_stats,
                scaling_attributes,
                attribute_scalings_at_upgrade_level,
                scaling_curve
            );

            this->attack_powers[attack_power_type_integral] = {
                base_attack_power,
                base_attack_power * this->total_scalings[attack_power_type_integral]
            };
        }
    
    public:
        void calculate_attack_power_inplace(AttackPowerType attack_power_type)
        {
            auto&& weapon = this->weapon.get();

            auto upgrade_level = this->upgrade_level();
            auto relevant_stats = this->stats.relevant_stats();
            auto adjusted_relevant_stats = this->adjust_stats_for_two_handing();
            this->calculate_ineffective_attributes_inplace(adjusted_relevant_stats);

            this->calculate_attack_power_inplace_impl(
                attack_power_type,
                relevant_stats,
                adjusted_relevant_stats,
                this->ineffective_attributes,
                this->base_attack_powers(),
                this->attribute_scalings_at_upgrade_level()
            );
        }
        void calculate_spell_scaling_inplace()
        {
            auto&& weapon = this->weapon.get();

            if (weapon.is_sorcery_or_incantation_tool)
            {
                this->calculate_attack_power_inplace(AttackPowerType::PHYSICAL);
                this->spell_scaling = this->total_scalings[std::to_underlying(AttackPowerType::PHYSICAL)];
            }
            else
                this->spell_scaling = 0.;
        }
        void calculate_inplace()
        {
            auto&& weapon = this->weapon.get();

            auto upgrade_level = this->upgrade_level();
            auto relevant_stats = this->stats.relevant_stats();
            auto adjusted_relevant_stats = this->adjust_stats_for_two_handing();
            this->calculate_ineffective_attributes_inplace(adjusted_relevant_stats);
            auto&& attribute_scalings_at_upgrade_level = this->attribute_scalings_at_upgrade_level();
            auto&& base_attack_powers = this->base_attack_powers();

            for (auto attack_power_type : enumerators_of<AttackPowerType>())
                this->calculate_attack_power_inplace_impl(
                    attack_power_type,
                    relevant_stats,
                    adjusted_relevant_stats,
                    this->ineffective_attributes,
                    base_attack_powers,
                    attribute_scalings_at_upgrade_level
                );

            if (weapon.is_sorcery_or_incantation_tool)
                this->spell_scaling = this->total_scalings[std::to_underlying(AttackPowerType::PHYSICAL)];
            else
                this->spell_scaling = 0.;

            this->total_attack_power.fill(0.);
            for (auto&& attack_power : this->attack_powers | std::views::take(enumerators_of<DamageType>().size()))
            {
                this->total_attack_power[0] += attack_power[0];
                this->total_attack_power[1] += attack_power[1];
            }
        }

        bool is_total_attack_power_ineffective() const
        {
            for (auto&& ineffective_attack_power_type : this->ineffective_attack_power_types | std::views::take(enumerators_of<DamageType>().size()))
                if (ineffective_attack_power_type)
                    return true;
            return false;
        }
        bool is_spell_scaling_ineffective() const
        {
            AttackPowerType attack_power_type = AttackPowerType::PHYSICAL;
            if (weapon.get().sorcery_tool) 
                attack_power_type = AttackPowerType::MAGIC;
            else if (weapon.get().incantation_tool)
                attack_power_type = AttackPowerType::HOLY;
            
            return attack_power_type != AttackPowerType::PHYSICAL && this->ineffective_attack_power_types[std::to_underlying(attack_power_type)];
        }

        using FullAttackOptions::FullAttackOptions;

        static Attack calculate(const Weapon& weapon, const AttributeLevels& stats, const AttackOptions& attack_options)
        {
            Attack attack{ weapon, stats, attack_options };
            attack.calculate_inplace();
            return attack;
        }
    };
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
constexpr std::array<std::pair<calculator::Weapon::Type, std::string_view>, 43> enum_string_mapping<calculator::Weapon::Type> = {
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