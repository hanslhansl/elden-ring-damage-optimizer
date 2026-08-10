module;
#include <array>
export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export enum class Target
    {
        PHYSICAL_ATTACK_POWER = std::to_underlying(AttackPowerType::PHYSICAL),
        MAGIC_ATTACK_POWER = std::to_underlying(AttackPowerType::MAGIC),
        FIRE_ATTACK_POWER = std::to_underlying(AttackPowerType::FIRE),
        LIGHTNING_ATTACK_POWER = std::to_underlying(AttackPowerType::LIGHTNING),
        HOLY_ATTACK_POWER = std::to_underlying(AttackPowerType::HOLY),

        POISON_STATUS_EFFECT = std::to_underlying(AttackPowerType::POISON),
        SCARLET_ROT_STATUS_EFFECT = std::to_underlying(AttackPowerType::SCARLET_ROT),
        BLEED_STATUS_EFFECT = std::to_underlying(AttackPowerType::BLEED),
        FROST_STATUS_EFFECT = std::to_underlying(AttackPowerType::FROST),
        SLEEP_STATUS_EFFECT = std::to_underlying(AttackPowerType::SLEEP),
        MADNESS_STATUS_EFFECT = std::to_underlying(AttackPowerType::MADNESS),
        DEATH_BLIGHT_STATUS_EFFECT = std::to_underlying(AttackPowerType::DEATH_BLIGHT),

        TOTAL_ATTACK_POWER,
        SPELL_SCALING,
    };

    template<Target target>
    struct ProjectionImpl;
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target)))
    struct ProjectionImpl<target>
    {
        static constexpr auto attack_power_type = integral_to_enum<AttackPowerType>(std::to_underlying(target));
        static constexpr auto attack_power_type_integral = std::to_underlying(attack_power_type);

        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }
    };
    template<>
    struct ProjectionImpl<Target::SPELL_SCALING>
    {
        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.spell_scaling;
        }
    };
    template<>
    struct ProjectionImpl<Target::TOTAL_ATTACK_POWER>
    {
        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.total_attack_power[1];
        }
    };
    template<Target target>
    struct Projection
    {
        using value_type = std::remove_reference_t<std::invoke_result_t<decltype(ProjectionImpl<target>::operator()), AttackRating&>>;

        static value_type& operator()(AttackRating& attack_rating)
        {
            return ProjectionImpl<target>::operator()(attack_rating);
        }

        static const value_type& operator()(const AttackRating& attack_rating)
        {
            return ProjectionImpl<target>::operator()(const_cast<AttackRating&>(attack_rating));
        }
    };
    export template<Target target>
    constexpr Projection<target> projection{};

    template<Target target>
    struct BruteForceImpl
    {
        static void calculate_inplace(Attack& attack)
        {
            attack.calculate_inplace();
        }
    };
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target)))
    struct BruteForceImpl<target>
    {
        static constexpr auto attack_power_type = ProjectionImpl<target>::attack_power_type;
        static constexpr auto attack_power_type_integral = ProjectionImpl<target>::attack_power_type_integral;

        static void calculate_inplace(Attack& attack)
        {
            attack.calculate_attack_power_inplace(attack_power_type);
        }
    };
    template<>
    struct BruteForceImpl<Target::SPELL_SCALING>
    {
        static void calculate_inplace(Attack& attack)
        {
            attack.calculate_spell_scaling_inplace();
        }
    };
    template<Target target_>
    struct BruteForce
    {
        static constexpr auto target = target_;
        using BruteForceImpl = BruteForceImpl<target>;

        static Attack optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<Stats>& stat_variations)
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty");

            Attack attack{ weapon, {}, attack_options };
            Stats const* best_stats = nullptr;
            auto best_value = std::numeric_limits<typename Projection<target>::value_type>::lowest();

            for (const auto& stats : stat_variations)
            {
                attack.stats = stats;
                BruteForceImpl::calculate_inplace(attack);

                auto new_value = projection<target>(attack);
                if (best_value < new_value)
                {
                    best_stats = &stats;
                    best_value = new_value;
                }
            }

            attack.stats = *best_stats;
            attack.calculate_inplace();
            return attack;
        }

        static std::vector<std::function<Attack()>> get_tasks(
            const std::vector<std::reference_wrapper<const calculator::Weapon>>& weapons,
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty");

            std::vector<std::function<Attack()>> tasks;
            tasks.reserve(weapons.size());

            for (const auto& weapon : weapons)
            {
                tasks.emplace_back([&, weapon] {
                    return BruteForce::optimize_weapon(
                        weapon,
                        attack_options,
                        stat_variations
                    );
                });
            }

            return tasks;
        }

        static std::function<Attack(const Weapon&)> get_callback(
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty");

            return [&](const Weapon& weapon) {
                return BruteForce::optimize_weapon(
                    weapon,
                    attack_options,
                    stat_variations
                );
            };
        }

        static std::vector<Attack> run_synchronously(
            const std::vector<std::reference_wrapper<const Weapon>>& weapons,
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty");
            
            std::vector<Attack> attacks;
            attacks.reserve(weapons.size());
            attacks.append_range(
                weapons
                | std::views::transform([&](const calculator::Weapon& w) {
                    return BruteForce::optimize_weapon(
                        w,
                        attack_options,
                        stat_variations
                    );
                })
            );

            return attacks;
        }
    };
    export template<Target target>
    constexpr BruteForce<target> brute_force{};

    template<Target target>
    struct V2Impl
    {
        static void calculate_inplace(Attack& attack)
        {
            attack.calculate_inplace();
        }
    };
    template<Target target_>
    struct V2
    {
        static constexpr auto target = target_;
        using V2Impl = V2Impl<target>;

        static Attack optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<Stats>& stat_variations)
        {
            
        }

        static std::vector<std::function<Attack()>> get_tasks(
            const std::vector<std::reference_wrapper<const calculator::Weapon>>& weapons,
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            
        }

        static std::function<Attack(const Weapon&)> get_callback(
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
        
        }

        static std::vector<Attack> run_synchronously(
            const std::vector<std::reference_wrapper<const Weapon>>& weapons,
            const std::vector<Stats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            
        }
    };
    export template<Target target>
    constexpr V2<target> v2{};
}

using namespace erdo;
template<>
constexpr std::array<std::pair<optimizer::Target, std::string_view>, 14> enum_string_mapping<optimizer::Target> = {
    std::pair{optimizer::Target::PHYSICAL_ATTACK_POWER, "PHYSICAL_ATTACK_POWER"},
    std::pair{optimizer::Target::MAGIC_ATTACK_POWER, "MAGIC_ATTACK_POWER"},
    std::pair{optimizer::Target::FIRE_ATTACK_POWER, "FIRE_ATTACK_POWER"},
    std::pair{optimizer::Target::LIGHTNING_ATTACK_POWER, "LIGHTNING_ATTACK_POWER"},
    std::pair{optimizer::Target::HOLY_ATTACK_POWER, "HOLY_ATTACK_POWER"},
    std::pair{optimizer::Target::POISON_STATUS_EFFECT, "POISON_STATUS_EFFECT"},
    std::pair{optimizer::Target::SCARLET_ROT_STATUS_EFFECT, "SCARLET_ROT_STATUS_EFFECT"},
    std::pair{optimizer::Target::BLEED_STATUS_EFFECT, "BLEED_STATUS_EFFECT"},
    std::pair{optimizer::Target::FROST_STATUS_EFFECT, "FROST_STATUS_EFFECT"},
    std::pair{optimizer::Target::SLEEP_STATUS_EFFECT, "SLEEP_STATUS_EFFECT"},
    std::pair{optimizer::Target::MADNESS_STATUS_EFFECT, "MADNESS_STATUS_EFFECT"},
    std::pair{optimizer::Target::DEATH_BLIGHT_STATUS_EFFECT, "DEATH_BLIGHT_STATUS_EFFECT"},
    std::pair{optimizer::Target::TOTAL_ATTACK_POWER, "TOTAL_ATTACK_POWER"},
    std::pair{optimizer::Target::SPELL_SCALING, "SPELL_SCALING"},
};

