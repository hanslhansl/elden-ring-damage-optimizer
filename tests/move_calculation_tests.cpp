#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

import std;
import erdo;

using namespace erdo;
using namespace erdo::calculator;

namespace {
ResolvedHit make_hit(float physical, float magic, float bleed, int source = 0)
{
    ResolvedHit hit;
    hit.event = {30000, 1, 0, 0, source, .1, .2};
    hit.motionValues.known.fill(true);
    hit.motionValues.values[0] = physical;
    hit.motionValues.values[1] = magic;
    hit.motionValues.values[7] = bleed;
    return hit;
}

Weapon make_weapon()
{
    auto corrects = std::make_shared<AttackElementCorrects>();
    (*corrects)[0][0] = 1.; // Physical scales with Strength.
    (*corrects)[1][1] = 1.; // Magic and bleed scale with Dexterity.
    (*corrects)[7][1] = 1.;
    auto curve = std::make_shared<ScalingCurve>();
    for (std::size_t i = 0; i < curve->size(); ++i)
        (*curve)[i] = i / 100.;
    ScalingCurves curves;
    curves.fill(curve);
    Weapon weapon{
        .full_name = "Move fixture", .base_name = "Move fixture", .type = Weapon::Type::STRAIGHT_SWORD,
        .attribute_scalings_at_upgrade_levels = {{1, 1, 0, 0, 0}},
        .base_attack_powers_at_upgrade_levels = {{100, 100, 0, 0, 0, 0, 0, 50}},
        .attack_power_types_attribute_scalings = corrects,
        .attack_power_scaling_curves = curves,
        .scaling_tiers = std::make_shared<ScalingTiers>()
    };
    weapon.moveset = std::make_shared<Moveset>(23, 200);
    weapon.moveset->moves = {
        {MoveId::ONE_HAND_R1_1, "Two hits", 30000, {make_hit(100, 0, 50), make_hit(0, 200, 65)}},
        {MoveId::TWO_HAND_R1_1, "Two handed", 32000, {make_hit(100, 100, 100)}},
        {MoveId::POWERSTANCE_L1_1, "Powerstance", 34000,
            {make_hit(80, 80, 50, 2), make_hit(80, 80, 50, 2), make_hit(80, 80, 50, 1)}}
    };
    return weapon;
}
}

TEST_CASE("move calculation - per-hit powers and totals", "[moveset][unit][calculation]")
{
    AttackRating primary{};
    primary.attack_powers[0] = {200, 400};
    primary.attack_powers[1] = {50, 100};
    primary.attack_powers[7] = {40, 50};
    auto first = make_hit(80, 120, 65);
    first.behaviorParamId = 42;
    first.event.sourceMotionCategory = 23;
    const Move move{MoveId::ONE_HAND_R1_1, "Multi hit", 30000, {first, make_hit(50, 100, 100)}};
    const auto result = calculate_move(move, primary);
    REQUIRE(result.has_value());
    REQUIRE(result->hits.size() == 2);
    CHECK(result->hits[0].powers.attack_powers[0] == AttackPower{160, 320});
    CHECK(result->hits[0].powers.attack_powers[1] == AttackPower{60, 120});
    CHECK(result->hits[0].powers.attack_powers[7] == AttackPower{26, 32.5});
    CHECK(result->hits[0].powers.total_attack_power == AttackPower{220, 440});
    CHECK(result->total.total_attack_power == AttackPower{370, 740});
    CHECK(result->total.attack_powers[7] == AttackPower{66, 82.5});
    CHECK(result->selected_powers(0).total_attack_power == AttackPower{220, 440});
    CHECK(result->selected_powers().total_attack_power == AttackPower{370, 740});
    CHECK(result->hits[0].hit.behaviorParamId == 42);
    CHECK(result->hits[0].hit.event.sourceMotionCategory == 23);
    CHECK(result->hits[0].hit.event.startTime == .1);
    CHECK(primary.attack_powers[0] == AttackPower{200, 400});
    REQUIRE_THROWS_AS(result->selected_powers(2), std::out_of_range);
    const auto restored = json::read<MoveAttack>(json::write(*result));
    CHECK(restored.total.attack_powers == result->total.attack_powers);
    CHECK(restored.hits[0].hit.behaviorParamId == 42);
}

TEST_CASE("move calculation - sources and unsupported data", "[moveset][unit][calculation]")
{
    AttackRating primary{};
    AttackRating secondary{};
    primary.attack_powers[0] = {100, 200};
    secondary.attack_powers[0] = {300, 400};
    Move move{MoveId::POWERSTANCE_L1_1, "Two hands", 34000,
        {make_hit(80, 0, 0, 2), make_hit(80, 0, 0, 2), make_hit(80, 0, 0, 1)}};
    REQUIRE_FALSE(calculate_move(move, primary));
    const auto pair = calculate_move(move, primary, &secondary);
    REQUIRE(pair.has_value());
    CHECK(pair->hits[0].powers.total_attack_power[1] == 320);
    CHECK(pair->hits[1].powers.total_attack_power[1] == 320);
    CHECK(pair->hits[2].powers.total_attack_power[1] == 160);
    CHECK(pair->total.total_attack_power[1] == 800);

    SECTION("Unknown MV is not zero") {
        move.hits[0].motionValues.known[0] = false;
        const auto result = calculate_move(move, primary, &secondary);
        REQUIRE_FALSE(result);
        CHECK_THAT(result.error(), Catch::Matchers::ContainsSubstring("Unknown motion value"));
    }
    SECTION("Non-finite MV is rejected") {
        move.hits[0].motionValues.values[0] = std::numeric_limits<float>::infinity();
        REQUIRE_FALSE(calculate_move(move, primary, &secondary));
    }
    SECTION("Unknown source is not a default hand") {
        move.hits[0].event.source = 99;
        REQUIRE_FALSE(calculate_move(move, primary, &secondary));
    }
    SECTION("Source-less criticals retain raw MV semantics") {
        move.hits = {make_hit(294, 0, 0, -1)};
        REQUIRE_FALSE(calculate_move(move, primary));
        move.hits[0].event.eventType = 304;
        const auto result = calculate_move(move, primary);
        REQUIRE(result.has_value());
        CHECK_THAT(result->total.total_attack_power[1], Catch::Matchers::WithinAbs(588, 1e-10));
    }
    SECTION("Empty move is unsupported") {
        move.hits.clear();
        REQUIRE_FALSE(calculate_move(move, primary));
    }
}

TEST_CASE("move calculation - cached stances and all moves", "[moveset][unit][calculation]")
{
    auto weapon = make_weapon();
    const auto stats = get_character_starting_class_attributes("Wretch");
    const MoveEvaluator evaluator(weapon, stats, {0, 0, 0});
    const auto all = evaluator.evaluate_all();
    REQUIRE(all.size() == 3);
    REQUIRE(all[0].result.has_value());
    REQUIRE(all[1].result.has_value());
    REQUIRE_FALSE(all[2].result); // A second weapon must be supplied explicitly.
    CHECK_THAT(all[0].result->total.total_attack_power[1], Catch::Matchers::WithinAbs(330, 1e-10));
    CHECK_THAT(all[1].result->total.total_attack_power[1], Catch::Matchers::WithinAbs(225, 1e-10));
    CHECK_THAT(all[1].result->total.attack_powers[7][1], Catch::Matchers::WithinAbs(55, 1e-10));
    REQUIRE_FALSE(evaluator.evaluate(MoveId::ONE_HAND_R1_6));

    const MoveEvaluator pair(weapon, stats, {0, 0, 0}, &weapon);
    const auto paired = pair.evaluate(MoveId::POWERSTANCE_L1_1);
    REQUIRE(paired.has_value());
    CHECK_THAT(paired->total.total_attack_power[1], Catch::Matchers::WithinAbs(528, 1e-10));

    auto other = make_weapon();
    other.moveset->behaviorVariation = 999;
    const MoveEvaluator mixed(weapon, stats, {0, 0, 0}, &other);
    REQUIRE_FALSE(mixed.evaluate(MoveId::POWERSTANCE_L1_1));
    REQUIRE(mixed.evaluate(MoveId::ONE_HAND_R1_1).has_value());
    auto unsupported = make_weapon();
    unsupported.moveset.reset();
    CHECK(MoveEvaluator(unsupported, stats, {0, 0, 0}).evaluate_all().empty());
}

TEST_CASE("move calculation - paired weapon and PCBehavior powers", "[moveset][unit][calculation]")
{
    auto weapon = make_weapon();
    weapon.paired = true;
    auto pc = make_hit(150, 100, 65, -1);
    pc.event.eventType = 307;
    pc.event.attackIndex = -1;
    pc.event.pcBehaviorType = 16;
    pc.event.pcBehaviorCondition = 0;
    pc.event.pcBehaviorOffset = 0;
    weapon.moveset->moves.push_back({MoveId::ONE_HAND_RUNNING_R1, "PC attack", 30200, {pc}});
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    const std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    const MoveEvaluator single(weapon, stats, options.upgrade_levels);
    const auto one = Attack::calculate(weapon, stats, options);
    CHECK(Attack::calculate(weapon, stats, {{0, 0, 0}, true}).attack_powers == one.attack_powers);
    REQUIRE(single.evaluate(MoveId::TWO_HAND_R1_1));
    CHECK(single.evaluate(MoveId::TWO_HAND_R1_1)->total.total_attack_power == one.total_attack_power);
    REQUIRE_FALSE(single.evaluate(MoveId::POWERSTANCE_L1_1));
    REQUIRE(MoveEvaluator(weapon, stats, options.upgrade_levels, &weapon).evaluate(MoveId::POWERSTANCE_L1_1));
    const auto expected = single.evaluate(MoveId::ONE_HAND_RUNNING_R1);
    REQUIRE(expected);
    CHECK(expected->hits[0].hit.event.source == -1);
    CHECK(expected->hits[0].powers.attack_powers[0][1] == one.attack_powers[0][1] * 1.5);
    for (const auto id : {MoveId::TWO_HAND_R1_1, MoveId::ONE_HAND_RUNNING_R1}) {
        const auto powers = single.evaluate(id);
        REQUIRE(powers);
        for (const auto hit : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}}) {
            const optimizer::MoveObjective objective{{id, hit}};
            const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
                weapons, options, 80, stats, 99, false, objective};
            const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
                weapons, options, 80, stats, 99, false, objective};
            REQUIRE(brute.supports(weapon));
            REQUIRE(v2.supports(weapon));
            CHECK_THAT(brute.projection(brute(weapon)), Catch::Matchers::WithinAbs(powers->total.total_attack_power[1], 1e-10));
            CHECK_THAT(v2.projection(v2(weapon)), Catch::Matchers::WithinAbs(powers->total.total_attack_power[1], 1e-10));
        }
    }
    for (const auto member : {&AttackEvent::pcBehaviorType, &AttackEvent::pcBehaviorCondition,
            &AttackEvent::pcBehaviorOffset}) {
        auto invalid = pc;
        invalid.event.*member = 999;
        weapon.moveset->moves.back().hits = {invalid};
        CHECK_FALSE(single.evaluate(MoveId::ONE_HAND_RUNNING_R1));
        const optimizer::MoveObjective objective{{MoveId::ONE_HAND_RUNNING_R1, std::nullopt}};
        CHECK(optimizer::move_unsupported_reason(weapon, objective, optimizer::Target::TOTAL_ATTACK_POWER));
    }
}

TEST_CASE("move calculation - feint stances and optimizer scores", "[moveset][unit][calculation]")
{
    auto weapon = make_weapon();
    weapon.moveset->moves.clear();
    const std::array ids{MoveId::ONE_HAND_CHARGED_R2_1_FEINT, MoveId::ONE_HAND_CHARGED_R2_2_FEINT,
        MoveId::TWO_HAND_CHARGED_R2_1_FEINT, MoveId::TWO_HAND_CHARGED_R2_2_FEINT};
    for (auto id : ids) weapon.moveset->moves.push_back({id, "Feint", 30950, {make_hit(100, 100, 100)}});
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, true}; // Semantic move controls stance.
    const std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    const MoveEvaluator evaluator(weapon, stats, options.upgrade_levels);
    for (std::size_t i = 0; i < ids.size(); ++i) {
        CAPTURE(i);
        const auto evaluated = evaluator.evaluate(ids[i]);
        REQUIRE(evaluated);
        CHECK(move_uses_two_hands(ids[i]) == (i >= 2));
        CHECK_THAT(evaluated->total.total_attack_power[1], Catch::Matchers::WithinAbs(i >= 2 ? 225 : 220, 1e-10));
        for (const auto hit : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}}) {
            const optimizer::MoveObjective objective{{ids[i], hit}};
            const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
                weapons, options, 80, stats, 99, false, objective};
            const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
                weapons, options, 80, stats, 99, false, objective};
            REQUIRE(brute.supports(weapon));
            REQUIRE(v2.supports(weapon));
            const auto first = brute(weapon), second = v2(weapon);
            CHECK(first.two_handing == (i >= 2));
            CHECK(second.two_handing == (i >= 2));
            CHECK_THAT(brute.projection(first), Catch::Matchers::WithinAbs(evaluated->total.total_attack_power[1], 1e-10));
            CHECK_THAT(v2.projection(second), Catch::Matchers::WithinAbs(brute.projection(first), 1e-10));
        }
    }
}

TEST_CASE("move calculation - conditional activation", "[moveset][unit][calculation]")
{
    AttackRating rating{};
    rating.attack_powers[0] = {100, 200};
    rating.attack_powers[7] = {20, 40};
    Move move{MoveId::ONE_HAND_R1_1, "Conditional", 30000,
        {make_hit(100, 0, 50), make_hit(200, 0, 100), make_hit(50, 0, 25)}};
    move.hits[1].event.stateInfo = 187;
    move.hits[2].event.stateInfo = -1;
    const auto unknown = calculate_move(move, rating);
    REQUIRE_FALSE(unknown);
    CHECK_THAT(unknown.error(), Catch::Matchers::ContainsSubstring("condition 187"));
    REQUIRE_FALSE(calculate_move(move, rating, nullptr, {{{188, false}}}));
    for (const bool active : {false, true}) {
        const MoveContext context{{{187, active}}};
        const auto result = calculate_move(move, rating, nullptr, context);
        REQUIRE(result);
        REQUIRE(result->hits.size() == 3);
        CHECK(result->hits[0].active);
        CHECK(result->hits[1].active == active);
        CHECK(result->hits[2].active);
        CHECK(result->hits[1].hit.event.stateInfo == 187);
        CHECK(result->selected_powers(1).total_attack_power[1] == (active ? 400 : 0));
        CHECK(result->selected_powers(2).total_attack_power[1] == 100);
        CHECK(result->total.total_attack_power[1] == (active ? 700 : 300));
        CHECK(result->total.attack_powers[7][1] == (active ? 70 : 30));
        const auto restored = erdo::json::read<MoveAttack>(erdo::json::write(*result));
        CHECK(restored.hits[1].active == active);
        CHECK(restored.hits[1].hit.event.stateInfo == 187);
        CHECK(restored.total.total_attack_power == result->total.total_attack_power);
        CHECK(erdo::json::read<MoveContext>(erdo::json::write(context)).state_info == context.state_info);
    }
    SECTION("Inactive events need neither a weapon nor usable motion values") {
        move.hits[1].event.source = 2;
        move.hits[1].motionValues.known.fill(false);
        REQUIRE(calculate_move(move, rating, nullptr, {{{187, false}}}));
        REQUIRE_FALSE(calculate_move(move, rating, nullptr, {{{187, true}}}));
        REQUIRE_FALSE(calculate_move(move, rating, &rating, {{{187, true}}}));
    }
    SECTION("Every positive condition needs its own decision") {
        move.hits[2].event.stateInfo = 188;
        const auto second = calculate_move(move, rating, nullptr, {{{187, false}}});
        REQUIRE_FALSE(second);
        CHECK_THAT(second.error(), Catch::Matchers::ContainsSubstring("condition 188"));
        const auto none = calculate_move(move, rating, nullptr, {{{187, false}, {188, false}}});
        REQUIRE(none);
        CHECK(none->total.total_attack_power[1] == 200);
    }
}

TEST_CASE("move calculation - opposing target eligibility", "[moveset][unit][calculation][optimization]")
{
    auto weapon = make_weapon();
    auto& helper = weapon.moveset->moves[0].hits[0];
    helper.opposeTarget = false;
    helper.event.source = 2;
    helper.event.stateInfo = 999;
    helper.motionValues.known.fill(false);
    const auto restored = json::read<Weapon>(json::write(weapon));
    CHECK(restored.moveset->moves[0].hits[0].opposeTarget == false);
    const auto legacy = json::read<ResolvedHit>(json::write(make_hit(100, 0, 0)));
    CHECK_FALSE(legacy.opposeTarget);
    CHECK(*attack_hit_active(legacy, {}));
    const auto stats = get_character_starting_class_attributes("Wretch");
    const auto evaluated = MoveEvaluator(restored, stats, {0, 0, 0}).evaluate(MoveId::ONE_HAND_R1_1);
    if (!evaluated) INFO(evaluated.error());
    REQUIRE(evaluated);
    REQUIRE(evaluated->hits.size() == 2);
    CHECK_FALSE(evaluated->hits[0].active);
    CHECK(evaluated->hits[1].active);
    CHECK(evaluated->selected_powers(0).attack_powers == AttackPowers{});
    CHECK(evaluated->selected_powers(0).total_attack_power == AttackPower{});
    CHECK_THAT(evaluated->total.total_attack_power[1], Catch::Matchers::WithinAbs(220, 1e-10));
    CHECK(evaluated->total.attack_powers[7] == evaluated->hits[1].powers.attack_powers[7]);
    const auto roundTrip = json::read<MoveAttack>(json::write(*evaluated));
    CHECK_FALSE(roundTrip.hits[0].active);
    CHECK(roundTrip.hits[0].hit.opposeTarget == false);
    CHECK(roundTrip.total.attack_powers == evaluated->total.attack_powers);
    const std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    const AttackOptions options{{0, 0, 0}, false};
    for (const auto selected : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}}) {
        const optimizer::MoveObjective objective{{MoveId::ONE_HAND_R1_1, selected}};
        CHECK_FALSE(optimizer::move_unsupported_reason(weapon, objective, optimizer::Target::TOTAL_ATTACK_POWER));
        optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{weapons, options, 80, stats, 99, false, objective};
        optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{weapons, options, 80, stats, 99, false, objective};
        REQUIRE(brute.supports(weapon));
        REQUIRE(v2.supports(weapon));
        const auto expected = selected ? 0 : 220;
        CHECK_THAT(brute.projection(brute(weapon)), Catch::Matchers::WithinAbs(expected, 1e-10));
        CHECK_THAT(v2.projection(v2(weapon)), Catch::Matchers::WithinAbs(expected, 1e-10));
    }
    // Legacy/allowed hits retain strict source/MV/StateInfo validation.
    for (const auto flag : {std::optional<bool>{}, std::optional<bool>{true}}) {
        helper.opposeTarget = flag;
        const auto failed = MoveEvaluator(weapon, stats, {0, 0, 0}).evaluate(MoveId::ONE_HAND_R1_1);
        REQUIRE_FALSE(failed);
        CHECK_THAT(failed.error(), Catch::Matchers::ContainsSubstring("condition 999"));
    }
}

TEST_CASE("move calculation - Bullet references require projectile context", "[moveset][unit][calculation][optimization]")
{
    auto weapon = make_weapon();
    auto& move = weapon.moveset->moves.front();
    move.hits[1].bulletParamId = 200031311;
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    const auto evaluated = MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(move.id);
    REQUIRE_FALSE(evaluated);
    CHECK_THAT(evaluated.error(), Catch::Matchers::ContainsSubstring("Bullet 200031311"));
    const auto direct = calculate_move(move, AttackRating{});
    REQUIRE_FALSE(direct);
    CHECK_THAT(direct.error(), Catch::Matchers::ContainsSubstring("projectile emission and contact context"));
    std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    for (const auto selected : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}, std::optional<std::size_t>{1}}) {
        const optimizer::MoveObjective objective{{move.id, selected}};
        const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
            weapons, options, 82, stats, 99, false, objective};
        const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
            weapons, options, 82, stats, 99, false, objective};
        CHECK(brute.supports(weapon) == (selected == 0));
        CHECK(v2.supports(weapon) == (selected == 0));
        if (selected != 0) {
            REQUIRE(brute.unsupported_reason(weapon));
            REQUIRE(v2.unsupported_reason(weapon));
            CHECK_THAT(*brute.unsupported_reason(weapon), Catch::Matchers::ContainsSubstring("Bullet 200031311"));
            CHECK_THAT(*v2.unsupported_reason(weapon), Catch::Matchers::ContainsSubstring("Bullet 200031311"));
        }
    }
    // Ineligible events retain their identity but contribute zero, just as
    // direct events do. Projectile requirements apply only to active hits.
    move.hits[1].opposeTarget = false;
    const auto inactive = MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(move.id);
    REQUIRE(inactive);
    CHECK_FALSE(inactive->hits[1].active);
    CHECK(inactive->hits[1].hit.bulletParamId == 200031311);
    move.hits[1].opposeTarget.reset();
    move.hits[1].event.stateInfo = 187;
    const auto gated = MoveEvaluator(weapon, stats, options.upgrade_levels, nullptr, {{{187, false}}}).evaluate(move.id);
    REQUIRE(gated);
    CHECK_FALSE(gated->hits[1].active);
    CHECK_FALSE(MoveEvaluator(weapon, stats, options.upgrade_levels, nullptr, {{{187, true}}}).evaluate(move.id));
    // Metadata without its Bullet ID must never become a direct weapon hit,
    // including on legacy type-1 events and selected optimizer objectives.
    move.hits[1].event.stateInfo.reset();
    move.hits[1].bulletParamId.reset();
    move.hits[1].bulletParameters.emplace();
    const auto malformed = calculate_move(move, AttackRating{});
    REQUIRE_FALSE(malformed);
    CHECK_THAT(malformed.error(), Catch::Matchers::ContainsSubstring("no Bullet provenance"));
    CHECK_FALSE(MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(move.id));
    const optimizer::MoveObjective objective{{move.id, 1}};
    const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
        weapons, options, 82, stats, 99, false, objective};
    const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
        weapons, options, 82, stats, 99, false, objective};
    CHECK_FALSE(brute.supports(weapon));
    CHECK_FALSE(v2.supports(weapon));
}

TEST_CASE("move calculation - selected hits isolate unsupported siblings", "[moveset][unit][calculation][optimization]")
{
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    for (int failure = 0; failure < 5; ++failure) {
        CAPTURE(failure);
        auto weapon = make_weapon();
        auto& move = weapon.moveset->moves[0];
        const auto complete = MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(move.id);
        REQUIRE(complete);
        auto& sibling = move.hits[1];
        switch (failure) {
        case 0: sibling.bulletParamId = 200031311; break;
        case 1: sibling.event.source = 99; break;
        case 2: sibling.motionValues.known[1] = false; break;
        case 3: sibling.event.stateInfo = 188; break;
        case 4: sibling.event.source = 2; break;
        }
        const MoveEvaluator evaluator(weapon, stats, options.upgrade_levels);
        CHECK_FALSE(evaluator.evaluate(move.id));
        CHECK_FALSE(evaluator.evaluate_hit(move.id, 1));
        const auto selected = evaluator.evaluate_hit(move.id, 0);
        REQUIRE(selected);
        CHECK(selected->active);
        CHECK(selected->powers.attack_powers == complete->hits[0].powers.attack_powers);
        CHECK(selected->powers.total_attack_power == complete->hits[0].powers.total_attack_power);
        const auto rating = Attack::calculate(weapon, stats, options);
        const auto lowLevel = calculate_hit(move.hits[0], move.id, rating);
        REQUIRE(lowLevel);
        CHECK(lowLevel->powers.attack_powers == selected->powers.attack_powers);

        std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
        const optimizer::MoveObjective objective{{move.id, 0}};
        const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
            weapons, options, 82, stats, 99, false, objective};
        const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
            weapons, options, 82, stats, 99, false, objective};
        REQUIRE(brute.supports(weapon));
        REQUIRE(v2.supports(weapon));
        const auto best = brute(weapon);
        const auto other = v2(weapon);
        const auto expected = MoveEvaluator(weapon, best.stats, options.upgrade_levels).evaluate_hit(move.id, 0);
        REQUIRE(expected);
        CHECK_THAT(brute.projection(best), Catch::Matchers::WithinAbs(expected->powers.total_attack_power[1], 1e-10));
        CHECK_THAT(v2.projection(other), Catch::Matchers::WithinAbs(brute.projection(best), 1e-10));

        // Imported indices are authoritative even when the unsupported record
        // occurs first. Never renumber or discard records to recover a result.
        std::swap(move.hits[0], move.hits[1]);
        const auto later = evaluator.evaluate_hit(move.id, 1);
        REQUIRE(later);
        CHECK(later->powers.attack_powers == selected->powers.attack_powers);
        CHECK_FALSE(evaluator.evaluate_hit(move.id, 0));
        CHECK_FALSE(evaluator.evaluate_hit(move.id, 2));
        CHECK_FALSE(evaluator.evaluate_hit(move.id, std::numeric_limits<std::size_t>::max()));
    }

    auto weapon = make_weapon();
    auto other = make_weapon();
    other.moveset->behaviorVariation += 1;
    const MoveEvaluator mismatched(weapon, stats, options.upgrade_levels, &other);
    CHECK(mismatched.evaluate_hit(MoveId::POWERSTANCE_L1_1, 2)); // Primary-only hit.
    const auto secondary = mismatched.evaluate_hit(MoveId::POWERSTANCE_L1_1, 0);
    REQUIRE_FALSE(secondary);
    CHECK_THAT(secondary.error(), Catch::Matchers::ContainsSubstring("moveset context"));
    const MoveEvaluator paired(weapon, stats, options.upgrade_levels, &weapon);
    const auto wholePair = paired.evaluate(MoveId::POWERSTANCE_L1_1);
    REQUIRE(wholePair);
    const auto pairedHit = paired.evaluate_hit(MoveId::POWERSTANCE_L1_1, 0);
    REQUIRE(pairedHit);
    CHECK(pairedHit->powers.attack_powers == wholePair->hits[0].powers.attack_powers);
    const auto twoHanded = paired.evaluate(MoveId::TWO_HAND_R1_1);
    REQUIRE(twoHanded);
    const auto twoHandedHit = paired.evaluate_hit(MoveId::TWO_HAND_R1_1, 0);
    REQUIRE(twoHandedHit);
    CHECK(twoHandedHit->powers.attack_powers == twoHanded->hits[0].powers.attack_powers);
    CHECK_FALSE(paired.evaluate_hit(MoveId::OFFHAND_R1_1, 0));
    auto& inactive = weapon.moveset->moves[0].hits[1];
    inactive.bulletParamId = 200031311;
    inactive.event.stateInfo = 187;
    inactive.motionValues.known.fill(false);
    const MoveEvaluator gated(weapon, stats, options.upgrade_levels, nullptr, {{{187, false}}});
    const auto zero = gated.evaluate_hit(MoveId::ONE_HAND_R1_1, 1);
    REQUIRE(zero);
    CHECK_FALSE(zero->active);
    CHECK(zero->hit.bulletParamId == 200031311);
    CHECK(zero->powers.total_attack_power == AttackPower{});
    CHECK(zero->powers.attack_powers == AttackPowers{});
    weapon.moveset->moves[0].hits.clear();
    CHECK_FALSE(gated.evaluate_hit(MoveId::ONE_HAND_R1_1, 0));
}

TEST_CASE("move calculation - known combat environments", "[moveset][unit][calculation]")
{
    auto weapon = make_weapon();
    weapon.moveset->moves[0].hits[1].event.stateInfo = 187;
    const auto stats = get_character_starting_class_attributes("Wretch");
    for (const auto environment : {MoveEnvironment::ORDINARY_COMBAT, MoveEnvironment::RYKARD_ARENA_EFFECT}) {
        const auto context = move_context(environment);
        REQUIRE(context.state_info.size() == 1);
        CHECK(context.state_info.at(187) == (environment == MoveEnvironment::RYKARD_ARENA_EFFECT));
        const auto result = MoveEvaluator(weapon, stats, {0, 0, 0}, nullptr, context).evaluate(MoveId::ONE_HAND_R1_1);
        REQUIRE(result);
        CHECK_THAT(result->total.total_attack_power[1], Catch::Matchers::WithinAbs(
            environment == MoveEnvironment::ORDINARY_COMBAT ? 110 : 330, 1e-10));
        weapon.moveset->moves[0].hits[0].event.stateInfo = 188;
        const auto unknown = MoveEvaluator(weapon, stats, {0, 0, 0}, nullptr, context).evaluate(MoveId::ONE_HAND_R1_1);
        REQUIRE_FALSE(unknown);
        CHECK_THAT(unknown.error(), Catch::Matchers::ContainsSubstring("condition 188"));
        weapon.moveset->moves[0].hits[0].event.stateInfo.reset();
    }
    CHECK_THROWS_AS(move_context(static_cast<MoveEnvironment>(99)), std::invalid_argument);
    // The low-level API still requires explicit context for positive conditions.
    CHECK_FALSE(MoveEvaluator(weapon, stats, {0, 0, 0}).evaluate(MoveId::ONE_HAND_R1_1));
}

TEST_CASE("move calculation - evaluator activation context", "[moveset][unit][calculation]")
{
    auto weapon = make_weapon();
    const auto stats = get_character_starting_class_attributes("Wretch");
    auto& move = weapon.moveset->moves[0];
    move.hits[1].event.stateInfo = 187;
    const auto unknown = MoveEvaluator(weapon, stats, {0, 0, 0}).evaluate_all();
    REQUIRE(unknown.size() == 3);
    CHECK_FALSE(unknown[0].result);
    CHECK(unknown[1].result.has_value());
    MoveContext context{{{187, false}}};
    const MoveEvaluator evaluator(weapon, stats, {0, 0, 0}, nullptr, context);
    context.state_info[187] = true;
    const auto inactive = evaluator.evaluate(move.id);
    REQUIRE(inactive);
    CHECK_FALSE(inactive->hits[1].active);
    CHECK_THAT(inactive->total.total_attack_power[1], Catch::Matchers::WithinAbs(110, 1e-10));
    const auto active = MoveEvaluator(weapon, stats, {0, 0, 0}, nullptr, context).evaluate(move.id);
    REQUIRE(active);
    CHECK_THAT(active->total.total_attack_power[1], Catch::Matchers::WithinAbs(330, 1e-10));
    move.hits[1].event.source = 2;
    CHECK(evaluator.evaluate(move.id).has_value());
    auto other = make_weapon();
    other.moveset->behaviorVariation = 999;
    CHECK(MoveEvaluator(weapon, stats, {0, 0, 0}, &other, {{{187, false}}}).evaluate(move.id).has_value());
    CHECK_FALSE(MoveEvaluator(weapon, stats, {0, 0, 0}, &other, context).evaluate(move.id));
    CHECK_FALSE(MoveEvaluator(weapon, stats, {0, 0, 0}, nullptr, context).evaluate(move.id));
    CHECK(MoveEvaluator(weapon, stats, {0, 0, 0}, &weapon, context).evaluate(move.id).has_value());
}

TEST_CASE("move optimization - conditional scores", "[moveset][unit][optimization]")
{
    auto weapon = make_weapon();
    weapon.moveset->moves[0].hits[1].event.stateInfo = 187;
    std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    const optimizer::MoveObjective unknown{{MoveId::ONE_HAND_R1_1, std::nullopt}};
    optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> unsupported{weapons, options, 82, stats, 99, false, unknown};
    CHECK_FALSE(unsupported.supports(weapon));
    REQUIRE(unsupported.unsupported_reason(weapon));
    CHECK_THAT(*unsupported.unsupported_reason(weapon), Catch::Matchers::ContainsSubstring("condition 187"));
    CHECK(unsupported.run_synchronously(weapons).empty());
    for (const auto selected : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}, std::optional<std::size_t>{1}}) {
        for (const bool active : {false, true}) {
            CAPTURE(selected, active);
            const optimizer::MoveObjective objective{{MoveId::ONE_HAND_R1_1, selected}, false, {{{187, active}}}};
            const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
                weapons, options, 82, stats, 99, false, objective};
            const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
                weapons, options, 82, stats, 99, false, objective};
            REQUIRE(brute.supports(weapon));
            REQUIRE(v2.supports(weapon));
            const auto best = brute(weapon);
            const auto other = v2(weapon);
            const auto evaluated = MoveEvaluator(weapon, best.stats, options.upgrade_levels, nullptr,
                objective.context).evaluate(objective.selection.id);
            REQUIRE(evaluated);
            CHECK_THAT(brute.projection(best), Catch::Matchers::WithinAbs(
                evaluated->selected_powers(selected).total_attack_power[1], 1e-10));
            CHECK_THAT(v2.projection(other), Catch::Matchers::WithinAbs(brute.projection(best), 1e-10));
            optimizer::V2<optimizer::Target::BLEED_STATUS_EFFECT> bleed{
                weapons, options, 82, stats, 99, false, objective};
            const auto bleed_best = bleed(weapon);
            const auto bleed_move = MoveEvaluator(weapon, bleed_best.stats, options.upgrade_levels, nullptr,
                objective.context).evaluate(objective.selection.id);
            REQUIRE(bleed_move);
            CHECK_THAT(bleed.projection(bleed_best), Catch::Matchers::WithinAbs(
                bleed_move->selected_powers(selected).attack_powers[7][1], 1e-10));
        }
    }
    const optimizer::MoveObjective first{{MoveId::ONE_HAND_R1_1, 0}};
    CHECK_FALSE(optimizer::move_unsupported_reason(weapon, first, optimizer::Target::TOTAL_ATTACK_POWER));
    weapon.moveset->moves[0].hits[1].event.source = 2;
    weapon.moveset->moves[0].hits[1].motionValues.known.fill(false);
    const optimizer::MoveObjective inactive{{MoveId::ONE_HAND_R1_1, 1}, false, {{{187, false}}}};
    CHECK_FALSE(optimizer::move_unsupported_reason(weapon, inactive, optimizer::Target::TOTAL_ATTACK_POWER));
    const optimizer::MoveObjective active{{MoveId::ONE_HAND_R1_1, 1}, true, {{{187, true}}}};
    CHECK(optimizer::move_unsupported_reason(weapon, active, optimizer::Target::TOTAL_ATTACK_POWER));
}

TEST_CASE("move optimization - selection changes the objective", "[moveset][unit][optimization]")
{
    auto weapon = make_weapon();
    auto unsupported = make_weapon();
    unsupported.moveset.reset();
    std::vector<std::reference_wrapper<const Weapon>> weapons{weapon, unsupported};
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, true}; // Move determines the stance.
    for (const auto selected_hit : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}}) {
        CAPTURE(selected_hit.has_value());
        const optimizer::MoveObjective objective{{MoveId::ONE_HAND_R1_1, selected_hit}};
        const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
            weapons, options, 82, stats, 99, false, objective};
        const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
            weapons, options, 82, stats, 99, false, objective};
        CHECK(brute.supports(weapon));
        CHECK_FALSE(brute.supports(unsupported));
        CHECK(brute.unsupported_reason(unsupported).has_value());
        REQUIRE_THROWS_AS(brute(unsupported), std::invalid_argument);
        REQUIRE_THROWS_AS(v2(unsupported), std::invalid_argument);
        const auto brute_results = brute.run_synchronously(weapons);
        const auto v2_results = v2.run_synchronously(weapons);
        REQUIRE(brute_results.size() == 1);
        REQUIRE(v2_results.size() == 1);
        const auto& best = brute_results.front();
        CHECK_FALSE(best.two_handing);
        CHECK(relevant_attribute_levels(best.stats)[std::to_underlying(RelevantAttribute::STRENGTH)] == (selected_hit ? 12 : 10));
        CHECK(relevant_attribute_levels(best.stats)[std::to_underlying(RelevantAttribute::DEXTERITY)] == (selected_hit ? 10 : 12));
        CHECK(v2_results[0].stats == best.stats);
        CHECK(brute.iteration_count == brute.stat_variations.size());
        const auto evaluated = MoveEvaluator(weapon, best.stats, options.upgrade_levels).evaluate(objective.selection.id);
        REQUIRE(evaluated.has_value());
        CHECK_THAT(brute.projection(best), Catch::Matchers::WithinAbs(
            evaluated->selected_powers(selected_hit).total_attack_power[1], 1e-10));
        CHECK_THAT(v2.projection(v2_results[0]), Catch::Matchers::WithinAbs(brute.projection(best), 1e-10));
    }
}

TEST_CASE("move optimization - status and support checks", "[moveset][unit][optimization]")
{
    auto weapon = make_weapon();
    std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    const optimizer::MoveObjective objective{{MoveId::ONE_HAND_R1_1, std::nullopt}};
    optimizer::V2<optimizer::Target::BLEED_STATUS_EFFECT> bleed{weapons, options, 82, stats, 99, false, objective};
    const auto best = bleed(weapon);
    CHECK(relevant_attribute_levels(best.stats)[std::to_underlying(RelevantAttribute::DEXTERITY)] == 12);
    CHECK_THAT(bleed.projection(best), Catch::Matchers::WithinAbs(64.4, 1e-10));

    SECTION("Only selected hit and channel need motion values") {
        weapon.moveset->moves[0].hits[0].motionValues.known[0] = false;
        CHECK(bleed.supports(weapon));
        const optimizer::MoveObjective first{{MoveId::ONE_HAND_R1_1, 0}};
        optimizer::V2<optimizer::Target::PHYSICAL_ATTACK_POWER> invalid{weapons, options, 82, stats, 99, false, first};
        CHECK_FALSE(invalid.supports(weapon));
        CHECK(invalid.iteration_count == 0);
        CHECK(invalid.run_synchronously(weapons).empty());
        const optimizer::MoveObjective second{{MoveId::ONE_HAND_R1_1, 1}};
        optimizer::V2<optimizer::Target::MAGIC_ATTACK_POWER> valid{weapons, options, 82, stats, 99, false, second};
        CHECK(valid.supports(weapon));
    }
    SECTION("Missing hit is excluded") {
        const optimizer::MoveObjective absent{{MoveId::ONE_HAND_R1_1, 2}};
        optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> invalid{weapons, options, 82, stats, 99, false, absent};
        CHECK(invalid.iteration_count == 0);
        CHECK(invalid.run_synchronously(weapons).empty());
    }
    SECTION("Powerstance requires opting into a matching pair") {
        optimizer::MoveObjective pair{{MoveId::POWERSTANCE_L1_1, std::nullopt}};
        optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> unsupported{weapons, options, 82, stats, 99, false, pair};
        CHECK_FALSE(unsupported.supports(weapon));
        pair.matching_powerstance = true;
        optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> supported{weapons, options, 82, stats, 99, false, pair};
        const auto result = supported(weapon);
        const auto evaluated = MoveEvaluator(weapon, result.stats, options.upgrade_levels, &weapon).evaluate(pair.selection.id);
        REQUIRE(evaluated.has_value());
        CHECK_THAT(supported.projection(result), Catch::Matchers::WithinAbs(evaluated->total.total_attack_power[1], 1e-10));
    }
    SECTION("Spell scaling keeps its own meaning") {
        REQUIRE_THROWS_AS((optimizer::V2<optimizer::Target::SPELL_SCALING>{weapons, options, 82, stats, 99, false, objective}),
                          std::invalid_argument);
    }
}

TEST_CASE("move calculation - offhand source roles and optimizer scores", "[moveset][unit][calculation][optimization]")
{
    AttackRating primary{}, secondary{};
    primary.attack_powers[0] = {200, 400};
    primary.attack_powers[1] = {50, 100};
    primary.attack_powers[7] = {40, 50};
    secondary.attack_powers[0] = {1000, 2000};
    secondary.attack_powers[1] = {500, 1000};
    secondary.attack_powers[7] = {400, 500};
    for (const auto id : {MoveId::OFFHAND_R1_1, MoveId::OFFHAND_R1_2, MoveId::OFFHAND_R1_3,
            MoveId::OFFHAND_R1_4, MoveId::OFFHAND_R1_5, MoveId::OFFHAND_R1_6}) {
        CAPTURE(id);
        Move move{id, "Explicit left source", 35000, {make_hit(80, 120, 65, 2)}};
        const auto left = calculate_move(move, primary);
        REQUIRE(left);
        CHECK(left->hits[0].hit.event.source == 2);
        CHECK((left->total.attack_powers[0] == AttackPower{160, 320}));
        CHECK((left->total.attack_powers[1] == AttackPower{60, 120}));
        CHECK((left->total.attack_powers[7] == AttackPower{26, 32.5}));
        const auto supplied = calculate_move(move, primary, &secondary);
        REQUIRE(supplied);
        CHECK(supplied->total.attack_powers == left->total.attack_powers);
        move.hits[0].event.source = 0;
        REQUIRE(calculate_move(move, primary));
        CHECK(calculate_move(move, primary)->total.attack_powers == left->total.attack_powers);
        move.hits[0].event.source = 1;
        REQUIRE_FALSE(calculate_move(move, primary));
        const auto right = calculate_move(move, primary, &secondary);
        REQUIRE(right);
        CHECK((right->total.attack_powers[0] == AttackPower{800, 1600}));
        move.hits[0].event.eventType = 307;
        move.hits[0].event.source = -1;
        move.hits[0].event.pcBehaviorType = 16;
        move.hits[0].event.pcBehaviorCondition = 0;
        move.hits[0].event.pcBehaviorOffset = 0;
        REQUIRE_FALSE(calculate_move(move, primary));
        REQUIRE(calculate_move(move, primary, &secondary));
        CHECK(calculate_move(move, primary, &secondary)->total.attack_powers == right->total.attack_powers);
    }
    // Other move roles retain explicit right/left selection.
    Move rightMove{MoveId::ONE_HAND_R1_1, "Right", 30000, {make_hit(100, 100, 100, 2)}};
    CHECK_FALSE(calculate_move(rightMove, primary));
    rightMove.hits[0].event.source = 1;
    REQUIRE(calculate_move(rightMove, primary));
    CHECK(calculate_move(rightMove, primary)->total.attack_powers == primary.attack_powers);

    auto weapon = make_weapon();
    weapon.moveset->moves = {{MoveId::OFFHAND_R1_1, "Offhand", 35000,
        {make_hit(80, 120, 65, 2), make_hit(50, 20, 100, 0)}}};
    const auto stats = get_character_starting_class_attributes("Wretch");
    const AttackOptions options{{0, 0, 0}, false};
    const auto evaluated = MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(MoveId::OFFHAND_R1_1);
    REQUIRE(evaluated);
    auto other = make_weapon();
    other.moveset = std::make_shared<Moveset>(*other.moveset);
    other.moveset->behaviorVariation = 999;
    other.base_attack_powers_at_upgrade_levels[0][0] = 1000;
    const auto ignored = MoveEvaluator(weapon, stats, options.upgrade_levels, &other).evaluate(MoveId::OFFHAND_R1_1);
    REQUIRE(ignored);
    CHECK(ignored->total.attack_powers == evaluated->total.attack_powers);
    for (const auto selected : {std::optional<std::size_t>{}, std::optional<std::size_t>{0}, std::optional<std::size_t>{1}}) {
        const optimizer::MoveObjective objective{{MoveId::OFFHAND_R1_1, selected}, false};
        std::vector<std::reference_wrapper<const Weapon>> weapons{weapon};
        const optimizer::BruteForce<optimizer::Target::TOTAL_ATTACK_POWER> brute{
            weapons, options, 80, stats, 99, false, objective};
        const optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> v2{
            weapons, options, 80, stats, 99, false, objective};
        REQUIRE(brute.supports(weapon));
        REQUIRE(v2.supports(weapon));
        CHECK_THAT(brute.projection(brute(weapon)), Catch::Matchers::WithinAbs(
            evaluated->selected_powers(selected).total_attack_power[1], 1e-10));
        CHECK_THAT(v2.projection(v2(weapon)), Catch::Matchers::WithinAbs(
            evaluated->selected_powers(selected).total_attack_power[1], 1e-10));
    }
    weapon.moveset->moves[0].hits[0].event.source = 1;
    CHECK_FALSE(MoveEvaluator(weapon, stats, options.upgrade_levels).evaluate(MoveId::OFFHAND_R1_1));
    CHECK_FALSE(MoveEvaluator(weapon, stats, options.upgrade_levels, &other).evaluate(MoveId::OFFHAND_R1_1));
    CHECK(MoveEvaluator(weapon, stats, options.upgrade_levels, &weapon).evaluate(MoveId::OFFHAND_R1_1));
    CHECK(optimizer::move_unsupported_reason(weapon, {{MoveId::OFFHAND_R1_1, std::nullopt}, false},
        optimizer::Target::TOTAL_ATTACK_POWER));
}
