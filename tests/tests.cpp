#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

import std;
import BS.thread_pool;
import erdo;

using namespace erdo;

const std::vector<calculator::Weapon>& get_weapons() {
    static std::once_flag flag;
    static std::vector<calculator::Weapon> data;

    std::call_once(flag, [] {
        auto xml_data_directory = std::filesystem::current_path() / "xml_data" / "11611000";
        data = parser::load_weapons(xml_data_directory);
    });

    return data;
}

const std::vector<double> expected_total_attack_powers_1 {
    #include "excpected_total_attack_powers_1.inc"
};

const std::vector<double> expected_total_attack_powers_2 {
    #include "excpected_total_attack_powers_2.inc"
};



static std::vector<std::string> make_universe()
{
    auto&& weapons = get_weapons();

    return weapons | std::views::transform([](const calculator::Weapon& w){ return w.base_name; })
        | std::ranges::to<std::vector>();
}


static std::vector<std::string> make_needles(
    const std::vector<std::string>& universe,
    const std::vector<bool>& in_haystack,
    double hit_rate,
    size_t count)
{
    std::vector<size_t> hits;
    std::vector<size_t> misses;

    for (size_t i = 0; i < universe.size(); ++i) {
        if (in_haystack[i])
            hits.push_back(i);
        else
            misses.push_back(i);
    }

    std::mt19937 rng(42);

    std::bernoulli_distribution hit(hit_rate);

    std::uniform_int_distribution<size_t> hit_dist(
        0, hits.size() - 1);

    std::uniform_int_distribution<size_t> miss_dist(
        0, misses.size() - 1);

    std::vector<std::string> needles;
    needles.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        if (hit(rng))
            needles.push_back(universe[hits[hit_dist(rng)]]);
        else
            needles.push_back(universe[misses[miss_dist(rng)]]);
    }

    return needles;
}

TEST_CASE("contains_lookup")
{
    auto universe = make_universe();

    size_t haystack_size = universe.size() / 2;

    std::vector<bool> present(universe.size());
    for (size_t i = 0; i < haystack_size; ++i)
        present[i] = true;


    auto needles = make_needles(
        universe,
        present,
        0.5,       // 50% hits
        1000000
    );

    std::println("universe size: {}, haystack size: {}, needles size: {}", universe.size(), haystack_size, needles.size());

    std::vector<std::string> haystack_vector(universe.begin(), universe.begin() + haystack_size);
    BENCHMARK("vector")
    {
        size_t found = 0;

        for (auto& needle : needles)
            found += std::find( haystack_vector.begin(), haystack_vector.end(), needle) != haystack_vector.end();

        return found;
    };

    std::set<std::string> haystack_set(universe.begin(), universe.begin() + haystack_size);
    BENCHMARK("set")
    {
        size_t found = 0;

        for (auto& needle : needles)
            found += haystack_set.contains(needle);

        return found;
    };

    std::unordered_set<std::string> haystack_unordered_set(universe.begin(), universe.begin() + haystack_size);
    BENCHMARK("unordered_set")
    {
        size_t found = 0;

        for (auto& needle : needles)
            found += haystack_unordered_set.contains(needle);

        return found;
    };

    auto haystack_vector_binary_search = std::vector<std::string>(universe.begin(), universe.begin() + haystack_size);
    std::sort(haystack_vector_binary_search.begin(), haystack_vector_binary_search.end());

    BENCHMARK("vector binary_search")
    {
        size_t found = 0;

        for (const auto& needle : needles) {
            found += std::binary_search(haystack_vector_binary_search.begin(), haystack_vector_binary_search.end(), needle);
        }

        return found;
    };

    std::flat_set<std::string> haystack_flat_set(haystack_vector_binary_search.begin(), haystack_vector_binary_search.end());
    BENCHMARK("flat_set")
    {
        size_t found = 0;

        for (const auto& needle : needles) {
            found += haystack_flat_set.contains(needle);
        }

        return found;
    };



}



TEST_CASE("verify stat variations correctness") {
    auto expected_stat_variation_count = 1365;

    auto stat_variation_count = calculator::get_stat_variation_count(
        1 + 60,
        calculator::character_class_stats.at("wretch").to_stats()
    );
    CHECK(stat_variation_count == expected_stat_variation_count);
    
    auto stat_variations = calculator::get_stat_variations(
        1 + 60,
        calculator::character_class_stats.at("wretch").to_stats()
    );
    CHECK(stat_variations.size() == expected_stat_variation_count);
}

TEST_CASE("verify total attack rating optimization correctness") {
    std::vector<calculator::Stats> stat_variations{};
#ifdef ENABLE_BENCHMARKS
    BENCHMARK("calculator::get_stat_variations")
#endif
    {
        stat_variations = calculator::get_stat_variations(
            1 + 60,
            calculator::character_class_stats.at("wretch").to_stats()
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
        attack_ratings = optimizer::optimize<optimizer::optimize_weapon>(weapons, stat_variations, attack_options, thread_pool).get();
    };
    auto&& attack_rating = attack_ratings.front();

    CHECK(attack_rating.weapon.get().full_name == "Fire Duelist Greataxe");
    CHECK(attack_rating.stats == calculator::Stats{ 21, 10, 10, 10, 10 });

    auto expected = 734.8908832256299;
    CHECK_THAT(attack_rating.total_attack_power.at(1), Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("verify total attack rating calculation correctness 1") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return w.calculate_attack_rating(attack_options, stats).total_attack_power.at(1); })
        | std::ranges::to<std::vector>();

    CHECK(total_attack_powers.size() == expected_total_attack_powers_1.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_1))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}

TEST_CASE("verify total attack rating calculation correctness 2") {
    auto&& weapons = get_weapons();

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 70, 70, 70, 70, 70 };

    auto total_attack_powers = weapons
        | std::views::transform([&](const calculator::Weapon& w){ return w.calculate_attack_rating(attack_options, stats).total_attack_power.at(1); })
        | std::ranges::to<std::vector>();

    // std::println("{}", total_attack_powers);

    CHECK(total_attack_powers.size() == expected_total_attack_powers_2.size());

    for (auto [actual, expected] : std::views::zip(total_attack_powers, expected_total_attack_powers_2))
        CHECK_THAT(actual, Catch::Matchers::WithinAbs(expected, 1e-12) || Catch::Matchers::WithinRel(expected, 1e-9));
}