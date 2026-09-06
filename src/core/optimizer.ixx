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

namespace erdo::optimizer
{
    export std::size_t get_stat_variation_count(const int free_attribute_points, const RelevantAttributeLevels& min_relevant_stats, const RelevantAttributeLevels& max_relevant_stats)
    {
        if (free_attribute_points < 0)
            throw std::invalid_argument("free_attribute_points must be >= 0.");

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
    export std::vector<AttributeLevels> get_stat_variations(const int free_attribute_points, const AttributeLevels &min_stats, const RelevantAttributeLevels& max_relevant_stats)
    {
        auto min_relevant_stats = min_stats.relevant_stats();

        const int min_relevant_sum = std::ranges::fold_left(min_relevant_stats, 0, std::plus<int>{});
        const int max_relevant_sum = std::ranges::fold_left(max_relevant_stats, 0, std::plus<int>{});
        const int SUM = min_relevant_sum + std::min(free_attribute_points, max_relevant_sum - min_relevant_sum);

        auto possible_occurances = get_stat_variation_count(free_attribute_points, min_relevant_stats, max_relevant_stats);

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
                "Mismatch in expected ({}) and actual ({}) number of stat variations generated.\nmin_stats: {}, free_attribute_points: {}",
                possible_occurances,
                std::distance(stat_variations.begin(), current_it),
                min_stats,
                free_attribute_points
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
    concept valid_optimizer_target = requires { sizeof(Projection<target>); sizeof(EfficientCalculateAttack<target>); };

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
            auto best_value = std::numeric_limits<double>::lowest();

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

        BruteForce(std::ranges::sized_range auto&& weapons, const AttackOptions& attack_options, int free_attribute_points, const AttributeLevels &min_stats, int max_stat)
            : attack_options{ attack_options }
        {
            this->stat_variations = get_stat_variations(
                free_attribute_points,
                min_stats,
                make_filled_array<RelevantAttributeLevels>(max_stat)
            );
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
            const int free_attribute_points,
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

        static std::vector<AttributeLevels> get_optimized_stat_variations(
            const int free_attribute_points,
            const AttributeLevels &min_stats,
            const RelevantAttributeLevels& max_relevant_stats,
            const NonscalingAttributes& nonscaling_attributes
        )
        {
            auto optimized_max_relevant_stats = get_optimized_max_relevant_stats(
                free_attribute_points, min_stats.relevant_stats(), max_relevant_stats, nonscaling_attributes
            );

            return get_stat_variations(free_attribute_points, min_stats, optimized_max_relevant_stats);
        }

        std::map<NonscalingAttributes, std::vector<AttributeLevels>> optimized_stat_variations_map{};
        AttackOptions attack_options;

        V2(std::ranges::sized_range auto&& weapons, const AttackOptions& attack_options, int free_attribute_points, const AttributeLevels &min_stats, int max_stat)
            : attack_options{ attack_options }
        {
            auto min_relevant_stats = min_stats.relevant_stats();
            auto max_relevant_stats = make_filled_array<RelevantAttributeLevels>(max_stat);

            if (get_stat_variation_count(free_attribute_points, min_relevant_stats, max_relevant_stats) != 0)
            {
                for (auto&& weapon : weapons)
                {
                    auto&& nonscaling_attributes = std::invoke(&Weapon::nonscaling_attributes, weapon);
                    auto [it, inserted] = this->optimized_stat_variations_map.try_emplace(nonscaling_attributes);
                    auto&& optimized_stat_variations = it->second;
                    if (inserted)
                        optimized_stat_variations = V2::get_optimized_stat_variations(free_attribute_points, min_stats, max_relevant_stats, nonscaling_attributes);
                    this->iteration_count += optimized_stat_variations.size();
                }
            }
        }

        Attack operator()(const Weapon& weapon) const
        {
            auto&& optimized_stat_variations = this->optimized_stat_variations_map.at(weapon.nonscaling_attributes);
            return this->optimize_weapon(weapon, attack_options, optimized_stat_variations);
        }
    };
}

