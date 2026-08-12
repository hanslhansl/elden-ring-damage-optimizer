export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export std::size_t get_stat_variation_count(const int free_attribute_points, const RelevantStats& min_relevant_stats, const RelevantStats& max_relevant_stats)
    {
        const auto A = free_attribute_points;
        const auto L = min_relevant_stats;
        const auto U = max_relevant_stats;
        constexpr auto P = min_relevant_stats.extent;

        // Restkapazitäten
        std::array<int, P> capacity{};

        int totalCapacity = 0;

        for (int i = 0; i < P; ++i)
        {
            if (L[i] > U[i])
                throw std::invalid_argument("L[i] must be less than U[i].");

            capacity[i] = U[i] - L[i];
            totalCapacity += capacity[i];
        }

        if (A > totalCapacity)
            return 0;

        // dp[a] = Anzahl Möglichkeiten, a Äpfel
        // auf die bisher betrachteten Personen zu verteilen.
        std::vector<int> dp(A + 1, 0);
        std::vector<int> next(A + 1, 0);

        dp[0] = 1;

        for (int C : capacity)
        {
            int window = 0;

            for (int a = 0; a <= A; ++a)
            {
                // dp[a] + dp[a-1] + ... + dp[a-C]
                window += dp[a];

                if (a - C - 1 >= 0)
                    window -= dp[a - C - 1];

                next[a] = window;
            }

            dp.swap(next);
        }

        return dp[A];
    }
    export std::vector<Stats> get_stat_variations(const int free_attribute_points, const Stats &min_stats, const RelevantStats& max_relevant_stats)
    {
        auto max_attribute_points = free_attribute_points + min_stats.attribute_points();
        const auto SUM = max_attribute_points - std::ranges::fold_left(min_stats.irrelevant_stats(), 0, std::plus<int>{});
        auto min_relevant_stats = min_stats.relevant_stats();

        auto possible_occurances = get_stat_variation_count(free_attribute_points, min_relevant_stats, max_relevant_stats);
        if (possible_occurances == 0)
            return {};

        std::vector<Stats> stat_variations{ possible_occurances };
        auto current_it = stat_variations.begin(); 

        auto result = min_stats;
        auto& i = result[irrelevant_attribute_count];
        auto& j = result[irrelevant_attribute_count + 1];
        auto& k = result[irrelevant_attribute_count + 2];
        auto& l = result[irrelevant_attribute_count + 3];
        auto& m = result[irrelevant_attribute_count + 4];

        for (i = min_relevant_stats[0]; std::cmp_less_equal(i, std::min<int>(max_relevant_stats[0], SUM)); ++i)
        {
            auto SUM_i = SUM - (int)i;
            for (j = min_relevant_stats[1]; std::cmp_less_equal(j, std::min<int>(max_relevant_stats[1], SUM_i)); ++j)
            {
                auto SUM_i_j = SUM_i - (int)j;
                for (k = min_relevant_stats[2]; std::cmp_less_equal(k, std::min<int>(max_relevant_stats[2], SUM_i_j)); ++k)
                {
                    auto SUM_i_j_k = SUM_i_j - (int)k;
                    for (l = std::max<int>(min_relevant_stats[3], SUM_i_j_k - max_relevant_stats[4]);
                        std::cmp_less_equal(l, std::min<int>(max_relevant_stats[3], SUM_i_j_k - min_relevant_stats[4]));
                        ++l
                    )
                        *current_it++ = result;
                }
            }
        }

        if (current_it != stat_variations.end())
            throw std::runtime_error(std::format(
                "Mismatch in expected ({}) and actual ({}) number of stat variations generated.\nmin_stats: {}, free_attribute_points: {}",
                possible_occurances,
                std::distance(stat_variations.begin(), current_it),
                min_stats,
                free_attribute_points
            ));

        return stat_variations;
    }

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

    template<Target target>
    struct ProjectionImpl;
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target)))
    struct ProjectionImpl<target>
    {
        static constexpr auto attack_power_type = integral_to_enum<AttackPowerType>(std::to_underlying(target));
        static constexpr auto attack_power_type_integral = std::to_underlying(attack_power_type);

        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.attack_powers[attack_power_type_integral][1];
        }
    };
    template<>
    struct ProjectionImpl<Target::SPELL_SCALING>
    {
        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.spell_scaling;
        }
    };
    template<>
    struct ProjectionImpl<Target::TOTAL_ATTACK_POWER>
    {
        static double& operator()(AttackRating& attack_rating)
        {
            return attack_rating.total_attack_power[1];
        }
    };
    template<Target target>
    struct Projection
    {
        using value_type = std::remove_reference_t<std::invoke_result_t<decltype(ProjectionImpl<target>::operator()), AttackRating&>>;

        static value_type& operator()(AttackRating& attack_rating)
        {
            return ProjectionImpl<target>::operator()(attack_rating);
        }

        static const value_type& operator()(const AttackRating& attack_rating)
        {
            return ProjectionImpl<target>::operator()(const_cast<AttackRating&>(attack_rating));
        }
    };
    export template<Target target>
    constexpr Projection<target> projection{};

    template<Target target>
    struct EfficientCalculateAttack
    {
        static void operator()(Attack& attack)
        {
            attack.calculate_inplace();
        }
    };
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target)))
    struct EfficientCalculateAttack<target>
    {
        static void operator()(Attack& attack)
        {
            attack.calculate_attack_power_inplace(ProjectionImpl<target>::attack_power_type);
        }
    };
    template<>
    struct EfficientCalculateAttack<Target::SPELL_SCALING>
    {
        static void operator()(Attack& attack)
        {
            attack.calculate_spell_scaling_inplace();
        }
    };
    template<Target target>
    constexpr EfficientCalculateAttack<target> efficient_calculate_attack{};

    template<template <Target> typename O, Target target_>
    struct OptimizerBase
    {
        static constexpr auto target = target_;
        static constexpr auto projection = optimizer::projection<target>;
        static constexpr auto efficient_calculate_attack = optimizer::efficient_calculate_attack<target>;
        using Optimizer = O<target>;

        static Attack optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<Stats>& stat_variations)
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty.");

            Attack attack{ weapon, {}, attack_options };
            Stats const* best_stats = nullptr;
            auto best_value = std::numeric_limits<typename Projection<target>::value_type>::lowest();

            for (const auto& stats : stat_variations)
            {
                attack.stats = stats;
                efficient_calculate_attack(attack);

                auto new_value = projection(attack);
                if (best_value < new_value)
                {
                    best_stats = &stats;
                    best_value = new_value;
                }
            }

            attack.stats = *best_stats;
            attack.calculate_inplace();

            return attack;
        }
    
        static std::vector<Attack> run_synchronously(
            const std::ranges::sized_range auto& weapons,
            const AttackOptions& attack_options,
            const int& free_attribute_points,
            const Stats &min_stats,
            const int& max_stat
        )
        {
            return Optimizer::get_tasks(weapons, attack_options, free_attribute_points, min_stats, max_stat)
                | std::views::transform([](const auto& task) { return task(); })
                | std::ranges::to<std::vector>();
        }
    };

    export template<Target target>
    struct BruteForce : OptimizerBase<BruteForce, target>
    {
        static auto get_tasks(
            const std::ranges::sized_range auto& weapons,
            const AttackOptions& attack_options,
            const int& free_attribute_points,
            const Stats &min_stats,
            const int& max_stat
        )
        {
            auto stat_variations = std::make_shared<std::vector<Stats>>(
                get_stat_variations(free_attribute_points, min_stats, make_filled_array<RelevantStats>(max_stat))
            );

            auto lambda = [&](){
                return weapons
                    | std::views::transform([&](const Weapon& w){
                        return [&, stat_variations] {
                            return BruteForce::optimize_weapon(w, attack_options, *stat_variations);
                        };
                    })
                    | std::ranges::to<std::vector>();
            };

            if (stat_variations->empty())
                return decltype(lambda()){};
            return lambda();
        }
    };

    export template<Target target>
    struct V2 : OptimizerBase<V2, target>
    {
        static RelevantStatsArray get_optimized_max_relevant_stats(
            const int free_attribute_points,
            const RelevantStats &min_relevant_stats,
            const RelevantStats& max_relevant_stats,
            const NonscalingAttributes& nonscaling_attributes
        )
        {
            RelevantStatsArray optimized_max_relevant_stats{};
            for (auto&& [min_relevant_stat, max_relevant_stat, nonscaling_attribute, optimized_max_relevant_stat] :
                std::views::zip(min_relevant_stats, max_relevant_stats, nonscaling_attributes, optimized_max_relevant_stats)
            )
                optimized_max_relevant_stat = nonscaling_attribute ? min_relevant_stat : max_relevant_stat;
            return optimized_max_relevant_stats;
        }

        static std::vector<Stats> get_optimized_stat_variations(
            const int free_attribute_points,
            const Stats &min_stats,
            const RelevantStats& max_relevant_stats,
            const NonscalingAttributes& nonscaling_attributes
        )
        {
            auto optimized_max_relevant_stats = V2::get_optimized_max_relevant_stats(
                free_attribute_points, min_stats.relevant_stats(), max_relevant_stats, nonscaling_attributes
            );

            auto optimized_stat_variations = get_stat_variations(free_attribute_points, min_stats, optimized_max_relevant_stats);
            if (optimized_stat_variations.empty())
                optimized_stat_variations.emplace_back(min_stats);
            return optimized_stat_variations;
        }

        static auto get_tasks(
            const std::ranges::sized_range auto& weapons,
            const AttackOptions& attack_options,
            const int& free_attribute_points,
            const Stats &min_stats,
            const int& max_stat
        )
        {
            auto min_relevant_stats = min_stats.relevant_stats();
            auto max_relevant_stats = make_filled_array<RelevantStats>(max_stat);
            auto optimized_stat_variations_map = std::make_shared<std::map<NonscalingAttributes, std::vector<Stats>>>();
            
            // std::size_t total_stat_variation_count = 0;

            auto lambda = [&](){
                return weapons
                    | std::views::transform([&](const calculator::Weapon& w) {
                        auto [it, inserted] = optimized_stat_variations_map->try_emplace(w.nonscaling_attributes);
                        auto&& optimized_stat_variations = it->second;
                        if (inserted)
                            optimized_stat_variations = V2::get_optimized_stat_variations(free_attribute_points, min_stats, max_relevant_stats, w.nonscaling_attributes);
                        // total_stat_variation_count += optimized_stat_variations.size();

                        // if (w.full_name == "Fire Duelist Greataxe")
                        // {
                        //     RelevantStatsArray optimized_max_relevant_stats{};
                        //     for (auto&& [min_relevant_stat, max_relevant_stat, nonscaling_attribute, optimized_max_relevant_stat] :
                        //         std::views::zip(min_stats.relevant_stats(), max_relevant_stats, w.nonscaling_attributes, optimized_max_relevant_stats)
                        //     )
                        //         optimized_max_relevant_stat = nonscaling_attribute ? min_relevant_stat : max_relevant_stat;
                        //     std::println("Fire Duelist Greataxe");
                        //     std::println("min_stats: {}", min_stats);
                        //     std::println("max_relevant_stats: {}", max_relevant_stats);
                        //     std::println("optimized_max_relevant_stats: {}", optimized_max_relevant_stats);
                        //     std::println("stat variation count: {}", optimized_stat_variations.size());
                        //     std::println("stat_variations:");
                        //     for (const auto& x : optimized_stat_variations)
                        //         std::println("{}", x);
                        // }

                        return [&, optimized_stat_variations_map](){
                            return V2::optimize_weapon(w, attack_options, optimized_stat_variations);
                        };
                    })
                    | std::ranges::to<std::vector>();
            };

            if (get_stat_variation_count(free_attribute_points, min_relevant_stats, max_relevant_stats) == 0)
                return decltype(lambda()){};
            // std::println("average stat variation count: {}", (double)total_stat_variation_count / (double)weapons.size());
            return lambda();
        }
    };
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