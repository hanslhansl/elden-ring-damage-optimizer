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

    struct OptimizerImplBase
    {
        // potentially provided more efficient implementation in the derived class
        static void efficient_calculate_inplace(AttackRating& attack_rating)
        {
            attack_rating.calculate_inplace();
        }

        // has to be provided by the derived class
        static double& projection(AttackRating& attack_rating);
    };

    template<OptimizationTarget target_>
    struct OptimizerImpl;

    template<OptimizationTarget target_> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target_)))
    struct OptimizerImpl<target_> : OptimizerImplBase
    {
        static constexpr auto attack_power_type = integral_to_enum<AttackPowerType>(std::to_underlying(target_));
        static constexpr auto attack_power_type_integral = std::to_underlying(attack_power_type);

        static void efficient_calculate_inplace(AttackRating& attack_rating)
        {
            attack_rating.calculate_attack_power_inplace(attack_power_type);
        }

        static double& projection(AttackRating& attack_rating)
        {
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }
    };

    template<>
    struct OptimizerImpl<OptimizationTarget::SPELL_SCALING> : OptimizerImplBase
    {
        static void efficient_calculate_inplace(AttackRating& attack_rating)
        {
            attack_rating.calculate_spell_scaling_inplace();
        }

        static double& projection(AttackRating& attack_rating)
        {
            return attack_rating.spell_scaling;
        }
    };

    template<>
    struct OptimizerImpl<OptimizationTarget::TOTAL_ATTACK_POWER> : OptimizerImplBase
    {
        static double& projection(AttackRating& attack_rating)
        {
            return attack_rating.total_attack_power[1];
        }
    };


    template<OptimizationTarget target_>
    struct Optimizer
    {
        static constexpr auto target = target_;
        using OptimizerImpl = OptimizerImpl<target_>;

        static AttackRating optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<FullStats>& stat_variations)
        {
            AttackRating attack_rating{ weapon, {}, attack_options };
            FullStats const* best_stats = nullptr;
            auto best_value = std::numeric_limits<typename Projection::value_type>::lowest();

            for (const auto& full_stats : stat_variations)
            {
                attack_rating.full_stats = full_stats;
                OptimizerImpl::efficient_calculate_inplace(attack_rating);

                if (best_value < OptimizerImpl::projection(attack_rating))
                {
                    best_stats = &full_stats;
                    best_value = OptimizerImpl::projection(attack_rating);
                }
            }

            attack_rating.full_stats = *best_stats;
            attack_rating.calculate_inplace();
            return attack_rating;
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
                tasks.emplace_back([&, weapon] {
                    return Optimizer::optimize_weapon(
                        weapon,
                        attack_options,
                        stat_variations
                    );
                });
            }

            return tasks;
        }

        static std::function<AttackRating(const Weapon&)> get_callback(
            const std::vector<FullStats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            return [&](const Weapon& weapon) {
                return Optimizer::optimize_weapon(
                    weapon,
                    attack_options,
                    stat_variations
                );
            };
        }

        static std::vector<AttackRating> run_synchronously(
            const std::vector<std::reference_wrapper<const Weapon>>& weapons,
            const std::vector<FullStats>& stat_variations,
            const AttackOptions& attack_options
        )
        {
            std::vector<AttackRating> attack_ratings;
            attack_ratings.reserve(weapons.size());
            attack_ratings.append_range(
                weapons
                | std::views::transform([&](const calculator::Weapon& w) {
                    return Optimizer::optimize_weapon(
                        w,
                        attack_options,
                        stat_variations
                    );
                })
            );

            return attack_ratings;
        }

        static constexpr struct Projection
        {
            using value_type = std::remove_reference_t<std::invoke_result_t<decltype(OptimizerImpl::projection), AttackRating&>>;

            static value_type& operator()(AttackRating& attack_rating)
            {
                return OptimizerImpl::projection(attack_rating);
            }

            static const value_type& operator()(const AttackRating& attack_rating)
            {
                return OptimizerImpl::projection(const_cast<AttackRating&>(attack_rating));
            }
        } projection{};
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

