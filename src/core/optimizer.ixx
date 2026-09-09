export module erdo:optimizer;
import :calculator;

import std;

using namespace erdo::calculator;


namespace erdo::optimizer
{
    export using VariedAttributes = std::array<bool, std::tuple_size_v<AttributeLevels>>;
    export constexpr VariedAttributes default_varied_attributes = { false, false, false, true, true, true, true, true };

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
namespace erdo
{
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
}

export namespace erdo::optimizer::starting_class_old
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
        const auto min_relevant = relevant_attribute_levels(min_stats);

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
            attribute_points(min_stats);

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

namespace erdo::optimizer::starting_class
{
    namespace detail
    {
        constexpr std::size_t P = std::tuple_size_v<AttributeLevels>;

        // Deliberately fixed-width. See validate_min_count() below.
        using MinMask = std::uint64_t;

        constexpr std::size_t MIN_MASK_BITS =
            std::numeric_limits<MinMask>::digits;

        void validate_min_count(std::size_t count)
        {
            if (count > MIN_MASK_BITS)
            {
                throw std::invalid_argument(std::format(
                    "Too many min_attr_lvls ({}) for the variation "
                    "deduplication bitmask (maximum {}).",
                    count,
                    MIN_MASK_BITS));
            }
        }

        struct Bounds
        {
            AttributeLevels adjusted_max{};
            int free_attribute_points{};
            unsigned int min_relevant_sum{};
            unsigned int max_relevant_sum{};
        };

        Bounds make_bounds(
            const int max_attribute_points,
            const AttributeLevels& min_attr_lvls,
            const AttributeLevels& max_attr_lvls,
            const VariedAttributes& varied_attributes)
        {
            Bounds result;
            result.free_attribute_points =
                max_attribute_points - attribute_points(min_attr_lvls);

            for (std::size_t i = 0; i < P; ++i)
            {
                if (varied_attributes[i])
                {
                    if (min_attr_lvls[i] > max_attr_lvls[i])
                    {
                        throw std::invalid_argument(
                            "min_attr_lvls[i] must be <= max_attr_lvls[i].");
                    }

                    result.min_relevant_sum += min_attr_lvls[i];
                    result.max_relevant_sum += max_attr_lvls[i];
                    result.adjusted_max[i] = max_attr_lvls[i];
                }
                else
                {
                    result.adjusted_max[i] = min_attr_lvls[i];
                }
            }

            return result;
        }

        /*
        * Removes previous minima that can no longer contain the candidate.
        *
        * At this point candidate[i] has been assigned.
        *
        * For a varied attribute:
        *
        *     candidate[i] >= previous_min[i]
        *
        * is sufficient because candidate[i] is already constrained to
        * max_attr_lvls[i] by the enumeration.
        *
        * For a non-varied attribute:
        *
        *     candidate[i] == previous_min[i]
        */
        template <std::size_t I>
        MinMask filter_mask(
            const MinMask mask,
            const unsigned int value,
            const std::span<const AttributeLevels> mins,
            const VariedAttributes& varied_attributes)
        {
            MinMask result = mask;

            for (std::size_t j = 0; j < mins.size(); ++j)
            {
                const MinMask bit =
                    static_cast<MinMask>(MinMask{1} << j);

                if (!(result & bit))
                    continue;

                if (varied_attributes[I])
                {
                    if (value < mins[j][I])
                        result &= static_cast<MinMask>(~bit);
                }
                else
                {
                    if (value != mins[j][I])
                        result &= static_cast<MinMask>(~bit);
                }
            }

            return result;
        }

        /*
        * The same operation without the template parameter is useful for
        * the final p value.
        */
        MinMask filter_mask(
            const MinMask mask,
            const std::size_t attribute,
            const unsigned int value,
            const std::span<const AttributeLevels> mins,
            const VariedAttributes& varied_attributes)
        {
            MinMask result = mask;

            for (std::size_t j = 0; j < mins.size(); ++j)
            {
                const MinMask bit =
                    static_cast<MinMask>(MinMask{1} << j);

                if (!(result & bit))
                    continue;

                if (varied_attributes[attribute])
                {
                    if (value < mins[j][attribute])
                        result &= static_cast<MinMask>(~bit);
                }
                else
                {
                    if (value != mins[j][attribute])
                        result &= static_cast<MinMask>(~bit);
                }
            }

            return result;
        }

        /*
        * Enumerates the UNION.
        *
        * The current minimum is responsible for generating a candidate.
        * previous_mask contains the earlier minimums that could also contain
        * the candidate.
        *
        * Therefore:
        *
        *     previous_mask == 0
        *
        * means that this candidate belongs to the current minimum and to no
        * earlier minimum, so it is emitted exactly once.
        */
        template <typename Emit>
        void enumerate_variations(
            const int max_attribute_points,
            const std::vector<AttributeLevels>& min_attr_lvls,
            const AttributeLevels& max_attr_lvls,
            const VariedAttributes& varied_attributes,
            Emit&& emit)
        {
            validate_min_count(min_attr_lvls.size());

            if (min_attr_lvls.empty())
                return;

            const std::span<const AttributeLevels> all_mins{min_attr_lvls};

            for (std::size_t min_index = 0;
                min_index < min_attr_lvls.size();
                ++min_index)
            {
                const auto& min = min_attr_lvls[min_index];

                const auto bounds = make_bounds(
                    max_attribute_points,
                    min,
                    max_attr_lvls,
                    varied_attributes);

                /*
                * Preserve the original behavior:
                *
                *     free_attribute_points < 0
                *
                * means this minimum produces no variations.
                */
                if (bounds.free_attribute_points < 0)
                    continue;

                /*
                * Mask containing all EARLIER minima.
                *
                * The current minimum itself is deliberately not included.
                */
                const MinMask previous_mask =
                    min_index == 0
                        ? MinMask{0}
                        : static_cast<MinMask>(
                            (MinMask{1} << min_index) - MinMask{1});

                /*
                * Preserve the original behavior:
                *
                *     free_attribute_points >
                *         max_relevant_sum - min_relevant_sum
                *
                * means there is exactly one result:
                *
                *     all varied attributes at max
                *     all non-varied attributes at min
                */
                if (bounds.free_attribute_points >
                    static_cast<int>(
                        bounds.max_relevant_sum -
                        bounds.min_relevant_sum))
                {
                    const auto candidate = bounds.adjusted_max;

                    const auto remaining_mask = [&]
                    {
                        MinMask mask = previous_mask;

                        for (std::size_t i = 0; i < P; ++i)
                        {
                            mask = filter_mask(
                                mask,
                                i,
                                candidate[i],
                                all_mins.first(min_index),
                                varied_attributes);

                            if (mask == 0)
                                break;
                        }

                        return mask;
                    }();

                    if (remaining_mask == 0)
                        emit(candidate);

                    continue;
                }

                const int SUM = max_attribute_points;

                auto result = min;
                auto&& [i, j, k, l, m, n, o, p] = result;

                for (i = min[0];
                    std::cmp_less_equal(
                        i,
                        std::min<int>(bounds.adjusted_max[0], SUM));
                    ++i)
                {
                    auto mask_i = filter_mask<0>(
                        previous_mask,
                        i,
                        all_mins.first(min_index),
                        varied_attributes);

                    if (mask_i == 0)
                    {
                        /*
                        * No earlier minimum can contain anything below this
                        * branch, so normal generation can proceed without
                        * carrying a mask.
                        *
                        * We still use the same loops below; the compiler
                        * should make the zero-mask path very cheap.
                        */
                    }

                    const auto sum_i = SUM - static_cast<int>(i);

                    for (j = min[1];
                        std::cmp_less_equal(
                            j,
                            std::min<int>(
                                bounds.adjusted_max[1],
                                sum_i));
                        ++j)
                    {
                        const auto mask_j = filter_mask<1>(
                            mask_i,
                            j,
                            all_mins.first(min_index),
                            varied_attributes);

                        const auto sum_ij =
                            sum_i - static_cast<int>(j);

                        for (k = min[2];
                            std::cmp_less_equal(
                                k,
                                std::min<int>(
                                    bounds.adjusted_max[2],
                                    sum_ij));
                            ++k)
                        {
                            const auto mask_k = filter_mask<2>(
                                mask_j,
                                k,
                                all_mins.first(min_index),
                                varied_attributes);

                            const auto sum_ijk =
                                sum_ij - static_cast<int>(k);

                            const auto l_min = std::max<int>(
                                min[3],
                                sum_ijk
                                    - static_cast<int>(bounds.adjusted_max[4])
                                    - static_cast<int>(bounds.adjusted_max[5])
                                    - static_cast<int>(bounds.adjusted_max[6])
                                    - static_cast<int>(bounds.adjusted_max[7]));

                            const auto l_max = std::min<int>(
                                bounds.adjusted_max[3],
                                sum_ijk
                                    - static_cast<int>(min[4])
                                    - static_cast<int>(min[5])
                                    - static_cast<int>(min[6])
                                    - static_cast<int>(min[7]));

                            for (l = l_min;
                                std::cmp_less_equal(l, l_max);
                                ++l)
                            {
                                const auto mask_l = filter_mask<3>(
                                    mask_k,
                                    l,
                                    all_mins.first(min_index),
                                    varied_attributes);

                                const auto sum_ijkl =
                                    sum_ijk - static_cast<int>(l);

                                const auto m_min = std::max<int>(
                                    min[4],
                                    sum_ijkl
                                        - static_cast<int>(bounds.adjusted_max[5])
                                        - static_cast<int>(bounds.adjusted_max[6])
                                        - static_cast<int>(bounds.adjusted_max[7]));

                                const auto m_max = std::min<int>(
                                    bounds.adjusted_max[4],
                                    sum_ijkl
                                        - static_cast<int>(min[5])
                                        - static_cast<int>(min[6])
                                        - static_cast<int>(min[7]));

                                for (m = m_min;
                                    std::cmp_less_equal(m, m_max);
                                    ++m)
                                {
                                    const auto mask_m = filter_mask<4>(
                                        mask_l,
                                        m,
                                        all_mins.first(min_index),
                                        varied_attributes);

                                    const auto sum_ijklm =
                                        sum_ijkl - static_cast<int>(m);

                                    const auto n_min = std::max<int>(
                                        min[5],
                                        sum_ijklm
                                            - static_cast<int>(bounds.adjusted_max[6])
                                            - static_cast<int>(bounds.adjusted_max[7]));

                                    const auto n_max = std::min<int>(
                                        bounds.adjusted_max[5],
                                        sum_ijklm
                                            - static_cast<int>(min[6])
                                            - static_cast<int>(min[7]));

                                    for (n = n_min;
                                        std::cmp_less_equal(n, n_max);
                                        ++n)
                                    {
                                        const auto mask_n = filter_mask<5>(
                                            mask_m,
                                            n,
                                            all_mins.first(min_index),
                                            varied_attributes);

                                        const auto remaining =
                                            sum_ijklm - static_cast<int>(n);

                                        const auto o_min = std::max<int>(
                                            min[6],
                                            remaining
                                                - static_cast<int>(bounds.adjusted_max[7]));

                                        const auto o_max = std::min<int>(
                                            bounds.adjusted_max[6],
                                            remaining
                                                - static_cast<int>(min[7]));

                                        for (o = o_min;
                                            std::cmp_less_equal(o, o_max);
                                            ++o)
                                        {
                                            const auto mask_o = filter_mask<6>(
                                                mask_n,
                                                o,
                                                all_mins.first(min_index),
                                                varied_attributes);

                                            /*
                                            * p is uniquely determined.
                                            */
                                            p = remaining - static_cast<int>(o);

                                            /*
                                            * p is the final opportunity for an
                                            * earlier minimum to contain this
                                            * candidate.
                                            */
                                            const auto mask_p = filter_mask(
                                                mask_o,
                                                7,
                                                p,
                                                all_mins.first(min_index),
                                                varied_attributes);

                                            if (mask_p == 0)
                                                emit(result);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    export std::size_t get_stat_variation_count(
        const int max_attribute_points,
        const std::vector<AttributeLevels>& min_attr_lvls,
        const AttributeLevels& max_attr_lvls,
        const VariedAttributes& varied_attributes)
    {
        std::size_t count = 0;

        detail::enumerate_variations(
            max_attribute_points,
            min_attr_lvls,
            max_attr_lvls,
            varied_attributes,
            [&](const AttributeLevels&)
            {
                ++count;
            });

        return count;
    }

    export std::vector<AttributeLevels> get_stat_variations(
        const int max_attribute_points,
        const std::vector<AttributeLevels>& min_attr_lvls,
        const AttributeLevels& max_attr_lvls,
        const VariedAttributes& varied_attributes)
    {
        std::vector<AttributeLevels> result;

        detail::enumerate_variations(
            max_attribute_points,
            min_attr_lvls,
            max_attr_lvls,
            varied_attributes,
            [&](const AttributeLevels& variation)
            {
                result.push_back(variation);
            });

        return result;
    }

    export std::vector<AttributeLevels> get_min_stats(const AttributeLevels& min_stats)
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
    export std::size_t get_stat_variation_count(
        const int max_attribute_points,
        const AttributeLevels& min_attr_lvls,
        const AttributeLevels& max_attr_lvls,
        const VariedAttributes& varied_attributes
    )
    {
        auto free_attribute_points = max_attribute_points - attribute_points(min_attr_lvls);
        if (free_attribute_points < 0)
            return 0;

        const auto A = static_cast<std::size_t>(free_attribute_points);

        constexpr std::size_t P = std::tuple_size_v<AttributeLevels>;

        std::array<std::size_t, P> capacity{};
        std::size_t totalCapacity = 0;

        for (std::size_t i = 0; i < P; ++i)
        {
            if (!varied_attributes[i])
                continue;

            if (min_attr_lvls[i] > max_attr_lvls[i])
                throw std::invalid_argument("min_attr_lvls[i] must be <= max_attr_lvls[i].");

            const auto C = static_cast<std::size_t>(max_attr_lvls[i]) - static_cast<std::size_t>(min_attr_lvls[i]);

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
    export std::vector<AttributeLevels> get_stat_variations(
        const int max_attribute_points,
        const AttributeLevels &min_attr_lvls,
        const AttributeLevels& max_attr_lvls,
        const VariedAttributes& varied_attributes
    )
    {
        const auto free_attribute_points = max_attribute_points - attribute_points(min_attr_lvls);
        if (free_attribute_points < 0)
            return {};

        unsigned int min_relevant_sum = 0;
        unsigned int max_relevant_sum = 0;
        AttributeLevels adjusted_max_attr_lvls{};
        for(auto&& [min_attr_lvl, max_attr_lvl, adjusted_max_attr_lvl, varied_attribute] : std::views::zip(
            min_attr_lvls,
            max_attr_lvls,
            adjusted_max_attr_lvls,
            varied_attributes
        ))
        {
            if(varied_attribute)
            {
                min_relevant_sum += min_attr_lvl;
                max_relevant_sum += max_attr_lvl;
                adjusted_max_attr_lvl = max_attr_lvl;
            }
            else
            {
                adjusted_max_attr_lvl = min_attr_lvl;
            }
        }

        if (free_attribute_points > max_relevant_sum - min_relevant_sum)
            return { adjusted_max_attr_lvls };

        const int SUM = max_attribute_points;

        const auto possible_occurances = get_stat_variation_count(max_attribute_points, min_attr_lvls, max_attr_lvls, varied_attributes);

        std::vector<AttributeLevels> stat_variations{ possible_occurances };
        auto current_it = stat_variations.begin(); 

        auto result = min_attr_lvls;
        auto&& [i, j, k, l, m, n, o, p] = result;

        for (i = min_attr_lvls[0]; std::cmp_less_equal(i, std::min<int>(adjusted_max_attr_lvls[0], SUM)); ++i)
        {
            const auto sum_i = SUM - (int)i;

            for (j = min_attr_lvls[1]; std::cmp_less_equal(j, std::min<int>(adjusted_max_attr_lvls[1], sum_i)); ++j)
            {
                const auto sum_ij = sum_i - (int)j;

                for (k = min_attr_lvls[2]; std::cmp_less_equal(k, std::min<int>(adjusted_max_attr_lvls[2], sum_ij)); ++k)
                {
                    const auto sum_ijk = sum_ij - (int)k;

                    // l must leave enough room for stages 4..7.
                    const auto l_min = std::max<int>(
                        min_attr_lvls[3],
                        sum_ijk - adjusted_max_attr_lvls[4] - adjusted_max_attr_lvls[5] - adjusted_max_attr_lvls[6] - adjusted_max_attr_lvls[7]
                    );

                    const auto l_max = std::min<int>(
                        adjusted_max_attr_lvls[3],
                        sum_ijk - min_attr_lvls[4] - min_attr_lvls[5] - min_attr_lvls[6] - min_attr_lvls[7]
                    );

                    for (l = l_min; std::cmp_less_equal(l, l_max); ++l)
                    {
                        const auto sum_ijkl = sum_ijk - (int)l;

                        // m must leave enough room for stages 5..7.
                        const auto m_min = std::max<int>(
                            min_attr_lvls[4],
                            sum_ijkl - adjusted_max_attr_lvls[5] - adjusted_max_attr_lvls[6] - adjusted_max_attr_lvls[7]
                        );

                        const auto m_max = std::min<int>(
                            adjusted_max_attr_lvls[4],
                            sum_ijkl - min_attr_lvls[5] - min_attr_lvls[6] - min_attr_lvls[7]
                        );

                        for (m = m_min; std::cmp_less_equal(m, m_max); ++m)
                        {
                            const auto sum_ijklm = sum_ijkl - (int)m;

                            // n must leave enough room for o and p.
                            const auto n_min = std::max<int>(
                                min_attr_lvls[5], sum_ijklm - adjusted_max_attr_lvls[6] - adjusted_max_attr_lvls[7]
                            );

                            const auto n_max = std::min<int>(
                                adjusted_max_attr_lvls[5],
                                sum_ijklm - min_attr_lvls[6] - min_attr_lvls[7]
                            );

                            for (n = n_min; std::cmp_less_equal(n, n_max); ++n)
                            {
                                const auto remaining = sum_ijklm - (int)n;

                                // Instead of looping over o AND p:
                                //   o + p == remaining
                                // o is constrained by both its own range and the range available to p.
                                const auto o_min = std::max<int>(
                                    min_attr_lvls[6],
                                    remaining - adjusted_max_attr_lvls[7]
                                );

                                const auto o_max = std::min<int>(
                                    adjusted_max_attr_lvls[6],
                                    remaining - min_attr_lvls[7]
                                );

                                for (o = o_min; std::cmp_less_equal(o, o_max); ++o)
                                {
                                    // p is uniquely determined.
                                    p = remaining - (int)o;

                                    *current_it++ = result;
                                }
                            }
                        }
                    }
                }
            }
        }


        if (current_it != stat_variations.end())
            throw std::runtime_error(std::format(
                "Mismatch in expected ({}) and actual ({}) number of stat variations generated.\n"
                    "free_attribute_points: {}\n"
                    "min_attr_lvls: {}\n"
                    "max_attr_lvls: {}\n"
                    "adjusted_max_attr_lvls: {}\n"
                    "varied_attributes: {}",
                possible_occurances,
                std::distance(stat_variations.begin(), current_it),
                free_attribute_points,
                min_attr_lvls,
                max_attr_lvls,
                adjusted_max_attr_lvls,
                varied_attributes
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
                    (best_stats && best_value == new_value && character_level(*best_stats) > character_level(attack.stats)))
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
            const AttributeLevels &min_attr_lvls,
            int max_attr_lvl,
            bool optimize_starting_class
        ) : attack_options{ attack_options }
        {
            auto max_attr_lvls = make_filled_array<AttributeLevels>(max_attr_lvl);

            if (optimize_starting_class)
            {
                std::println("{}", starting_class::get_min_stats(min_attr_lvls));
                this->stat_variations = starting_class::get_stat_variations(
                    max_attribute_points,
                    starting_class::get_min_stats(min_attr_lvls),
                    max_attr_lvls,
                    default_varied_attributes
                );
            }
            else
            {
                this->stat_variations = get_stat_variations(
                    max_attribute_points,
                    min_attr_lvls,
                    max_attr_lvls,
                    default_varied_attributes
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
        static AttributeLevels get_optimized_max_attr_lvls(
            const AttributeLevels &min_attr_lvls,
            const AttributeLevels& max_attr_lvls,
            const NonscalingAttributes& nonscaling_attributes
        )
        {
            AttributeLevels optimized_max_attr_lvls{};
            for (auto&& [
                min_attr_lvl,
                max_attr_lvl,
                nonscaling_attribute,
                optimized_max_attr_lvl
            ] : std::views::zip(
                relevant_attribute_levels(min_attr_lvls),
                relevant_attribute_levels(max_attr_lvls),
                nonscaling_attributes,
                relevant_attribute_levels(optimized_max_attr_lvls)
            ))
                optimized_max_attr_lvl = nonscaling_attribute ? min_attr_lvl : max_attr_lvl;
            return optimized_max_attr_lvls;
        }

        std::map<NonscalingAttributes, std::vector<AttributeLevels>> optimized_stat_variations_map{};
        AttackOptions attack_options;

        V2(
            std::ranges::sized_range auto&& weapons,
            const AttackOptions& attack_options,
            int max_attribute_points,
            const AttributeLevels &min_attr_lvls,
            int max_attr_lvl,
            bool optimize_starting_class
        ) : attack_options{ attack_options }
        {
            auto max_attr_lvls = make_filled_array<AttributeLevels>(max_attr_lvl);

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
                            starting_class::get_min_stats(min_attr_lvls),
                            get_optimized_max_attr_lvls(min_attr_lvls, max_attr_lvls, nonscaling_attributes),
                            default_varied_attributes
                        );
                        // static_assert(false, "get_optimized_max_attr_lvls needs to be adjusted for optimize_starting_class");
                    }
                    else
                    {
                        optimized_stat_variations = get_stat_variations(
                            max_attribute_points,
                            min_attr_lvls,
                            get_optimized_max_attr_lvls(min_attr_lvls, max_attr_lvls, nonscaling_attributes),
                            default_varied_attributes
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

