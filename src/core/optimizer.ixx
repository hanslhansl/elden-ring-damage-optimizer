module;
#include <array>
export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export constexpr std::size_t get_stat_variation_count(const Stats &min_stats, const int max_attribute_points)
    {
        constexpr auto UPPER = 99;
        const auto SUM = max_attribute_points - std::ranges::fold_left(min_stats.irrelevant_stats(), 0, std::plus<>{});
        std::size_t count = 0;

        if (max_attribute_points > UPPER * min_stats.size())
            throw std::invalid_argument(std::format("max_attribute_points must be <= {}", UPPER * min_stats.size()));

        if (std::ranges::any_of(min_stats, [](auto v) { return v > UPPER; }))
            throw std::invalid_argument(std::format("min_stats must be <= {}", UPPER));

        auto min_relevant_stats = min_stats.relevant_stats();

        for (auto i = min_relevant_stats[0]; i <= std::min(UPPER, SUM); ++i)
        {
            auto SUM_i = SUM - i;
            for (auto j = min_relevant_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
            {
                auto SUM_i_j = SUM_i - j;

                if (0ll == min_relevant_stats[4])
                {
                    auto a1 = std::max(min_relevant_stats[2], SUM_i_j - min_relevant_stats[3] - UPPER);
                    auto b1 = std::min(UPPER, SUM_i_j - min_relevant_stats[3]);
                    auto b1_a1_1 = b1 - a1 + 1;
                    if (b1_a1_1 > 0)
                        count += (1 - min_relevant_stats[3]) * b1_a1_1;

                    auto a2 = a1;
                    auto b2 = std::min(UPPER, SUM_i_j - UPPER - 1);
                    auto b2_a2_1 = b2 - a2 + 1;
                    if (b2_a2_1 > 0)
                        count += UPPER * b2_a2_1;

                    auto a3 = std::max(min_relevant_stats[2], SUM_i_j - UPPER);
                    auto b3 = b1;
                    auto b3_a3_1 = b3 - a3 + 1;
                    if (b3_a3_1 > 0)
                        count += SUM_i_j * b3_a3_1 - (a3 + b3) * b3_a3_1 / 2;

                    auto a4 = std::max(min_relevant_stats[2], SUM_i_j - UPPER - UPPER);
                    auto b4 = std::min(UPPER, SUM_i_j - min_relevant_stats[3] - UPPER - 1);
                    auto b4_a4_1 = b4 - a4 + 1;
                    if (b4_a4_1 > 0)
                        count += (UPPER + 1 - SUM_i_j + UPPER) * b4_a4_1 + (a4 + b4) * b4_a4_1 / 2;

                    auto a5 = std::max(min_relevant_stats[2], SUM_i_j - UPPER);
                    auto b5 = b4;
                    auto b5_a5_1 = b5 - a5 + 1;
                    if (b5_a5_1 > 0)
                        count += SUM_i_j * b5_a5_1;
                }
                else
                {
                    auto a2 = std::max(min_relevant_stats[2], SUM_i_j - min_relevant_stats[3] - UPPER);
                    auto b2 = std::min({UPPER, SUM_i_j - UPPER - min_relevant_stats[4], SUM_i_j - UPPER - 1});
                    auto b2_a2_1 = b2 - a2 + 1;
                    if (b2_a2_1 > 0)
                        count += (1 + UPPER - min_relevant_stats[3]) * b2_a2_1;

                    auto a3 = std::max(min_relevant_stats[2], SUM_i_j - UPPER);
                    auto b3 = std::min(UPPER, SUM_i_j - UPPER - min_relevant_stats[4]);
                    auto b3_a3_1 = b3 - a3 + 1;
                    if (b3_a3_1 > 0)
                        count += (1 + SUM_i_j - min_relevant_stats[3]) * b3_a3_1 - (a3 + b3) * b3_a3_1 / 1;

                    auto a4 = std::max({min_relevant_stats[2], SUM_i_j - min_relevant_stats[3] - UPPER, SUM_i_j - min_relevant_stats[4] - UPPER + 1});
                    auto b4 = std::min(UPPER, SUM_i_j - min_relevant_stats[4] - min_relevant_stats[3]);
                    auto b4_a4_1 = b4 - a4 + 1;
                    if (b4_a4_1 > 0)
                        count += (SUM_i_j - min_relevant_stats[4] - min_relevant_stats[3] + 1) * b4_a4_1 - (a4 + b4) * b4_a4_1 / 2;

                    auto a5 = std::max(min_relevant_stats[2], SUM_i_j - UPPER - UPPER);
                    auto b5 = std::min({UPPER, SUM_i_j - UPPER - 1 - min_relevant_stats[3], SUM_i_j - min_relevant_stats[4] - UPPER});
                    auto b5_a5_1 = b5 - a5 + 1;
                    if (b5_a5_1 > 0)
                        count += (UPPER - SUM_i_j + UPPER + 1) * b5_a5_1 + (a5 + b5) * b5_a5_1 / 2;

                    auto a7 = std::max(min_relevant_stats[2], SUM_i_j - UPPER);
                    auto b7 = b5;
                    auto b7_a7_1 = b7 - a7 + 1;
                    if (b7_a7_1 > 0)
                        count += (UPPER + 1) * b7_a7_1;

                    auto a8 = std::max(min_relevant_stats[2], SUM_i_j - UPPER - min_relevant_stats[4] + 1);
                    auto b8 = std::min(UPPER, SUM_i_j - min_relevant_stats[3] - UPPER - 1);
                    auto b8_a8_1 = b8 - a8 + 1;
                    if (b8_a8_1 > 0)
                        count += (UPPER - min_relevant_stats[4] + 1) * b8_a8_1;
                }
            }
        }

        return count;
    }
    export std::vector<Stats> get_stat_variations(const Stats &min_stats, const int max_attribute_points)
    {
        constexpr auto UPPER = 99;
        const auto SUM = max_attribute_points - std::ranges::fold_left(min_stats.irrelevant_stats(), 0, std::plus<>{});

        auto possible_occurances = get_stat_variation_count(min_stats, max_attribute_points);
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

        auto min_relevant_stats = min_stats.relevant_stats();
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

    template<Target target_>
    struct BruteForce
    {
        static constexpr auto target = target_;
        
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
                efficient_calculate_attack<target>(attack);

                auto new_value = projection<target>(attack);
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
            auto stat_variations = std::make_shared<std::vector<Stats>>(get_stat_variations(min_stats, max_attribute_points));
            auto enable = !stat_variations->empty();

            auto view = weapons | std::views::transform([&, stat_variations=std::move(stat_variations)](const Weapon& w){
                return [&, stat_variations] {
                    return BruteForce::optimize_weapon(
                        w,
                        attack_options,
                        *stat_variations
                    );
                };
            });

            decltype(view | std::ranges::to<std::vector>()) tasks{};

            if(enable)
            {
                tasks.reserve(std::ranges::size(weapons));
                tasks.append_range(view);
            }

            return tasks;
        }

        static std::vector<Attack> run_synchronously(
            const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int max_attribute_points
        )
        {
            std::vector<Attack> attacks;
            attacks.reserve(std::ranges::size(weapons));
            attacks.append_range(
                BruteForce::get_tasks(weapons, attack_options, min_stats, max_attribute_points)
                    | std::views::transform([](const auto& task) { return task(); })
            );
            return attacks;
        }
    };
    export template<Target target>
    constexpr BruteForce<target> brute_force{};

    template<Target target_>
    struct V2
    {
        static constexpr auto target = target_;

        static std::vector<Stats> get_stat_variations(const Stats &min_stats, const int max_attribute_points, const NonscalingAttributes& nonscaling_attributes)
        {
            return optimizer::get_stat_variations(min_stats, max_attribute_points);
        }

        static Attack optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<Stats>& stat_variations)
        {
            return BruteForce<target>::optimize_weapon(weapon, attack_options, stat_variations);
        }

        static auto get_tasks(const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int max_attribute_points)
        {
            auto stat_variations_map = std::make_shared<std::map<NonscalingAttributes, std::vector<Stats>>>();
            auto enable = get_stat_variation_count(min_stats, max_attribute_points) > 0;

            auto view = weapons | std::views::transform([&, stat_variations_map=std::move(stat_variations_map)](const calculator::Weapon& w) {
                auto [it, inserted] = stat_variations_map->try_emplace(w.nonscaling_attributes);
                auto&& stat_variations = it->second;
                if (inserted)
                    stat_variations = V2::get_stat_variations(min_stats, max_attribute_points, w.nonscaling_attributes);

                return [&, stat_variations_map](){
                    return V2::optimize_weapon(
                        w,
                        attack_options,
                        stat_variations
                    );
                };
            });

            decltype(view | std::ranges::to<std::vector>()) tasks{};

            if(enable)
            {
                tasks.reserve(std::ranges::size(weapons));
                tasks.append_range(view);
            }

            return tasks;
        }

        static std::vector<Attack> run_synchronously(
            const std::ranges::sized_range auto& weapons, const AttackOptions& attack_options, const Stats &min_stats, const int max_attribute_points
        )
        {
            std::vector<Attack> attacks;
            attacks.reserve(std::ranges::size(weapons));
            attacks.append_range(
                V2::get_tasks(weapons, attack_options, min_stats, max_attribute_points)
                    | std::views::transform([](const auto& task) { return task(); })
            );
            return attacks;
        }
    };
    export template<Target target>
    constexpr V2<target> v2{};
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
    export template<template <Target> auto o, std::size_t I = 0, typename F>
    constexpr void visit_optimizer(std::size_t i, F&& f)
    {
        if constexpr (I < enumerators_of<Target>().size())
        {
            if (i == I)
                std::forward<F>(f)(o<integral_to_enum<Target>(I)>);
            else
                visit_optimizer<o, I + 1>(i, std::forward<F>(f));
        }
        else
            throw std::out_of_range("invalid optimizer index");
    }
}