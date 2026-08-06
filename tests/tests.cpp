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
        data_reference.reserve(data.size());
        data_reference.append_range(data);
    });

    return data_reference;
}

const std::vector<double> expected_total_attack_powers_1 {
    #include "excpected_total_attack_powers_1.inc"
};

const std::vector<double> expected_total_attack_powers_2 {
    #include "excpected_total_attack_powers_2.inc"
};


TEST_CASE("calculation - total attack power 1")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 10, 10, 10, 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::AttackRating::calculate(w, stats, attack_options); })
        | std::views::transform(calculator::optimizers<calculator::OptimizationTarget::TOTAL_ATTACK_POWER>.projection)
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_1.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_1))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("calculation - total attack power 2")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{};
    stats.fill(70);

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::AttackRating::calculate(w, stats, attack_options); })
        | std::views::transform(calculator::optimizers<calculator::OptimizationTarget::TOTAL_ATTACK_POWER>.projection)
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("stat variations")
{
    auto expected_stat_variation_count = 1365;

    auto stat_variation_count = calculator::get_stat_variation_count(
        91,
        calculator::character_class_stats.at("wretch")
    );
    CHECK(stat_variation_count == expected_stat_variation_count);
    
    std::vector<calculator::Stats> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("calculator::get_stat_variations")
#endif
    {
        stat_variations = calculator::get_stat_variations(
            91,
            calculator::character_class_stats.at("wretch")
        );
    };

    CHECK(stat_variations.size() == expected_stat_variation_count);
}

TEST_CASE("optimization - total attack power")
{
    auto stat_variations = calculator::get_stat_variations(
        91,
        calculator::character_class_stats.at("wretch")
    );
    REQUIRE(stat_variations.size() == 1365);

    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::AttackRating> attack_ratings{};
    constexpr auto optimizer = calculator::optimizers<calculator::OptimizationTarget::TOTAL_ATTACK_POWER>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attack_ratings = optimizer.run_synchronously(weapons, stat_variations, attack_options);
    };
    std::ranges::sort(attack_ratings, {}, optimizer.projection);
    auto&& attack_rating = attack_ratings.back();

    CHECK(attack_rating.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack_rating.stats == calculator::Stats{ 10, 10, 10, 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack_rating.total_attack_power.at(1),
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}

TEST_CASE("optimization - spell scaling")
{
    auto stat_variations = calculator::get_stat_variations(
        91,
        calculator::character_class_stats.at("wretch")
    );
    REQUIRE(stat_variations.size() == 1365);

    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::AttackRating> attack_ratings{};
    constexpr auto optimizer = calculator::optimizers<calculator::OptimizationTarget::SPELL_SCALING>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attack_ratings = optimizer.run_synchronously(weapons, stat_variations, attack_options);
    };
    std::ranges::sort(attack_ratings, {}, optimizer.projection);
    auto&& attack_rating = attack_ratings.back();

    CHECK(attack_rating.weapon.get().full_name == "Demi-Human Queen's Staff");
    CHECK(attack_rating.stats == calculator::Stats{ 10, 10, 10, 10, 10, 21, 10, 10 });

    auto expected = 1.9225000000000001;
    CHECK_THAT(attack_rating.spell_scaling,
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}