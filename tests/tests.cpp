#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>


import std;
import BS.thread_pool;
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


TEST_CASE("verify stat variations correctness") {
    auto expected_stat_variation_count = 1365;

    auto stat_variation_count = calculator::get_stat_variation_count(
        91,
        calculator::character_class_stats.at("wretch")
    );
    CHECK(stat_variation_count == expected_stat_variation_count);
    
    auto stat_variations = calculator::get_stat_variations(
        91,
        calculator::character_class_stats.at("wretch")
    );
    CHECK(stat_variations.size() == expected_stat_variation_count);
}

TEST_CASE("verify total attack rating optimization correctness") {
    std::vector<calculator::FullStats> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("calculator::get_stat_variations")
#endif
    {
        stat_variations = calculator::get_stat_variations(
            91,
            calculator::character_class_stats.at("wretch")
        );
    };

    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    BS::thread_pool<> thread_pool{ 1 };
    std::vector<calculator::AttackRating> attack_ratings{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::OptimizationContext")
#endif
    {
        attack_ratings = optimizer::optimizers<optimizer::Target::TOTAL_ATTACK_POWER>(weapons, stat_variations, attack_options, thread_pool).get();
    };
    std::ranges::sort(attack_ratings, {}, optimizer::optimizers<optimizer::Target::TOTAL_ATTACK_POWER>.projection);
    auto&& attack_rating = attack_ratings.back();

    CHECK(attack_rating.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack_rating.full_stats == calculator::FullStats{ 10, 10, 10, 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack_rating.total_attack_power.at(1),
        Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9)
    );
}

TEST_CASE("verify total attack rating calculation correctness 1")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::FullStats full_stats{ 10, 10, 10, 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return w.calculate_attack_rating(attack_options, full_stats).total_attack_power.at(1); })
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_1.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_1))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("verify total attack rating calculation correctness 2")
{
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::FullStats full_stats{};
    full_stats.fill(70);

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return w.calculate_attack_rating(attack_options, full_stats).total_attack_power.at(1); })
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}