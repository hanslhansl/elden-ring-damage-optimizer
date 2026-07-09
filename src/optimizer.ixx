export module erdo:optimizer;
import :calculator;

import std;
import BS.thread_pool;

export namespace optimizer
{
    using namespace calculator;

    struct OptimizationContext
    {
        std::vector<AttackRating::full> optional_results;
        BS::thread_pool<> pool;
        const std::vector<Weapon>& weapons;
        AttackOptions attack_options;

        OptimizationContext(int threads, const std::vector<Stats> &stat_variations, const std::vector<Weapon> &weapons, AttackOptions attack_options_)
            : optional_results{}, pool(threads), weapons(weapons), attack_options(attack_options_)
        {
            // create a vector of optional results for each weapon
            auto &optional_results = this->optional_results;
            optional_results.resize(this->weapons.size());

            // process one weapon
            auto do_weapon = [&](std::size_t i) {
                auto &&weapon = this->weapons.at(i);

                auto &best_attack_rating = optional_results[i];

                // loop through all stat variations and find the one resulting in the best attack rating
                for (auto &&stats : stat_variations)
                {
                    auto intermediate_attack_rating = weapon.get_attack_rating(this->attack_options, stats);

                    if (best_attack_rating.total_attack_power.at(2) < intermediate_attack_rating.total_attack_power.at(2))
                        best_attack_rating = std::move(intermediate_attack_rating);
                }

                std::println("completed {}: {}", i, weapon.full_name); };

            // loop through all weapons and get the best attack rating each asynchronously
            // for (std::size_t i = 0; i < this->weapons.size(); i++)
            //     do_weapon(i);
            this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);
        }

        AttackRating::full wait_and_get_result()
        {
            // wait for all threads to finish
            this->pool.wait();

            std::println();

            // loop through all optional attack ratings and get the best one
            auto loop_lambda = [](auto &vec) {
                auto sparse_result_it = std::ranges::max_element(
                    vec, {}, [](const auto &attack_rating) { return attack_rating.total_attack_power.at(2); });
                if (sparse_result_it == vec.end())
                    return std::pair{-1ll, Stats{}};
                auto index = std::distance(vec.begin(), sparse_result_it);
                return std::pair{index, sparse_result_it->stats};
            };

            auto [index, stats] = loop_lambda(this->optional_results);

            if (index == -1)
                return AttackRating::full{};

            return this->weapons.at(index).get_attack_rating(this->attack_options, stats);
        }
    };
}