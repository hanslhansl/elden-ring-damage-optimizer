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



    export
    SortedMultiFuture<AttackRating, std::ranges::greater, decltype(&total_attack_power_projection)>
    optimize(const std::vector<Stats> &stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options, std::size_t threads = 0) {

        if (stat_variations.empty())
            return {
                {},
                {},
                total_attack_power_projection
            };

        BS::thread_pool<> pool{ threads };

        // process one weapon
        auto do_weapon = [&](std::size_t i) {
            auto &&weapon = weapons.at(i);

            auto attack_ratings_view = stat_variations | std::views::transform([&](const Stats &stats) {
                return weapon.get_attack_rating(attack_options, stats);
            }) | std::ranges::to<std::vector>();

            return std::ranges::max(attack_ratings_view, {}, total_attack_power_projection);
        };

        return SortedMultiFuture{
            pool.submit_sequence(0ull, weapons.size(), do_weapon),
            std::ranges::greater{},
            &total_attack_power_projection
        };
    }
}