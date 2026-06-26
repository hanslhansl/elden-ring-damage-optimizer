#include <catch2/catch_test_macros.hpp>
import std;
import erdo;

TEST_CASE("addition")
{
    auto xml_data_directory = std::filesystem::current_path().parent_path() / "xml_data";

    auto weapon_contain = xml::WeaponContainer(xml_data_directory);

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    auto stat_variations = calculator::get_stat_variations(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));

    auto filtered_weapons = weapon_contain.apply_filter(calculator::Weapon::Filter{{}, {}, {}});

    auto attack_rating = calculator::OptimizationContext(
        10,
        stat_variations,
        filtered_weapons,
        attack_options,
        std::type_identity<calculator::AttackRating::total>{}
    ).wait_and_get_result();

    REQUIRE(attack_rating.stats == calculator::Stats{ 21, 10, 10, 10, 10 });
    REQUIRE(attack_rating.weapon->full_name == "Fire Duelist Greataxe");
    REQUIRE(attack_rating.total_attack_power.at(2) == 734.8908832256299);
}