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

const std::vector<double> excpected_calculation_total_attack_power_1 {
    #include "excpected_calculation_total_attack_power_1.inc"
};
const std::vector<double> excpected_calculation_total_attack_power_2 {
    #include "excpected_calculation_total_attack_power_2.inc"
};
const std::vector<double> excpected_optimization_total_attack_power {
    #include "excpected_optimization_total_attack_power.inc"
};
const std::vector<double> excpected_optimization_spell_scaling {
    #include "excpected_optimization_spell_scaling.inc"
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

    REQUIRE(total_attack_powers.size() == excpected_calculation_total_attack_power_1.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, excpected_calculation_total_attack_power_1))
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

    REQUIRE(total_attack_powers.size() == excpected_calculation_total_attack_power_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, excpected_calculation_total_attack_power_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

auto test_stat_variation_count(
    const int expected_stat_variation_count,
    const calculator::AttributeLevels& min_stats,
    const calculator::RelevantAttributeLevels& max_relevant_stats,
    const int free_attribute_points
)
{
    const auto min_relevant_stats = min_stats.relevant_stats();

    std::size_t stat_variation_count;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::get_stat_variation_count")
#endif
    {
        stat_variation_count = optimizer::get_stat_variation_count(
            free_attribute_points,
            min_relevant_stats,
            max_relevant_stats
        );
    };
    REQUIRE(stat_variation_count == expected_stat_variation_count);
    
    std::vector<calculator::AttributeLevels> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::get_stat_variations")
#endif
    {
        stat_variations = optimizer::get_stat_variations(
            free_attribute_points,
            min_stats,
            max_relevant_stats
        );
    };
    REQUIRE(stat_variations.size() == expected_stat_variation_count);
    return stat_variations;
}
TEST_CASE("stat variation count 1")
{
    test_stat_variation_count(
        1365,
        {},
        make_filled_array<calculator::RelevantAttributeLevels>(calculator::attribute_level_limit),
        11
    );
}
TEST_CASE("stat variation count 2 - lower edge case")
{
    test_stat_variation_count(
        1,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        0
    );
}
TEST_CASE("stat variation count 3 - single point")
{
    test_stat_variation_count(
        5,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        1
    );
}
TEST_CASE("stat variation count 4 - two points")
{
    test_stat_variation_count(
        15,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        2
    );
}
TEST_CASE("stat variation count 5")
{
    test_stat_variation_count(
        71,
        {0, 0, 0, 0, 0, 0, 0, 0},
        std::array<unsigned int, 5>{1, 2, 3, 4, 5},
        5
    );
}
TEST_CASE("stat variation count 6 - exactly upper edge case")
{
    test_stat_variation_count(
        1,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        50
    );
}
TEST_CASE("stat variation count 7 - above upper edge case")
{
    test_stat_variation_count(
        1,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        51
    );
}
TEST_CASE("stat variation count 8")
{
    test_stat_variation_count(
        15,
        {0, 0, 0, 10, 20, 30, 40, 50},
        std::array<unsigned int, 5>{12, 22, 32, 42, 52},
        2
    );
}
TEST_CASE("stat variation count 9 - irrelevant attributes")
{
    test_stat_variation_count(
        15,
        {100, 200, 300, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        2
    );

    test_stat_variation_count(
        15,
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        2
    );
}
TEST_CASE("stat variation count 10")
{
    test_stat_variation_count(
        1,
        {0, 0, 0, 10, 20, 30, 40, 50},
        std::array<unsigned int, 5>{12, 22, 32, 42, 52},
        11
    );
}

void test_stat_variations(
    const int expected_stat_variation_count,
    std::vector<calculator::AttributeLevels> expected_stat_variations,
    const calculator::AttributeLevels& min_stats,
    const calculator::RelevantAttributeLevels& max_relevant_stats,
    const int free_attribute_points
)
{
    auto stat_variations = test_stat_variation_count(
        expected_stat_variation_count,
        min_stats,
        max_relevant_stats,
        free_attribute_points
    );
    REQUIRE(stat_variations == expected_stat_variations);
}
TEST_CASE("stat variations - single point")
{
    test_stat_variations(
        5,
        {
            {0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 1, 0},
            {0, 0, 0, 0, 0, 1, 0, 0},
            {0, 0, 0, 0, 1, 0, 0, 0},
            {0, 0, 0, 1, 0, 0, 0, 0}
        },
        {0, 0, 0, 0, 0, 0, 0, 0},
        make_filled_array<calculator::RelevantAttributeLevels>(10),
        1
    );
}

template<typename Optimizer>
void test_optimization(std::string_view expected_weapon_full_name, const calculator::AttributeLevels& expected_stats, const std::vector<double>& expected_values)
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    const auto min_stats = calculator::get_character_class_attributes("Wretch");
    const auto min_relevant_stats = min_stats.relevant_stats();
    const auto free_attribute_points = 11;
    const auto max_stat = 99;

    std::vector<calculator::Attack> attacks{};
    Optimizer optimizer{weapons, attack_options, free_attribute_points, min_stats, max_stat};
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
        excpected_optimization_total_attack_power
    );
}

TEST_CASE("optimization - brute force - spell scaling")
{
    test_optimization<optimizer::BruteForce<optimizer::Target::SPELL_SCALING>>(
        "Demi-Human Queen's Staff",
        { 10, 10, 10, 10, 10, 21, 10, 10 },
        excpected_optimization_spell_scaling
    );
}

TEST_CASE("optimization - v2 - total attack power")
{
    test_optimization<optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER>>(
        "Fire Duelist Greataxe",
        { 10, 10, 10, 21, 10, 10, 10, 10 },
        excpected_optimization_total_attack_power
    );
}

TEST_CASE("optimization - v2 - spell scaling")
{
    test_optimization<optimizer::V2<optimizer::Target::SPELL_SCALING>>(
        "Demi-Human Queen's Staff",
        { 10, 10, 10, 10, 10, 21, 10, 10 },
        excpected_optimization_spell_scaling
    );
}