#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

import std;
import erdo;



using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;


const std::vector<calculator::Weapon>& get_weapons() {
    static std::once_flag flag;
    static std::vector<calculator::Weapon> data;

    std::call_once(flag, [] {
        auto xml_data_directory = std::filesystem::current_path() / "test_xml_data";
        data = xml::get_weapons(xml_data_directory);
    });

    return data;
}

const std::vector<double> expected_total_attack_powers {
    #include "excpected_total_attack_powers.inc"
};

TEST_CASE("verify total attack rating optimization correctness") {
    
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};

    std::vector<calculator::Stats> stat_variations{};
    BENCHMARK("calculator::get_stat_variations")
    {
        stat_variations = calculator::get_stat_variations(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));
    };

    std::vector<calculator::AttackRating> attack_ratings{};
    BENCHMARK("optimizer::OptimizationContext")
    {
        attack_ratings = optimizer::OptimizationContext(
            0,
            stat_variations,
            weapons,
            attack_options
        ).wait_and_get_result();
    };
    auto&& attack_rating = attack_ratings.front();

    REQUIRE(attack_rating.stats == calculator::Stats{ 21, 10, 10, 10, 10 });
    REQUIRE(attack_rating.weapon.get().full_name == "Fire Duelist Greataxe");

    auto expected = 734.8908832256299;
    REQUIRE_THAT(attack_rating.total_attack_power.at(2), WithinAbs(expected, 1e-12) || WithinRel(expected, 1e-9));
}

TEST_CASE("verify total attack rating calculation correctness") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons | std::views::transform([&](const calculator::Weapon& w){
        return w.get_attack_rating(attack_options, stats).total_attack_power.at(2);
    }) | std::ranges::to<std::vector>();


    REQUIRE(total_attack_powers.size() == expected_total_attack_powers.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers))
        CHECK_THAT(actual, WithinAbs(expected, 1e-12) || WithinRel(expected, 1e-9));
}