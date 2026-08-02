module;
#include <array>
export module erdo:optimizer;
import :calculator;

import std;
import BS.thread_pool;



namespace erdo::optimizer
{
    using namespace calculator;

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

    template<Target target_>
    struct Optimizer
    {
        static constexpr auto target = target_;

        static double projection(const AttackRating& attack_rating)
        {
            static_assert(is_valid_enum_integral<AttackPowerType>(std::to_underlying(target)), "target must be a valid AttackPowerType integral value");
            static constexpr auto attack_power_type_integral = std::to_underlying(target);
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }

        static AttackRating optimize_weapon(const Weapon &weapon, const std::vector<FullStats>& stat_variations, const AttackOptions& attack_options)
        {
            auto attack_ratings = stat_variations
                | std::views::transform([&](const FullStats &full_stats) { return weapon.calculate_attack_rating(attack_options, full_stats); })
                | std::ranges::to<std::vector>();

            return std::ranges::max(attack_ratings, {}, &Optimizer::projection);
        }

        static BS::multi_future<AttackRating> operator()(
            const std::vector<std::reference_wrapper<const calculator::Weapon>>& weapons,
            const std::vector<FullStats>& stat_variations,
            const AttackOptions& attack_options,
            BS::thread_pool<>& pool
        )
        {
            if (std::ranges::empty(stat_variations))
                return {};

            auto do_weapon = [&](std::size_t i) {
                return Optimizer::optimize_weapon(weapons[i].get(), stat_variations, attack_options);
            };

            return pool.submit_sequence(0ull, weapons.size(), do_weapon);
        }
    };

    template<>
    double Optimizer<Target::TOTAL_ATTACK_POWER>::projection(const AttackRating& attack_rating)
    {
        return attack_rating.total_attack_power[1];
    }

    template<>
    double Optimizer<Target::SPELL_SCALING>::projection(const AttackRating& attack_rating)
    {
        return attack_rating.spell_scaling;
    }
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

namespace erdo::optimizer
{
    export template<Target target>
    constexpr Optimizer<target> optimizers{};
}

