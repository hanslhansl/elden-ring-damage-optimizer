#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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

TEST_CASE("optimize total attack rating") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    auto stat_variations = calculator::get_stat_variations(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));

    auto filtered_weapons = xml::apply_filter(weapons, calculator::Weapon::Filter{{}, {}, {}});

    auto attack_rating = optimizer::OptimizationContext(
        0,
        stat_variations,
        filtered_weapons,
        attack_options,
        std::type_identity<calculator::AttackRating::total>{}
    ).wait_and_get_result();

    REQUIRE(attack_rating.stats == calculator::Stats{ 21, 10, 10, 10, 10 });
    REQUIRE(attack_rating.weapon->full_name == "Fire Duelist Greataxe");

    auto expected = 734.8908832256299;
    REQUIRE_THAT(attack_rating.total_attack_power.at(2), WithinAbs(expected, 1e-12) || WithinRel(expected, 1e-9));
}

const std::vector<double> expected_total_attack_powers {
    #include "excpected_total_attack_powers.inc"
};


TEST_CASE("check all weapons total attack rating") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons | std::views::transform([&](const calculator::Weapon& w){
        calculator::AttackRating::total attack_rating{};
        w.get_attack_rating(attack_options, stats, attack_rating);
        return attack_rating.total_attack_power;
    }) | std::ranges::to<std::vector>();


    REQUIRE(total_attack_powers.size() == expected_total_attack_powers.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers))
        CHECK_THAT(actual, WithinAbs(expected, 1e-12) || WithinRel(expected, 1e-9));
}