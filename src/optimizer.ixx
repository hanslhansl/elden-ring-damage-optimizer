export module erdo:optimizer;
import :calculator;

import std;
import BS.thread_pool;

export namespace optimizer
{
    using namespace calculator;

    struct OptimizationContext
    {
        std::vector<std::optional<AttackRating>> optional_results;
        BS::thread_pool<> pool;
        const std::vector<Weapon>& weapons;
        AttackOptions attack_options;

        OptimizationContext(int threads, const std::vector<Stats> &stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options)
            : optional_results{}, pool(threads), weapons(weapons), attack_options{ attack_options }
        {
            // create a vector of optional results for each weapon
            this->optional_results.resize(this->weapons.size());

            // process one weapon
            auto do_weapon = [&](std::size_t i) {
                auto &&weapon = this->weapons.at(i);

                auto &best_attack_rating = this->optional_results.at(i);

                // loop through all stat variations and find the one resulting in the best attack rating
                for (auto &&stats : stat_variations)
                {
                    auto intermediate_attack_rating = weapon.get_attack_rating(this->attack_options, stats);

                    if (!best_attack_rating || best_attack_rating->total_attack_power.at(2) < intermediate_attack_rating.total_attack_power.at(2))
                        best_attack_rating.emplace(std::move(intermediate_attack_rating));
                }

                std::println("completed {}: {}", i, weapon.full_name);
            };

            // loop through all weapons and get the best attack rating each asynchronously
            this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);
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
                [](const AttackRating& attack_rating) { return attack_rating.total_attack_power.at(2); }
            );
            return results;
        }
    };
}