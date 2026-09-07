export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export enum class Target
    {
        TOTAL_ATTACK_POWER,

        PHYSICAL_ATTACK_POWER,
        MAGIC_ATTACK_POWER,
        FIRE_ATTACK_POWER,
        LIGHTNING_ATTACK_POWER,
        HOLY_ATTACK_POWER,

        POISON_STATUS_EFFECT,
        SCARLET_ROT_STATUS_EFFECT,
        BLEED_STATUS_EFFECT,
        FROST_STATUS_EFFECT,
        SLEEP_STATUS_EFFECT,
        MADNESS_STATUS_EFFECT,
        DEATH_BLIGHT_STATUS_EFFECT,

        SPELL_SCALING,

        STRENGTH_SCALING,
        DEXTERITY_SCALING,
        INTELLIGENCE_SCALING,
        FAITH_SCALING,
        ARCAINE_SCALING,
    };
}
using namespace erdo;
template<>
constexpr std::array<std::pair<optimizer::Target, std::string_view>, 19> enum_string_mapping<optimizer::Target> = {
    std::pair{optimizer::Target::TOTAL_ATTACK_POWER, "TOTAL_ATTACK_POWER"},
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
    std::pair{optimizer::Target::SPELL_SCALING, "SPELL_SCALING"},
    std::pair{optimizer::Target::STRENGTH_SCALING, "STRENGTH_SCALING"},
    std::pair{optimizer::Target::DEXTERITY_SCALING, "DEXTERITY_SCALING"},
    std::pair{optimizer::Target::INTELLIGENCE_SCALING, "INTELLIGENCE_SCALING"},
    std::pair{optimizer::Target::FAITH_SCALING, "FAITH_SCALING"},
    std::pair{optimizer::Target::ARCAINE_SCALING, "ARCAINE_SCALING"}
};


export namespace erdo::optimizer::starting_class
{

    using RelevantBounds = std::array<unsigned int, 5>;

    struct StatOrthant
    {
        RelevantBounds min;
    };

    struct StatVariationRegion
    {
        RelevantBounds min;
        RelevantBounds max;
    };

    constexpr std::size_t relevant_dimension_count = 5;

    RelevantBounds relevant_bounds(const AttributeLevels& stats)
    {
        RelevantBounds result{};

        for (std::size_t i = 0; i < relevant_dimension_count; ++i)
            result[i] =
                stats[irrelevant_attribute_count + i];

        return result;
    }

    bool bounds_less_equal(const RelevantBounds& lhs, const RelevantBounds& rhs)
    {
        return std::ranges::all_of(
            std::views::iota(std::size_t{0}, relevant_dimension_count),
            [&](const std::size_t i)
            {
                return lhs[i] <= rhs[i];
            });
    }

    bool bounds_equal(const RelevantBounds& lhs, const RelevantBounds& rhs)
    {
        return lhs == rhs;
    }

    std::vector<StatOrthant> simplify_orthants(std::vector<StatOrthant> orthants)
    {
        // Remove exact duplicates first.
        std::ranges::sort(
            orthants,
            {},
            [](const StatOrthant& orthant)
            {
                return orthant.min;
            });

        orthants.erase(
            std::ranges::unique(
                orthants,
                {},
                [](const StatOrthant& orthant)
                {
                    return orthant.min;
                }).begin(),
            orthants.end());

        std::vector<StatOrthant> result;
        result.reserve(orthants.size());

        for (std::size_t i = 0; i < orthants.size(); ++i)
        {
            const auto& candidate = orthants[i];

            const bool dominated = std::ranges::any_of(
                orthants,
                [&](const StatOrthant& other)
                {
                    return &other != &candidate &&
                        bounds_less_equal(
                            other.min,
                            candidate.min);
                });

            if (!dominated)
                result.push_back(candidate);
        }

        return result;
    }

    template <typename Callback>
    void for_each_union_region(
        const std::vector<StatOrthant>& orthants,
        const RelevantBounds& global_max,
        Callback&& callback)
    {
        using Index = std::size_t;

        if (orthants.empty())
            return;

        std::vector<Index> initial_indices(orthants.size());
        std::ranges::iota(initial_indices, 0);

        RelevantBounds region_min{};
        RelevantBounds region_max = global_max;

        auto recurse =
            [&](auto&& self,
                std::span<const Index> indices,
                const std::size_t dimension,
                RelevantBounds min_bounds,
                RelevantBounds max_bounds) -> void
        {
            if (indices.empty())
                return;

            if (dimension == relevant_dimension_count)
            {
                callback(StatVariationRegion{
                    .min = min_bounds,
                    .max = max_bounds
                });
                return;
            }

            // Sort the currently active orthants by their threshold
            // in this dimension.
            std::vector<Index> sorted(
                indices.begin(),
                indices.end());

            std::ranges::sort(
                sorted,
                [&](const Index lhs, const Index rhs)
                {
                    return orthants[lhs].min[dimension]
                        < orthants[rhs].min[dimension];
                });

            const auto coordinate_min =
                min_bounds[dimension];

            const auto coordinate_max =
                max_bounds[dimension];

            /*
            * Number of orthants which are already active at
            * coordinate_min.
            */
            std::size_t active_count = 0;

            while (active_count < sorted.size() &&
                orthants[sorted[active_count]].min[dimension]
                    <= coordinate_min)
            {
                ++active_count;
            }

            /*
            * If nothing is active at coordinate_min, jump directly
            * to the first threshold.
            */
            unsigned int slab_min = coordinate_min;

            if (active_count == 0)
            {
                slab_min =
                    orthants[sorted.front()].min[dimension];

                if (slab_min > coordinate_max)
                    return;

                while (active_count < sorted.size() &&
                    orthants[sorted[active_count]].min[dimension]
                        <= slab_min)
                {
                    ++active_count;
                }
            }

            while (active_count != 0)
            {
                /*
                * Find the next point where the active set changes.
                */
                const auto next_threshold =
                    active_count < sorted.size()
                        ? orthants[sorted[active_count]].min[dimension]
                        : coordinate_max + 1;

                const auto slab_max =
                    std::min(
                        coordinate_max,
                        next_threshold - 1);

                if (slab_min <= slab_max)
                {
                    auto next_min = min_bounds;
                    auto next_max = max_bounds;

                    next_min[dimension] = slab_min;
                    next_max[dimension] = slab_max;

                    self(
                        self,
                        std::span<const Index>(
                            sorted.data(),
                            active_count),
                        dimension + 1,
                        next_min,
                        next_max);
                }

                if (active_count == sorted.size())
                    break;

                slab_min = next_threshold;

                if (slab_min > coordinate_max)
                    break;

                while (active_count < sorted.size() &&
                    orthants[sorted[active_count]].min[dimension]
                        <= slab_min)
                {
                    ++active_count;
                }
            }
        };

        recurse(
            recurse,
            std::span<const Index>(initial_indices),
            0,
            region_min,
            region_max);
    }

    std::optional<int> calculate_target_sum(
        const int max_attribute_points,
        const AttributeLevels& min_stats,
        const RelevantAttributeLevels& global_max)
    {
        const auto min_relevant =
            min_stats.relevant_stats();

        int min_relevant_sum = 0;
        int max_relevant_sum = 0;

        for (std::size_t i = 0; i < relevant_dimension_count; ++i)
        {
            if (min_relevant[i] > global_max[i])
            {
                throw std::invalid_argument(
                    "min relevant attribute exceeds maximum relevant attribute.");
            }

            min_relevant_sum +=
                static_cast<int>(min_relevant[i]);

            max_relevant_sum +=
                static_cast<int>(global_max[i]);
        }

        const int free_attribute_points =
            max_attribute_points -
            min_stats.attribute_points();

        if (free_attribute_points < 0)
            return std::nullopt;

        return min_relevant_sum +
            std::min(
                free_attribute_points,
                max_relevant_sum - min_relevant_sum);
    }

    std::size_t count_region(
        const int target_sum,
        const StatVariationRegion& region)
    {
        int min_sum = 0;
        int max_sum = 0;

        std::array<std::size_t, relevant_dimension_count> capacity{};

        for (std::size_t i = 0;
            i < relevant_dimension_count;
            ++i)
        {
            if (region.min[i] > region.max[i])
                return 0;

            min_sum += static_cast<int>(region.min[i]);
            max_sum += static_cast<int>(region.max[i]);

            capacity[i] =
                static_cast<std::size_t>(region.max[i]) -
                static_cast<std::size_t>(region.min[i]);
        }

        /*
        * Unlike the original public count function, there is
        * deliberately NO saturation here.
        */
        if (target_sum < min_sum ||
            target_sum > max_sum)
        {
            return 0;
        }

        const auto free_points =
            static_cast<std::size_t>(target_sum - min_sum);

        std::vector<std::uint64_t> dp(free_points + 1);
        std::vector<std::uint64_t> next(free_points + 1);

        dp[0] = 1;

        for (const auto C : capacity)
        {
            std::uint64_t window = 0;

            for (std::size_t sum = 0;
                sum <= free_points;
                ++sum)
            {
                window += dp[sum];

                if (sum > C)
                    window -= dp[sum - C - 1];

                next[sum] = window;
            }

            dp.swap(next);
        }

        return static_cast<std::size_t>(
            dp[free_points]);
    }

    template <std::output_iterator<AttributeLevels> OutputIt>
    void generate_region(
        const int target_sum,
        const AttributeLevels& base_stats,
        const StatVariationRegion& region,
        OutputIt output)
    {
        std::array<int, relevant_dimension_count> lo{};
        std::array<int, relevant_dimension_count> hi{};

        int min_sum = 0;
        int max_sum = 0;

        for (std::size_t i = 0;
            i < relevant_dimension_count;
            ++i)
        {
            if (region.min[i] > region.max[i])
                return;

            lo[i] = static_cast<int>(region.min[i]);
            hi[i] = static_cast<int>(region.max[i]);

            min_sum += lo[i];
            max_sum += hi[i];
        }

        if (target_sum < min_sum ||
            target_sum > max_sum)
        {
            return;
        }

        auto result = base_stats;

        auto& a = result[irrelevant_attribute_count + 0];
        auto& b = result[irrelevant_attribute_count + 1];
        auto& c = result[irrelevant_attribute_count + 2];
        auto& d = result[irrelevant_attribute_count + 3];
        auto& e = result[irrelevant_attribute_count + 4];

        for (int ai = lo[0];
            ai <= std::min(hi[0], target_sum);
            ++ai)
        {
            const int remaining_a =
                target_sum - ai;

            for (int bi = lo[1];
                bi <= std::min(hi[1], remaining_a);
                ++bi)
            {
                const int remaining_ab =
                    remaining_a - bi;

                for (int ci = lo[2];
                    ci <= std::min(hi[2], remaining_ab);
                    ++ci)
                {
                    const int remaining_abc =
                        remaining_ab - ci;

                    const int d_min =
                        std::max(
                            lo[3],
                            remaining_abc - hi[4]);

                    const int d_max =
                        std::min(
                            hi[3],
                            remaining_abc - lo[4]);

                    for (int di = d_min;
                        di <= d_max;
                        ++di)
                    {
                        const int ei =
                            remaining_abc - di;

                        a = static_cast<unsigned int>(ai);
                        b = static_cast<unsigned int>(bi);
                        c = static_cast<unsigned int>(ci);
                        d = static_cast<unsigned int>(di);
                        e = static_cast<unsigned int>(ei);

                        *output++ = result;
                    }
                }
            }
        }
    }

    std::vector<AttributeLevels> get_stat_variations(
        const int max_attribute_points,
        const std::vector<AttributeLevels>& min_stats,
        const RelevantAttributeLevels& max_relevant_stats)
    {
        if (min_stats.empty())
            return {};

        const RelevantBounds global_max = [](const RelevantAttributeLevels& max_stats) {
            RelevantBounds result{};

            for (std::size_t i = 0; i < relevant_dimension_count; ++i)
                result[i] = max_stats[i];

            return result;
        }(max_relevant_stats);

        /*
        * First group by the three irrelevant attributes.
        */
        using IrrelevantKey = std::array<unsigned int, irrelevant_attribute_count>;

        std::map<IrrelevantKey, std::vector<AttributeLevels>> groups;

        for (const auto& stats : min_stats)
        {
            IrrelevantKey key{};

            for (std::size_t i = 0; i < irrelevant_attribute_count; ++i)
                key[i] = stats[i];

            groups[key].push_back(stats);
        }

        std::vector<AttributeLevels> result;

        for (auto& [irrelevant, group] : groups)
        {
            const auto target =
                calculate_target_sum(
                    max_attribute_points,
                    group.front(),
                    max_relevant_stats);

            if (!target)
                continue;

            std::vector<StatOrthant> orthants;
            orthants.reserve(group.size());

            for (const auto& stats : group)
            {
                const auto min = relevant_bounds(stats);

                /*
                * This is already impossible at the requested
                * target sum, so this orthant can be discarded.
                */
                int min_sum = 0;

                for (const auto value : min)
                    min_sum += static_cast<int>(value);

                if (min_sum > *target)
                    continue;

                orthants.push_back({
                    .min = min
                });
            }

            orthants = simplify_orthants(
                std::move(orthants));

            if (orthants.empty())
                continue;

            /*
            * Count the disjoint decomposition first.
            */
            std::size_t group_count = 0;

            for_each_union_region(
                orthants,
                global_max,
                [&](const StatVariationRegion& region)
                {
                    group_count +=
                        count_region(*target, region);
                });

            result.reserve(result.size() + group_count);

            /*
            * Generate exactly the same regions we counted.
            */
            for_each_union_region(
                orthants,
                global_max,
                [&](const StatVariationRegion& region)
                {
                    generate_region(
                        *target,
                        group.front(),
                        region,
                        std::back_inserter(result));
                });
        }

        return result;
    }

    std::size_t get_stat_variation_count(
        const int max_attribute_points,
        const std::vector<AttributeLevels>& min_stats,
        const RelevantAttributeLevels& max_relevant_stats)
    {
        if (min_stats.empty())
            return 0;

        const RelevantBounds global_max = [](
            const RelevantAttributeLevels& max_stats)
        {
            RelevantBounds result{};

            for (std::size_t i = 0;
                i < relevant_dimension_count;
                ++i)
            {
                result[i] = max_stats[i];
            }

            return result;
        }(max_relevant_stats);

        using IrrelevantKey =
            std::array<unsigned int, irrelevant_attribute_count>;

        std::map<IrrelevantKey, std::vector<AttributeLevels>> groups;

        for (const auto& stats : min_stats)
        {
            IrrelevantKey key{};

            for (std::size_t i = 0;
                i < irrelevant_attribute_count;
                ++i)
            {
                key[i] = stats[i];
            }

            groups[key].push_back(stats);
        }

        std::size_t total_count = 0;

        for (auto& [irrelevant, group] : groups)
        {
            const auto target =
                calculate_target_sum(
                    max_attribute_points,
                    group.front(),
                    max_relevant_stats);

            if (!target)
                continue;

            std::vector<StatOrthant> orthants;
            orthants.reserve(group.size());

            for (const auto& stats : group)
            {
                const auto min = relevant_bounds(stats);

                int min_sum = 0;

                for (const auto value : min)
                    min_sum += static_cast<int>(value);

                // No solution from this orthant can reach the target.
                if (min_sum > *target)
                    continue;

                orthants.push_back({
                    .min = min
                });
            }

            orthants = simplify_orthants(
                std::move(orthants));

            if (orthants.empty())
                continue;

            for_each_union_region(
                orthants,
                global_max,
                [&](const StatVariationRegion& region)
                {
                    total_count +=
                        count_region(*target, region);
                });
        }

        return total_count;
    }

    std::vector<AttributeLevels> get_min_stats(const AttributeLevels& min_stats)
    {
        std::vector<AttributeLevels> result{character_starting_class_attributes.size()};
        result.reserve(character_starting_class_attributes.size());

        for (auto&& [starting_class, result_elem] : std::views::zip(character_starting_class_attributes, result))
        {
            auto&& [_, stats] = starting_class;
            for (auto&& [min_stat, stat, result_elem_elem] : std::views::zip(min_stats, stats, result_elem))
            {
                result_elem_elem = std::max(min_stat, stat);
            }
        }

        return result;
    }
}

namespace erdo::optimizer
{
    export std::size_t get_stat_variation_count(const int max_attribute_points, const AttributeLevels& min_stats, const RelevantAttributeLevels& max_relevant_stats)
    {
        auto min_relevant_stats = min_stats.relevant_stats();
        auto free_attribute_points = max_attribute_points - min_stats.attribute_points();

        if (free_attribute_points < 0)
            return 0;

        const auto A = static_cast<std::size_t>(free_attribute_points);
        constexpr std::size_t P = min_relevant_stats.extent;

        std::array<std::size_t, P> capacity{};
        std::size_t totalCapacity = 0;

        for (std::size_t i = 0; i < P; ++i)
        {
            if (min_relevant_stats[i] > max_relevant_stats[i])
                throw std::invalid_argument("min_relevant_stats[i] must be <= max_relevant_stats[i].");

            const auto C = static_cast<std::size_t>(max_relevant_stats[i]) - static_cast<std::size_t>(min_relevant_stats[i]);

            capacity[i] = C;
            totalCapacity += C;
        }

        if (A > totalCapacity)
            return 1;

        std::vector<std::uint64_t> dp(A + 1);
        std::vector<std::uint64_t> next(A + 1);

        dp[0] = 1;

        for (const auto C : capacity)
        {
            std::uint64_t window = 0;

            for (std::size_t a = 0; a <= A; ++a)
            {
                window += dp[a];

                if (a > C)
                    window -= dp[a - C - 1];

                next[a] = window;
            }

            dp.swap(next);
        }

        return static_cast<std::size_t>(dp[A]);
    }
    export std::vector<AttributeLevels> get_stat_variations(const int max_attribute_points, const AttributeLevels &min_stats, const RelevantAttributeLevels& max_relevant_stats)
    {
        auto min_relevant_stats = min_stats.relevant_stats();
        auto free_attribute_points = max_attribute_points - min_stats.attribute_points();

        const int min_relevant_sum = std::ranges::fold_left(min_relevant_stats, 0, std::plus<int>{});
        const int max_relevant_sum = std::ranges::fold_left(max_relevant_stats, 0, std::plus<int>{});
        const int SUM = min_relevant_sum + std::min(free_attribute_points, max_relevant_sum - min_relevant_sum);

        auto possible_occurances = get_stat_variation_count(max_attribute_points, min_stats, max_relevant_stats);

        std::vector<AttributeLevels> stat_variations{ possible_occurances };
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
                    {
                        m = SUM_i_j_k - l;

                        *current_it++ = result;
                    }
                }
            }
        }

        if (current_it != stat_variations.end())
            throw std::runtime_error(std::format(
                "Mismatch in expected ({}) and actual ({}) number of stat variations generated.\nmin_stats: {}, max_attribute_points: {}",
                possible_occurances,
                std::distance(stat_variations.begin(), current_it),
                min_stats,
                max_attribute_points
            ));

        return stat_variations;
    }

    export template<Target target>
    struct Projection;
    template<>
    struct Projection<Target::TOTAL_ATTACK_POWER>
    {
        static const double& operator()(const Attack& attack)
        {
            return attack.total_attack_power[1];
        }
    };
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target) - std::to_underlying(Target::PHYSICAL_ATTACK_POWER)))
    struct Projection<target>
    {
        static constexpr auto attack_power_type_integral = std::to_underlying(target) - std::to_underlying(Target::PHYSICAL_ATTACK_POWER);
        static constexpr auto attack_power_type = integral_to_enum<AttackPowerType>(attack_power_type_integral);

        static const double& operator()(const Attack& attack)
        {
            return attack.attack_powers[attack_power_type_integral][1];
        }
    };
    template<>
    struct Projection<Target::SPELL_SCALING>
    {
        static const double& operator()(const Attack& attack)
        {
            return attack.spell_scaling;
        }
    };
    template<Target target> requires (is_valid_enum_integral<RelevantAttribute>(std::to_underlying(target) - std::to_underlying(Target::STRENGTH_SCALING)))
    struct Projection<target>
    {
        static constexpr auto attribute_integral = std::to_underlying(target) - std::to_underlying(Target::STRENGTH_SCALING);
        static constexpr auto attribute = integral_to_enum<calculator::RelevantAttribute>(attribute_integral);

        static const double& operator()(const calculator::Attack& attack)
        {
            return attack.attribute_scalings_at_upgrade_level()[attribute_integral];
        }
    };
    export template<Target target>
    constexpr Projection<target> projection{};
    export constexpr auto projections = [](auto){
        static constexpr auto [...targets] = enumerators_of<Target>();
        return std::array{ Projection<targets>::operator()... };
    }(1);

    template<Target target>
    struct EfficientCalculateAttack;
    template<>
    struct EfficientCalculateAttack<Target::TOTAL_ATTACK_POWER>
    {
        static void operator()(Attack& attack)
        {
            attack.calculate_inplace();
        }
    };
    template<Target target> requires (is_valid_enum_integral<AttackPowerType>(std::to_underlying(target) - std::to_underlying(Target::PHYSICAL_ATTACK_POWER)))
    struct EfficientCalculateAttack<target>
    {
        static void operator()(Attack& attack)
        {
            attack.calculate_attack_power_inplace(Projection<target>::attack_power_type);
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

    export template<Target target>
    concept valid_optimizer_target = requires {
        sizeof(Projection<target>);
        sizeof(EfficientCalculateAttack<target>);
    };

    template<Target target_> requires valid_optimizer_target<target_>
    struct OptimizerBase
    {
        static constexpr auto target = target_;
        static constexpr Projection<target> projection{};
        static constexpr EfficientCalculateAttack<target> efficient_calculate_attack{};

        static Attack optimize_weapon(const Weapon& weapon, const AttackOptions& attack_options, const std::vector<AttributeLevels>& stat_variations)
        {
            if (stat_variations.empty())
                throw std::invalid_argument("stat_variations must not be empty.");

            Attack attack{ weapon, {}, attack_options };
            AttributeLevels const* best_stats = nullptr;
            auto best_value = std::numeric_limits<std::remove_cvref_t<decltype(projection(attack))>>::lowest();

            for (const auto& stats : stat_variations)
            {
                attack.stats = stats;
                efficient_calculate_attack(attack);

                auto new_value = projection(attack);
                if (best_value < new_value ||
                    (best_stats && best_value == new_value && best_stats->character_level() > attack.stats.character_level()))
                {
                    best_stats = &stats;
                    best_value = new_value;
                }
            }

            attack.stats = *best_stats;
            attack.calculate_inplace();

            return attack;
        }
    
        std::vector<Attack> run_synchronously(this const auto& self, const std::ranges::sized_range auto& weapons)
        {
            return weapons
                | std::views::transform(self)
                | std::ranges::to<std::vector>();
        }
   
        std::size_t iteration_count = 0;
    };

    export template<Target target> requires valid_optimizer_target<target>
    struct BruteForce : OptimizerBase<target>
    {
        std::vector<AttributeLevels> stat_variations{};
        AttackOptions attack_options;

        BruteForce(
            std::ranges::sized_range auto&& weapons,
            const AttackOptions& attack_options,
            int max_attribute_points,
            const AttributeLevels &min_stats,
            int max_stat,
            bool optimize_starting_class
        ) : attack_options{ attack_options }
        {
            if (optimize_starting_class)
            {
                std::println("{}", starting_class::get_min_stats(min_stats));
                this->stat_variations = starting_class::get_stat_variations(
                    max_attribute_points,
                    starting_class::get_min_stats(min_stats),
                    make_filled_array<RelevantAttributeLevels>(max_stat)
                );
            }
            else
            {
                this->stat_variations = get_stat_variations(
                    max_attribute_points,
                    min_stats,
                    make_filled_array<RelevantAttributeLevels>(max_stat)
                );
            }
            
            this->iteration_count = this->stat_variations.size() * weapons.size();
        }

        Attack operator()(const Weapon& weapon) const
        {
            return this->optimize_weapon(weapon, this->attack_options, this->stat_variations);
        }
    };

    export template<Target target> requires valid_optimizer_target<target>
    struct V2 : OptimizerBase<target>
    {
        static RelevantAttributeLevelsArray get_optimized_max_relevant_stats(
            const RelevantAttributeLevels &min_relevant_stats,
            const RelevantAttributeLevels& max_relevant_stats,
            const NonscalingAttributes& nonscaling_attributes
        )
        {
            RelevantAttributeLevelsArray optimized_max_relevant_stats{};
            for (auto&& [min_relevant_stat, max_relevant_stat, nonscaling_attribute, optimized_max_relevant_stat] :
                std::views::zip(min_relevant_stats, max_relevant_stats, nonscaling_attributes, optimized_max_relevant_stats)
            )
                optimized_max_relevant_stat = nonscaling_attribute ? min_relevant_stat : max_relevant_stat;
            return optimized_max_relevant_stats;
        }

        std::map<NonscalingAttributes, std::vector<AttributeLevels>> optimized_stat_variations_map{};
        AttackOptions attack_options;

        V2(
            std::ranges::sized_range auto&& weapons,
            const AttackOptions& attack_options,
            int max_attribute_points,
            const AttributeLevels &min_stats,
            int max_stat,
            bool optimize_starting_class
        ) : attack_options{ attack_options }
        {
            auto min_relevant_stats = min_stats.relevant_stats();
            auto max_relevant_stats = make_filled_array<RelevantAttributeLevels>(max_stat);

            for (auto&& weapon : weapons)
            {
                auto&& nonscaling_attributes = std::invoke(&Weapon::nonscaling_attributes, weapon);
                auto [it, inserted] = this->optimized_stat_variations_map.try_emplace(nonscaling_attributes);
                auto&& optimized_stat_variations = it->second;
                if (inserted)
                {
                    if (optimize_starting_class)
                    {
                        optimized_stat_variations = starting_class::get_stat_variations(
                            max_attribute_points,
                            starting_class::get_min_stats(min_stats),
                            get_optimized_max_relevant_stats(min_relevant_stats, max_relevant_stats, nonscaling_attributes)
                        );
                        static_assert(false, "get_optimized_max_relevant_stats needs to be adjusted for optimize_starting_class");
                    }
                    else
                    {
                        optimized_stat_variations = get_stat_variations(
                            max_attribute_points,
                            min_stats,
                            get_optimized_max_relevant_stats(min_relevant_stats, max_relevant_stats, nonscaling_attributes)
                        );
                    }
                    
                    if(optimized_stat_variations.size() == 0)
                    {
                        this->iteration_count = 0;
                        this->optimized_stat_variations_map.clear();
                        break;
                    }
                }

                this->iteration_count += optimized_stat_variations.size();
            }
        }

        Attack operator()(const Weapon& weapon) const
        {
            auto&& optimized_stat_variations = this->optimized_stat_variations_map.at(weapon.nonscaling_attributes);
            return this->optimize_weapon(weapon, attack_options, optimized_stat_variations);
        }
    };
}

