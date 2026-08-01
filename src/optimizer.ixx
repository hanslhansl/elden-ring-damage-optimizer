export module erdo:optimizer;
import :calculator;

import std;
import BS.thread_pool;



namespace erdo::optimizer
{
    using namespace calculator;


    export namespace projections
    {
        using projection_type = double(const AttackRating&);

        constexpr auto spell_scaling(const AttackRating& attack_rating)
        {
            return attack_rating.spell_scaling;
        }

        constexpr auto total_attack_power(const AttackRating& attack_rating)
        {
            return attack_rating.total_attack_power[1];
        }

        template<AttackPowerType attack_power_type>
        constexpr auto attack_power(const AttackRating& attack_rating)
        {
            static constexpr auto attack_power_type_integral = std::to_underlying(attack_power_type);
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }
    }

    template<projections::projection_type projection>
    AttackRating optimize_weapon(const Weapon &weapon, const std::vector<FullStats>& stat_variations, const AttackOptions& attack_options)
    {
        auto attack_ratings_view = stat_variations
            | std::views::transform([&](const FullStats &full_stats) { return weapon.calculate_attack_rating(attack_options, full_stats); })
            | std::ranges::to<std::vector>();

        return std::ranges::max(attack_ratings_view, {}, projection);
    }

    export template<projections::projection_type projection, auto...th_flags>
    BS::multi_future<AttackRating> optimize(
        const std::vector<std::reference_wrapper<const calculator::Weapon>>& weapons,
        const std::vector<FullStats>& stat_variations,
        const AttackOptions& attack_options,
        BS::thread_pool<th_flags...>& pool
    )
    {
        if (std::ranges::empty(stat_variations))
            return {};

        auto do_weapon = [&](std::size_t i) {
            return optimize_weapon<projection>(weapons[i].get(), stat_variations, attack_options);
        };

        return pool.submit_sequence(0ull, weapons.size(), do_weapon);
    }
}