#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>


import std;
import erdo;

using namespace erdo;

const auto& get_weapons()
{
    static std::once_flag flag;
    static std::vector<calculator::Weapon> data;
    static std::vector<std::reference_wrapper<const calculator::Weapon>> data_reference;

    std::call_once(flag, [] {
        auto xml_data_directory = std::filesystem::current_path() / "xml_data" / "11611000";
        data = parser::load_weapons(xml_data_directory);
        std::ranges::sort(data, {}, &calculator::Weapon::full_name);
        data_reference.reserve(data.size());
        data_reference.append_range(data);
    });

    return data_reference;
}

const std::vector<double> expected_calculation_total_attack_power_1 {
    #include "expected_calculation_total_attack_power_1.inc"
};
const std::vector<double> expected_calculation_total_attack_power_2 {
    #include "expected_calculation_total_attack_power_2.inc"
};
const std::vector<double> expected_optimization_total_attack_power {
    #include "expected_optimization_total_attack_power.inc"
};
const std::vector<double> expected_optimization_spell_scaling {
    #include "expected_optimization_spell_scaling.inc"
};
const std::vector<calculator::AttributeLevels> expected_stat_variations {
    #include "expected_stat_variations.inc"
};


TEST_CASE("calculation - total attack power 1")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::AttributeLevels stats{ 10, 10, 10, 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::Attack::calculate(w, stats, attack_options); })
        | std::views::transform(optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>)
        | std::ranges::to<std::vector>();

    REQUIRE(total_attack_powers.size() == expected_calculation_total_attack_power_1.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_calculation_total_attack_power_1))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("calculation - total attack power 2")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::AttributeLevels stats{};
    stats.fill(70);

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::Attack::calculate(w, stats, attack_options); })
        | std::views::transform(optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>)
        | std::ranges::to<std::vector>();

    REQUIRE(total_attack_powers.size() == expected_calculation_total_attack_power_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_calculation_total_attack_power_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

auto test_stat_variation_count(
    const int max_attribute_points,
    const calculator::AttributeLevels& min_stats,
    const calculator::AttributeLevels& max_stats,
    const int expected_stat_variation_count
)
{
    optimizer::VariedAttributes varied_attributes{ false, false, false, true, true, true, true, true };

    std::size_t stat_variation_count;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::get_stat_variation_count")
#endif
    {
        stat_variation_count = optimizer::get_stat_variation_count(
            max_attribute_points,
            min_stats,
            max_stats,
            varied_attributes
        );
    };
    REQUIRE(stat_variation_count == expected_stat_variation_count);
    
    std::vector<calculator::AttributeLevels> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::get_stat_variations")
#endif
    {
        stat_variations = optimizer::get_stat_variations(
            max_attribute_points,
            min_stats,
            max_stats,
            varied_attributes
        );
    };
    REQUIRE(stat_variations.size() == expected_stat_variation_count);
    return stat_variations;
}
TEST_CASE("stat variation count 1")
{
    test_stat_variation_count(
        11,
        {},
        make_filled_array<calculator::AttributeLevels>(calculator::attribute_level_limit),
        1365
    );
}
TEST_CASE("stat variation count 2 - lower edge case")
{
    test_stat_variation_count(
        0,
        {},
        make_filled_array<calculator::AttributeLevels>(10),
        1
    );
}
TEST_CASE("stat variation count 3 - single point")
{
    test_stat_variation_count(
        1,
        {},
        make_filled_array<calculator::AttributeLevels>(10),
        5
    );
}
TEST_CASE("stat variation count 4 - two points")
{
    test_stat_variation_count(
        2,
        {},
        make_filled_array<calculator::AttributeLevels>(10),
        15
    );
}
TEST_CASE("stat variation count 5")
{
    test_stat_variation_count(
        5,
        {},
        {0, 0, 0, 1, 2, 3, 4, 5},
        71
    );
}
TEST_CASE("stat variation count 6 - exactly upper edge case")
{
    test_stat_variation_count(
        50,
        {},
        make_filled_array<calculator::AttributeLevels>(10),
        1
    );
}
TEST_CASE("stat variation count 7 - above upper edge case")
{
    test_stat_variation_count(
        51,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::AttributeLevels>(10),
        1
    );
}
TEST_CASE("stat variation count 8")
{
    test_stat_variation_count(
        152,
        {0, 0, 0, 10, 20, 30, 40, 50},
        {0, 0, 0, 12, 22, 32, 42, 52},
        15
    );
}
TEST_CASE("stat variation count 9 - irrelevant attributes")
{
    test_stat_variation_count(
        602,
        {100, 200, 300, 0, 0, 0, 0, 0},
        make_filled_array<calculator::AttributeLevels>(10),
        15
    );

    test_stat_variation_count(
        2,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::AttributeLevels>(10),
        15
    );
}
TEST_CASE("stat variation count 10")
{
    test_stat_variation_count(
        161,
        {0, 0, 0, 10, 20, 30, 40, 50},
        {0, 0, 0, 12, 22, 32, 42, 52},
        1
    );
}

void test_stat_variations(
    const int max_attribute_points,
    const calculator::AttributeLevels& min_stats,
    const calculator::AttributeLevels& max_stats,
    const int expected_stat_variation_count,
    std::vector<calculator::AttributeLevels> expected_stat_variations
)
{
    auto stat_variations = test_stat_variation_count(
        max_attribute_points,
        min_stats,
        max_stats,
        expected_stat_variation_count
    );
    REQUIRE(stat_variations == expected_stat_variations);
}
TEST_CASE("stat variations 1 - single point")
{
    test_stat_variations(
        1,
        {},
        make_filled_array<calculator::AttributeLevels>(10),
        5,
        {
            {0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 1, 0},
            {0, 0, 0, 0, 0, 1, 0, 0},
            {0, 0, 0, 0, 1, 0, 0, 0},
            {0, 0, 0, 1, 0, 0, 0, 0}
        }
    );
}
TEST_CASE("stat variations 2")
{
    test_stat_variations(
        80,
        {10, 10, 10, 0, 0, 0, 20, 10},
        make_filled_array<calculator::AttributeLevels>(99),
        10626,
        expected_stat_variations
    );
}
TEST_CASE("stat variations 3 - starting class algorithm")
{
    const int max_attribute_points = 80;
    const std::vector<calculator::AttributeLevels> min_stats = {{10, 10, 10, 0, 0, 0, 20, 10}};
    const auto max_stats = make_filled_array<calculator::RelevantAttributeLevels>(99);
    const int expected_stat_variation_count = 10626;

    std::size_t stat_variation_count;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::starting_class::get_stat_variation_count")
#endif
    {
        stat_variation_count = optimizer::starting_class::get_stat_variation_count(
            max_attribute_points,
            min_stats,
            max_stats
        );
    };
    REQUIRE(stat_variation_count == expected_stat_variation_count);
    
    std::vector<calculator::AttributeLevels> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::starting_class::get_stat_variations")
#endif
    {
        stat_variations = optimizer::starting_class::get_stat_variations(
            max_attribute_points,
            min_stats,
            max_stats
        );
    };
    REQUIRE(stat_variations.size() == expected_stat_variation_count);
    REQUIRE(stat_variations == expected_stat_variations);
}

template<typename Optimizer>
void test_optimization(std::string_view expected_weapon_full_name, const calculator::AttributeLevels& expected_stats, const std::vector<double>& expected_values)
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    const auto min_stats = calculator::get_character_starting_class_attributes("Wretch");
    const auto free_attribute_points = 91;

    std::vector<calculator::Attack> attacks{};
    Optimizer optimizer{weapons, attack_options, free_attribute_points, min_stats, calculator::attribute_level_limit, false};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attacks = optimizer.run_synchronously(weapons);
    };
    REQUIRE(attacks.size() == expected_values.size());

    for (auto&& [i, values] : std::views::zip(attacks, expected_values) | std::views::enumerate)
    {
        auto&& [attack, expected] = values;
        auto&& weapon = attack.weapon.get();
        CAPTURE(i);
        CAPTURE(weapon.full_name);
        CAPTURE(attack.stats);
        CAPTURE(min_stats);
        CHECK_THAT(
            optimizer.projection(attack),
            Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
        );
    }

    auto&& attack = std::ranges::max(attacks, {}, optimizer.projection);
    CHECK(attack.weapon.get().full_name == expected_weapon_full_name);
    CHECK(attack.stats == expected_stats);
}

TEST_CASE("optimization - brute force - total attack power")
{
    test_optimization<optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER>>(
        "Fire Duelist Greataxe",
        { 10, 10, 10, 21, 10, 10, 10, 10 },
        expected_optimization_total_attack_power
    );
}

TEST_CASE("optimization - brute force - spell scaling")
{
    test_optimization<optimizer::BruteForce<optimizer::Target::SPELL_SCALING>>(
        "Demi-Human Queen's Staff",
        { 10, 10, 10, 10, 10, 21, 10, 10 },
        expected_optimization_spell_scaling
    );
}

TEST_CASE("optimization - v2 - total attack power")
{
    test_optimization<optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER>>(
        "Fire Duelist Greataxe",
        { 10, 10, 10, 21, 10, 10, 10, 10 },
        expected_optimization_total_attack_power
    );
}

TEST_CASE("optimization - v2 - spell scaling")
{
    test_optimization<optimizer::V2<optimizer::Target::SPELL_SCALING>>(
        "Demi-Human Queen's Staff",
        { 10, 10, 10, 10, 10, 21, 10, 10 },
        expected_optimization_spell_scaling
    );
}