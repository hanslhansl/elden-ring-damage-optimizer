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

    bool maximize_total_attack_power(const AttackRating& attack_rating) {
        return attack_rating.total_attack_power.at(2);
    }



    export
    // SortedMultiFuture<AttackRating, std::ranges::greater, bool(*)(const AttackRating&)>
    std::vector<AttackRating>
    optimize(const std::vector<Stats> &stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options, std::size_t threads = 0) {

        if (stat_variations.empty())
            return {};/*{
                {},
                {},
                maximize_total_attack_power
            };*/

        BS::thread_pool<> pool{ threads };

        // process one weapon
        auto do_weapon = [&](std::size_t i) {
            auto &&weapon = weapons.at(i);

            auto attack_ratings_view = stat_variations | std::views::transform([&](const Stats &stats) {
                return weapon.get_attack_rating(attack_options, stats);
            });

            return std::ranges::max(attack_ratings_view, std::ranges::greater{}, maximize_total_attack_power);
        };

        return SortedMultiFuture{
            pool.submit_sequence(0ull, weapons.size(), do_weapon),
            std::ranges::greater{},
            &maximize_total_attack_power
        }.get();
    }


    export struct OptimizationContext
    {
        std::vector<std::optional<AttackRating>> optional_results;
        BS::thread_pool<> pool;
        const std::vector<Weapon>& weapons;
        AttackOptions attack_options;

        OptimizationContext(int threads, const std::vector<Stats> &stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options)
            : optional_results{}, pool(threads), weapons(weapons), attack_options{ attack_options }
        {
            if (stat_variations.empty())
                return;

            // create a vector of optional results for each weapon
            this->optional_results.resize(this->weapons.size());

            // process one weapon
            auto do_weapon = [&](std::size_t i) {
                auto &&weapon = this->weapons.at(i);

                auto attack_ratings_view = stat_variations | std::views::transform([&](const Stats &stats) {
                    return weapon.get_attack_rating(this->attack_options, stats);
                });

                return std::ranges::max(attack_ratings_view, std::ranges::greater{}, maximize_total_attack_power);
            };

            // loop through all weapons and get the best attack rating each asynchronously
            // this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);

            auto res = this->pool.submit_sequence(0ull, this->weapons.size(), do_weapon);

            res.get();
        }

        std::vector<AttackRating> wait_and_get_result()
        {
            // wait for all threads to finish
            this->pool.wait();

            // extract results
            auto results = this->optional_results | std::views::join | std::ranges::to<std::vector>();

            // return sorted attack ratings
            std::ranges::sort(
                results,
                std::ranges::greater{},
                maximize_total_attack_power
            );
            return results;
        }
    };
}