export module erdo:optimizer;
import :calculator;

import std;
import BS.thread_pool;



namespace optimizer
{
    using namespace calculator;

    template<typename T, class Comp = std::ranges::less, class Proj = std::identity>
    class SortedMultiFuture
    {
        BS::multi_future<T> mf;
        Comp comp;
        Proj proj;

    public:
        SortedMultiFuture(BS::multi_future<T>&& mf, Comp comp = {}, Proj proj = {}) : mf(std::move(mf)), comp(std::move(comp)), proj(std::move(proj)) { }

        std::vector<T> get() {
            auto results = this->mf.get();
            std::ranges::sort(results, this->comp, this->proj);
            return results;
        }
    };

    export auto total_attack_power_projection(const AttackRating& attack_rating) {
        return attack_rating.total_attack_power.at(2);
    }


    export AttackRating optimize_weapon(const Weapon &weapon, const std::vector<Stats>& stat_variations, const AttackOptions& attack_options) {
        auto attack_ratings_view = stat_variations | std::views::transform([&](const Stats &stats) {
                return weapon.get_attack_rating(attack_options, stats);
            }) | std::ranges::to<std::vector>();

        return std::ranges::max(attack_ratings_view, {}, total_attack_power_projection);
    }

    export
    template<auto optimize_weapon = optimize_weapon, auto...th_flags>
    SortedMultiFuture<AttackRating, std::ranges::greater, decltype(&total_attack_power_projection)>
    optimize(const std::vector<Weapon> &weapons, const std::vector<Stats>& stat_variations, AttackOptions attack_options, BS::thread_pool<th_flags...>& pool) {
        if (std::ranges::empty(stat_variations))
            return {
                {},
                {},
                total_attack_power_projection
            };

        auto do_weapon = [&](std::size_t i) {
            return optimize_weapon(weapons.at(i), stat_variations, attack_options);
        };

        return SortedMultiFuture{
            pool.submit_sequence(0ull, weapons.size(), do_weapon),
            std::ranges::greater{},
            &total_attack_power_projection
        };
    }

    export
    template<typename R, auto...th_flags>
    // requires std::ranges::input_range<R> && std::same_as<std::ranges::range_value_t<R>, Stats>
    SortedMultiFuture<AttackRating, std::ranges::greater, decltype(&total_attack_power_projection)>
    optimize_range(R&& stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options, BS::thread_pool<th_flags...>& pool) {

        // if (std::ranges::empty(stat_variations))
        //     return {
        //         {},
        //         {},
        //         total_attack_power_projection
        //     };

        // process one weapon
        auto do_weapon = [&](std::size_t i) {
            auto &&weapon = weapons.at(i);

            auto attack_ratings_view = stat_variations | std::views::transform([&](const Stats &stats) {
                return weapon.get_attack_rating(attack_options, stats);
            });

            auto max_element = std::ranges::max_element(attack_ratings_view, {}, total_attack_power_projection);

            if (max_element == std::ranges::end(attack_ratings_view))
                return std::optional<AttackRating>{};

            return std::optional<AttackRating>(*max_element);
        };

        return SortedMultiFuture{
            pool.submit_sequence(0ull, weapons.size(), do_weapon),
            std::ranges::greater{},
            &total_attack_power_projection
        };
    }
}