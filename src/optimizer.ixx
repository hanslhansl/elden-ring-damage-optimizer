export module erdo:optimizer;

import :calculator;
import std;

export namespace optimizer
{
    using namespace calculator;

    struct OptimizationContext
    {
        using var_vec = std::variant<std::vector<AttackRating::total>, std::vector<AttackRating::physical>, std::vector<AttackRating::magic>, std::vector<AttackRating::fire>, std::vector<AttackRating::lightning>, std::vector<AttackRating::holy>, std::vector<AttackRating::poison_status>, std::vector<AttackRating::scarlet_rot_status>, std::vector<AttackRating::bleed_status>, std::vector<AttackRating::frost_status>, std::vector<AttackRating::sleep_status>, std::vector<AttackRating::madness_status>, std::vector<AttackRating::death_blight_status>, std::vector<AttackRating::spell_scaling>>;

        var_vec optional_results;
        // BS::thread_pool<> pool;
        const std::vector<const Weapon *> &weapons;
        AttackOptions attack_options;

        template <typename T>
            requires requires(T t) { t.value(); }
        OptimizationContext(int threads, const std::vector<Stats> &stat_variations, const std::vector<const Weapon *> &filtered_weapons_, AttackOptions attack_options_, std::type_identity<T>)
            : optional_results{}, /*pool(threads),*/ weapons(filtered_weapons_), attack_options(attack_options_)
        {
            // create a vector of optional results for each weapon
            auto &optional_results = this->optional_results.emplace<std::vector<T>>();
            optional_results.resize(this->weapons.size());

            // process one weapon
            auto do_weapon = [&](std::size_t i) {
                auto &weapon = *this->weapons.at(i);

                T intermediate_attack_rating{};
                T &best_attack_rating = optional_results[i];

                // loop through all stat variations and find the one resulting
                // in the best attack rating
                for (auto &&stats : stat_variations)
                {
                    weapon.get_attack_rating(this->attack_options, stats, intermediate_attack_rating);

                    if (best_attack_rating.value() < intermediate_attack_rating.value())
                        best_attack_rating = std::move(intermediate_attack_rating);
                }

                std::println("completed {}: {}", i, weapon.full_name); };

            // loop through all weapons and get the best attack rating each
            // asynchronously
            for (std::size_t i = 0; i < this->weapons.size(); i++)
                do_weapon(i);
            // this->pool.detach_sequence(0ull, this->weapons.size(), do_weapon);
        }

        AttackRating::full wait_and_get_result()
        {
            // wait for all threads to finish
            // this->pool.wait();

            std::println();

            // loop through all optional attack ratings and get the best one
            auto loop_lambda = [](auto &vec) {
                auto sparse_result_it = std::ranges::max_element(
                    vec, {}, &std::remove_reference_t<decltype(vec)>::value_type::value);
                if (sparse_result_it == vec.end())
                    return std::pair{-1ll, Stats{}};
                auto index = std::distance(vec.begin(), sparse_result_it);
                return std::pair{index, sparse_result_it->stats}; };

            auto [index, stats] = std::visit(loop_lambda, this->optional_results);

            if (index == -1)
                return AttackRating::full{};

            AttackRating::full best_attack_rating{};
            this->weapons[index]->get_attack_rating(this->attack_options, stats, best_attack_rating);
            return best_attack_rating;
        }
    };
}