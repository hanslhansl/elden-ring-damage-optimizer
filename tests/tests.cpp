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
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::Attack::calculate(w, stats, attack_options); })
        | std::views::transform(optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>)
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
        | std::views::transform([&](const calculator::Weapon& w){ return calculator::Attack::calculate(w, stats, attack_options); })
        | std::views::transform(optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>)
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("stat variations")
{
    auto expected_stat_variation_count = 1365;

    auto stat_variation_count = optimizer::get_stat_variation_count(
        calculator::character_class_stats.at("wretch"),
        91
    );
    CHECK(stat_variation_count == expected_stat_variation_count);
    
    std::vector<calculator::Stats> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::get_stat_variations")
#endif
    {
        stat_variations = optimizer::get_stat_variations(
            calculator::character_class_stats.at("wretch"),
            91
        );
    };

    CHECK(stat_variations.size() == expected_stat_variation_count);
}

TEST_CASE("optimization - brute force - total attack power")
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::Attack> attacks{};
    constexpr auto optimizer = optimizer::brute_force<optimizer::Target::TOTAL_ATTACK_POWER>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attacks = optimizer.run_synchronously(weapons, attack_options, calculator::character_class_stats.at("wretch"), 91);
    };
    std::ranges::sort(attacks, {}, optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>);
    auto&& attack = attacks.back();

    CHECK(attack.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack.stats == calculator::Stats{ 10, 10, 10, 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack.total_attack_power.at(1),
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}

TEST_CASE("optimization - brute force - spell scaling")
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::Attack> attacks{};
    constexpr auto optimizer = optimizer::brute_force<optimizer::Target::SPELL_SCALING>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attacks = optimizer.run_synchronously(weapons, attack_options, calculator::character_class_stats.at("wretch"), 91);
    };
    std::ranges::sort(attacks, {}, optimizer::projection<optimizer::Target::SPELL_SCALING>);
    auto&& attack = attacks.back();

    CHECK(attack.weapon.get().full_name == "Demi-Human Queen's Staff");
    CHECK(attack.stats == calculator::Stats{ 10, 10, 10, 10, 10, 21, 10, 10 });

    auto expected = 1.9225000000000001;
    CHECK_THAT(attack.spell_scaling,
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}

TEST_CASE("optimization - v2 - total attack power")
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::Attack> attacks{};
    constexpr auto optimizer = optimizer::v2<optimizer::Target::TOTAL_ATTACK_POWER>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attacks = optimizer.run_synchronously(weapons, attack_options, calculator::character_class_stats.at("wretch"), 91);
    };
    std::ranges::sort(attacks, {}, optimizer::projection<optimizer::Target::TOTAL_ATTACK_POWER>);
    auto&& attack = attacks.back();

    CHECK(attack.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack.stats == calculator::Stats{ 10, 10, 10, 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack.total_attack_power.at(1),
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}

TEST_CASE("optimization - v2 - spell scaling")
{
    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    std::vector<calculator::Attack> attacks{};
    constexpr auto optimizer = optimizer::v2<optimizer::Target::SPELL_SCALING>;
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer.run_synchronously")
#endif
    {
        attacks = optimizer.run_synchronously(weapons, attack_options, calculator::character_class_stats.at("wretch"), 91);
    };
    std::ranges::sort(attacks, {}, optimizer::projection<optimizer::Target::SPELL_SCALING>);
    auto&& attack = attacks.back();

    CHECK(attack.weapon.get().full_name == "Demi-Human Queen's Staff");
    CHECK(attack.stats == calculator::Stats{ 10, 10, 10, 10, 10, 21, 10, 10 });

    auto expected = 1.9225000000000001;
    CHECK_THAT(attack.spell_scaling,
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}