#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

import std;
import BS.thread_pool;
import erdo;


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


TEST_CASE("verify stat variations correctness") {
    auto expected_stat_variation_count = 1365;

    auto stat_variation_count = calculator::get_stat_variation_count(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));
    CHECK(stat_variation_count == expected_stat_variation_count);

    auto stat_variations = calculator::get_stat_variations(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));
    CHECK(stat_variations.size() == expected_stat_variation_count);
}

TEST_CASE("verify total attack rating optimization correctness") {
    std::vector<calculator::Stats> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("calculator::get_stat_variations")
#endif
    {
        stat_variations = calculator::get_stat_variations(1 + 60, calculator::ALL_CLASS_STATS.at(calculator::Class::WRETCH));
    };

    auto&& weapons = get_weapons();
    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    BS::thread_pool<> thread_pool{ 1 };
    std::vector<calculator::AttackRating> attack_ratings{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("optimizer::OptimizationContext")
#endif
    {
        attack_ratings = optimizer::optimize(stat_variations, weapons, attack_options, thread_pool).get();
    };
    auto&& attack_rating = attack_ratings.front();

    CHECK(attack_rating.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack_rating.stats == calculator::Stats{ 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack_rating.total_attack_power.at(2), Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("verify total attack rating calculation correctness") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons | std::views::transform([&](const calculator::Weapon& w){
        return w.get_attack_rating(attack_options, stats).total_attack_power.at(2);
    }) | std::ranges::to<std::vector>();


    CHECK(total_attack_powers.size() == expected_total_attack_powers.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}