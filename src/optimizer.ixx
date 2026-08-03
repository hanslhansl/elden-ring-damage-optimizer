module;
#include <array>
export module erdo:optimizer;
import :calculator;

import std;



namespace erdo::calculator
{
    export enum class OptimizationTarget
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

    template<OptimizationTarget target_>
    struct Optimizer;

    template<OptimizationTarget target_>
    struct OptimizerBase
    {
        static constexpr auto target = target_;

        static AttackRating optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<FullStats>& stat_variations)
        {
            AttackRating attack_rating{ weapon, {}, attack_options };
            auto attack_ratings = stat_variations
                | std::views::transform([&](const FullStats &full_stats) {
                    attack_rating.calculate_inplace(full_stats);
                    return attack_rating;
                })
                | std::ranges::to<std::vector>();

            return std::ranges::max(attack_ratings, {}, Optimizer<target_>::projection);
        }

        static std::vector<std::function<AttackRating()>> get_tasks(
            const std::vector<std::reference_wrapper<const calculator::Weapon>>& weapons,
            const std::vector<FullStats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            std::vector<std::function<AttackRating()>> tasks;
            tasks.reserve(weapons.size());

            for (const auto& weapon : weapons)
            {
                tasks.emplace_back([&] {
                    return Optimizer<target_>::optimize_weapon(
                        weapon,
                        attack_options,
                        stat_variations
                    );
                });
            }

            return tasks;
        }

        static std::function<AttackRating(const calculator::Weapon&)> get_callback(
            const std::vector<FullStats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            return [&](const calculator::Weapon& weapon) {
                return Optimizer<target_>::optimize_weapon(
                    weapon,
                    attack_options,
                    stat_variations
                );
            };
        }
    };

    template<OptimizationTarget target_> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target_)))
    struct Optimizer<target_> : OptimizerBase<target_>
    {
        static constexpr auto attack_power_type = integral_to_enum<AttackPowerType>(std::to_underlying(target_));
        static constexpr auto attack_power_type_integral = std::to_underlying(attack_power_type);

        static AttackRating optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<FullStats>& stat_variations)
        {
            AttackRating attack_rating{ weapon, {}, attack_options };
            auto attack_ratings = stat_variations
                | std::views::transform([&](const FullStats &full_stats) {
                    attack_rating.calculate_inplace(full_stats);
                    return attack_rating;
                })
                | std::ranges::to<std::vector>();

            return std::ranges::max(attack_ratings, {}, Optimizer<target_>::projection);
        }

        static double projection(const AttackRating& attack_rating)
        {
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }
    };

    template<>
    struct Optimizer<OptimizationTarget::TOTAL_ATTACK_POWER> : OptimizerBase<OptimizationTarget::TOTAL_ATTACK_POWER>
    {
        static double projection(const AttackRating& attack_rating)
        {
            return attack_rating.total_attack_power[1];
        }
    };

    template<>
    struct Optimizer<OptimizationTarget::SPELL_SCALING> : OptimizerBase<OptimizationTarget::SPELL_SCALING>
    {
        static double projection(const AttackRating& attack_rating)
        {
            return attack_rating.spell_scaling;
        }
    };

    export template<OptimizationTarget target>
    constexpr Optimizer<target> optimizers{};
}

using namespace erdo;
template<>
constexpr std::array<std::pair<calculator::OptimizationTarget, std::string_view>, 14> enum_string_mapping<calculator::OptimizationTarget> = {
    std::pair{calculator::OptimizationTarget::PHYSICAL_ATTACK_POWER, "PHYSICAL_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::MAGIC_ATTACK_POWER, "MAGIC_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::FIRE_ATTACK_POWER, "FIRE_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::LIGHTNING_ATTACK_POWER, "LIGHTNING_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::HOLY_ATTACK_POWER, "HOLY_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::POISON_STATUS_EFFECT, "POISON_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::SCARLET_ROT_STATUS_EFFECT, "SCARLET_ROT_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::BLEED_STATUS_EFFECT, "BLEED_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::FROST_STATUS_EFFECT, "FROST_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::SLEEP_STATUS_EFFECT, "SLEEP_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::MADNESS_STATUS_EFFECT, "MADNESS_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::DEATH_BLIGHT_STATUS_EFFECT, "DEATH_BLIGHT_STATUS_EFFECT"},
    std::pair{calculator::OptimizationTarget::TOTAL_ATTACK_POWER, "TOTAL_ATTACK_POWER"},
    std::pair{calculator::OptimizationTarget::SPELL_SCALING, "SPELL_SCALING"},
};

