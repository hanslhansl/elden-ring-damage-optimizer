module;
#include <array>
#include <optional>
export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export std::size_t get_stat_variation_count(const std::size_t free_attribute_points, const RelevantStats min_relevant_stats)
    {
        const auto A = free_attribute_points;
        const auto T = min_relevant_stats;
        constexpr std::size_t M = 99;
        constexpr auto P = decltype(min_relevant_stats)::extent;

        // Restkapazitäten
        std::array<decltype(min_relevant_stats)::value_type, P> capacity{};

        std::size_t totalCapacity = 0;

        for (std::size_t i = 0; i < P; ++i)
        {
            if (T[i] < 0 || T[i] > M)
            {
                throw std::invalid_argument("T[i] muss zwischen 0 und M liegen.");
            }

            capacity[i] = M - T[i];
            totalCapacity += capacity[i];
        }

        if (A < 0)
        {
            throw std::invalid_argument("A darf nicht negativ sein.");
        }

        if (A > totalCapacity)
        {
            return 0;
        }

        // dp[a] = Anzahl Möglichkeiten, a Äpfel
        // auf die bisher betrachteten Personen zu verteilen.
        std::vector<std::size_t> dp(A + 1, 0);
        std::vector<std::size_t> next(A + 1, 0);

        dp[0] = 1;

        for (auto C : capacity)
        {

            long long window = 0;

            for (long long a = 0; a <= A; ++a)
            {

                // dp[a] + dp[a-1] + ... + dp[a-C]
                window += dp[a];

                auto dfhdf = a - C - 1;
                if (a - C - 1 >= 0)
                {
                    window -= dp[a - C - 1];
                }

                next[a] = window;
            }

            dp.swap(next);
        }

        return dp[A];
    }
    export std::vector<Stats> get_stat_variations(const std::size_t free_attribute_points, const Stats &min_stats)
    {
        constexpr std::size_t UPPER = 99;
        auto max_attribute_points = free_attribute_points + std::ranges::fold_left(min_stats, 0, std::plus<>{});
        const auto SUM = max_attribute_points - std::ranges::fold_left(min_stats.irrelevant_stats(), 0, std::plus<>{});
        auto min_relevant_stats = min_stats.relevant_stats();

        auto possible_occurances = get_stat_variation_count(free_attribute_points, min_relevant_stats);
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

        for (i = min_relevant_stats[0]; i <= std::min(UPPER, SUM); ++i)
        {
            auto SUM_i = SUM - i;
            for (j = min_relevant_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
            {
                auto SUM_i_j = SUM_i - j;
                for (k = min_relevant_stats[2]; k <= std::min(UPPER, SUM_i_j); ++k)
                {
                    auto SUM_i_j_k = SUM_i_j - k;
                    for (l = min_relevant_stats[3]; l <= std::min(UPPER, SUM_i_j_k); ++l)
                    {
                        auto SUM_i_j_k_l = SUM_i_j_k - l;
                        m = SUM_i_j_k_l;
                        if (min_relevant_stats[4] <= m && m <= UPPER)
                        {
                            *current_it++ = result;
                        }
                    }
                }
            }
        }

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
                throw std::invalid_argument("stat_variations must not be empty");

            std::optional<Attack> result{ { weapon, {}, attack_options } };
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
    
        static auto get_tasks(const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int max_attribute_points)
        {
            auto view = Optimizer::get_task_view(weapons, attack_options, min_stats, max_attribute_points);

            static_assert(std::ranges::sized_range<decltype(*view)>);

            if (view)
                return *view | std::ranges::to<std::vector>();
            return decltype(*view | std::ranges::to<std::vector>()){};
        }
    
        static std::vector<Attack> run_synchronously(
            const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int max_attribute_points
        )
        {
            return Optimizer::get_task_view(weapons, attack_options, min_stats, max_attribute_points)
                | std::views::join
                | std::views::transform([](const auto& task) { return task(); })
                | std::ranges::to<std::vector>();
        }
    };

    export template<Target target>
    struct BruteForce : OptimizerBase<BruteForce, target>
    {
        static auto get_task_view(const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int free_attribute_points)
        {
            auto stat_variations = std::make_shared<std::vector<Stats>>(get_stat_variations(free_attribute_points, min_stats));

            auto view = weapons | std::views::transform([&, stat_variations=stat_variations](const Weapon& w){
                return [&, stat_variations] {
                    return BruteForce::optimize_weapon(
                        w,
                        attack_options,
                        *stat_variations
                    );
                };
            });

            if(!stat_variations->empty())
                return std::optional(std::move(view));
            return decltype(std::optional(std::move(view))){std::nullopt};
        }
    };

    export template<Target target>
    struct V2 : OptimizerBase<V2, target>
    {
        static std::vector<Stats> get_stat_variations(const Stats &min_stats, const int free_attribute_points, const NonscalingAttributes& nonscaling_attributes)
        {
            return optimizer::get_stat_variations(free_attribute_points, min_stats);
        }

        static auto get_task_view(const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int free_attribute_points)
        {
            auto stat_variations_map = std::make_shared<std::map<NonscalingAttributes, std::vector<Stats>>>();

            auto view = weapons | std::views::transform([&, stat_variations_map](const calculator::Weapon& w) {
                auto [it, inserted] = stat_variations_map->try_emplace(w.nonscaling_attributes);
                auto&& stat_variations = it->second;
                if (inserted)
                    stat_variations = V2::get_stat_variations(min_stats, free_attribute_points, w.nonscaling_attributes);

                return [&, stat_variations_map](){
                    return V2::optimize_weapon(
                        w,
                        attack_options,
                        stat_variations
                    );
                };
            });

            if(get_stat_variation_count(free_attribute_points, min_stats.relevant_stats()) > 0)
                return std::optional(std::move(view));
            return decltype(std::optional(std::move(view))){std::nullopt};
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