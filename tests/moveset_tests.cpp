#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

import std;
import erdo;

using namespace erdo::witchy;

namespace {
struct ImporterFixture {
    ParamTable attacks;
    ParamTable behaviors;
    BehaviorIndex index;

    ImporterFixture()
    {
        attacks.loadXml(R"(<param><fields>
            <field name="atkPhysCorrection" defaultValue="100"/>
            <field name="atkMagCorrection" defaultValue="0"/>
            <field name="statusAilmentAtkPowerCorrectRate" defaultValue="65"/>
            </fields><rows><row id="200100" atkPhysCorrection="125"/>
            <row id="600200100" atkPhysCorrection="151" atkMagCorrection="11"
                atkFireCorrection="22" atkThunCorrection="33" atkDarkCorrection="44"/>
            <row id="7" atkPhysCorrection="bad"/></rows></param>)");
        behaviors.loadXml(R"(<param><fields>
            <field name="variationId" defaultValue="200"/>
            <field name="behaviorJudgeId" defaultValue="100"/>
            <field name="refType" defaultValue="0"/>
            <field name="category" defaultValue="1"/>
            </fields><rows>
            <row id="100200100" refId="200100"/>
            <row id="600200100" category="12" refId="600200100"/>
            <row id="3" behaviorJudgeId="3" refType="1" refId="200100"/>
            <row id="4" behaviorJudgeId="4" refId="404"/>
            <row id="5" behaviorJudgeId="5"/>
            <row id="6" behaviorJudgeId="6" refId="200100"/>
            <row id="7" behaviorJudgeId="7" refId="7"/>
            <row id="8" behaviorJudgeId="8" refId="200100"/>
            <row id="9" behaviorJudgeId="8" refId="200100"/>
            </rows></param>)");
        index.build(behaviors);
    }
};

constexpr const char* taeXml = R"(<anim><events>
        <event><type>1</type><startTime>0.2</startTime><endTime>0.4</endTime><params>
          <param name="BehaviorJudgeID" value="6"/><param name="AttackIndex" value="1"/>
          <param name="Source" value="2"/><param name="StateInfo" value="187"/></params></event>
        <event><type>304</type><startTime>0.1</startTime><endTime>0.3</endTime><params>
          <param name="BehaviorJudgeID" value="6"/><param name="Index" value="7"/></params></event>
        <event><type>1</type><startTime>0.2</startTime><endTime>0.4</endTime><params>
          <param name="BehaviorJudgeID" value="6"/><param name="Source" value="1"/>
          <param name="StateInfo" value="-1"/></params></event>
        <event><type>307</type></event>
        </events></anim>)";
}

TEST_CASE("moveset - IDs and catalogs", "[moveset][unit]")
{
    static_assert(std::to_underlying(MoveId::MOUNTED_CHARGED_L2) + 1 == 73);
    for (int id = 0; id <= std::to_underlying(MoveId::MOUNTED_CHARGED_L2); ++id) {
        const auto moveId = static_cast<MoveId>(id);
        UNSCOPED_INFO("Every move ID must round-trip through the existing serializer");
        REQUIRE((erdo::json::read<MoveId>(erdo::json::write(moveId)) == moveId));
    }
    for (const auto catalog : {std::span<const MoveDefinition>(daggerCatalog),
                              std::span<const MoveDefinition>(straightSwordCatalog),
                              std::span<const MoveDefinition>(axeCatalog),
                              std::span<const MoveDefinition>(hammerCatalog),
                              std::span<const MoveDefinition>(flailCatalog),
                              std::span<const MoveDefinition>(greatswordCatalog),
                              std::span<const MoveDefinition>(greataxeCatalog),
                              std::span<const MoveDefinition>(greathammerCatalog),
                              std::span<const MoveDefinition>(katanaCatalog),
                              std::span<const MoveDefinition>(curvedGreatswordCatalog),
                              std::span<const MoveDefinition>(colossalSwordCatalog),
                              std::span<const MoveDefinition>(colossalWeaponCatalog),
                              std::span<const MoveDefinition>(halberdCatalog),
                              std::span<const MoveDefinition>(scytheCatalog),
                              std::span<const MoveDefinition>(spearCatalog),
                              std::span<const MoveDefinition>(greatSpearCatalog),
                              std::span<const MoveDefinition>(whipCatalog),
                              std::span<const MoveDefinition>(thrustingSwordCatalog),
                              std::span<const MoveDefinition>(curvedSwordCatalog),
                              std::span<const MoveDefinition>(greatKatanaCatalog),
                              std::span<const MoveDefinition>(lightGreatswordCatalog),
                              std::span<const MoveDefinition>(thrustingShieldCatalog)}) {
        std::set<MoveId> ids;
        for (const auto& move : catalog) {
            UNSCOPED_INFO("A weapon catalog must not contain duplicate move IDs");
            REQUIRE((ids.insert(move.id).second));
        }
    }
    CHECK(flailCatalog.size() == 60);
    for (const auto id : {MoveId::ONE_HAND_R1_5, MoveId::TWO_HAND_R1_5, MoveId::OFFHAND_R1_5})
        CHECK(std::ranges::find(flailCatalog, id, &MoveDefinition::id) == flailCatalog.end());
    CHECK(std::ranges::find(flailCatalog, MoveId::POWERSTANCE_L1_4, &MoveDefinition::id) != flailCatalog.end());
    CHECK(greatswordCatalog.size() == 59);
    CHECK(katanaCatalog.size() == 63);
    CHECK(curvedGreatswordCatalog.size() == 59);
    CHECK(colossalSwordCatalog.size() == 56);
    CHECK(halberdCatalog.size() == 59);
    CHECK(scytheCatalog.size() == 57);
    CHECK(spearCatalog.size() == 60);
    CHECK(greatSpearCatalog.size() == 57);
    CHECK(whipCatalog.size() == 51);
    CHECK(thrustingSwordCatalog.size() == 71);
    CHECK(curvedSwordCatalog.size() == 70);
    CHECK(greatKatanaCatalog.size() == 63);
    CHECK(lightGreatswordCatalog.size() == 61);
    CHECK(thrustingShieldCatalog.size() == 52);
    for (const auto id : {MoveId::POWERSTANCE_L1_1, MoveId::POWERSTANCE_L1_2, MoveId::POWERSTANCE_L1_3,
            MoveId::POWERSTANCE_L1_4, MoveId::POWERSTANCE_L1_5, MoveId::POWERSTANCE_L1_6,
            MoveId::POWERSTANCE_RUNNING_L1, MoveId::POWERSTANCE_ROLLING_L1,
            MoveId::POWERSTANCE_BACKSTEP_L1, MoveId::POWERSTANCE_JUMPING_L1})
        CHECK(std::ranges::find(thrustingShieldCatalog, id, &MoveDefinition::id) == thrustingShieldCatalog.end());
    for (auto id : {MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING})
        CHECK(std::ranges::find(lightGreatswordCatalog, id, &MoveDefinition::id) == lightGreatswordCatalog.end());
    for (const auto [id, judge] : {std::pair{MoveId::MOUNTED_CHARGED_R2, 638}, std::pair{MoveId::MOUNTED_CHARGED_L2, 738}}) {
        const auto move = std::ranges::find(lightGreatswordCatalog, id, &MoveDefinition::id);
        REQUIRE(move != lightGreatswordCatalog.end());
        CHECK(move->eventJudgeId == judge);
    }
    for (auto id : {MoveId::ONE_HAND_R1_5, MoveId::TWO_HAND_R1_5, MoveId::OFFHAND_R1_5,
            MoveId::ONE_HAND_R1_6, MoveId::TWO_HAND_R1_6, MoveId::OFFHAND_R1_6, MoveId::POWERSTANCE_L1_4,
            MoveId::SHIELDPOKE})
        CHECK(std::ranges::find(greatKatanaCatalog, id, &MoveDefinition::id) == greatKatanaCatalog.end());
    CHECK(std::ranges::find(curvedSwordCatalog, MoveId::SHIELDPOKE, &MoveDefinition::id) == curvedSwordCatalog.end());
    for (auto id : {MoveId::ONE_HAND_R1_6, MoveId::TWO_HAND_R1_6, MoveId::OFFHAND_R1_6,
            MoveId::SHIELDPOKE, MoveId::ONE_HAND_CHARGED_R2_1_FEINT, MoveId::ONE_HAND_CHARGED_R2_2_FEINT,
            MoveId::TWO_HAND_CHARGED_R2_1_FEINT, MoveId::TWO_HAND_CHARGED_R2_2_FEINT})
        CHECK(std::ranges::find(thrustingSwordCatalog, id, &MoveDefinition::id) != thrustingSwordCatalog.end());
    for (auto id : {MoveId::BACKSTAB_WHIFF, MoveId::BACKSTAB, MoveId::RIPOSTE,
            MoveId::SMALL_PVE_BACKSTAB, MoveId::SMALL_PVE_RIPOSTE, MoveId::LARGE_PVE_RIPOSTE,
            MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING})
        CHECK(std::ranges::find(whipCatalog, id, &MoveDefinition::id) == whipCatalog.end());
    for (auto id : {MoveId::MOUNTED_CHARGED_R2, MoveId::MOUNTED_CHARGED_L2,
            MoveId::ONE_HAND_R1_4, MoveId::TWO_HAND_R1_4, MoveId::OFFHAND_R1_4,
            MoveId::POWERSTANCE_L1_3})
        CHECK(std::ranges::find(whipCatalog, id, &MoveDefinition::id) != whipCatalog.end());
    CHECK(std::ranges::find(spearCatalog, MoveId::SHIELDPOKE, &MoveDefinition::id) != spearCatalog.end());
    for (auto id : {MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING}) {
        CHECK(std::ranges::find(halberdCatalog, id, &MoveDefinition::id) != halberdCatalog.end());
        CHECK(std::ranges::find(scytheCatalog, id, &MoveDefinition::id) == scytheCatalog.end());
    }
    for (auto id : {MoveId::MOUNTED_CHARGED_R2, MoveId::MOUNTED_CHARGED_L2})
        CHECK(std::ranges::find(scytheCatalog, id, &MoveDefinition::id) != scytheCatalog.end());
    for (auto id : {MoveId::ONE_HAND_R1_4, MoveId::TWO_HAND_R1_4, MoveId::OFFHAND_R1_4})
        CHECK(std::ranges::find(colossalSwordCatalog, id, &MoveDefinition::id) == colossalSwordCatalog.end());
    for (auto id : {MoveId::ONE_HAND_R1_3, MoveId::TWO_HAND_R1_3, MoveId::OFFHAND_R1_3, MoveId::POWERSTANCE_L1_3})
        CHECK(std::ranges::find(colossalSwordCatalog, id, &MoveDefinition::id) != colossalSwordCatalog.end());
    CHECK(std::ranges::find(greatswordCatalog, MoveId::POWERSTANCE_L1_4, &MoveDefinition::id) == greatswordCatalog.end());
    for (const auto id : {MoveId::ONE_HAND_R1_4, MoveId::TWO_HAND_R1_4, MoveId::OFFHAND_R1_4, MoveId::POWERSTANCE_L1_3})
        CHECK(std::ranges::find(greatswordCatalog, id, &MoveDefinition::id) != greatswordCatalog.end());
}

TEST_CASE_METHOD(ImporterFixture, "moveset - behavior resolution", "[moveset][unit]")
{
    index.build(behaviors);
    UNSCOPED_INFO("Rebuilding must not duplicate candidates");
    REQUIRE((index.find(200, 100).size() == 2));

    const std::vector<AttackEvent> events{{30000, 1, 100, 0, 0, .1, .2}};
    auto result = resolveAttackEvents(events, 200, index, attacks);
    UNSCOPED_INFO("Ambiguity must retain both behavior candidates");
    REQUIRE((result.hits.empty() && result.unresolved.size() == 1 &&
            result.unresolved.front().candidates.size() == 2));
    UNSCOPED_INFO("Diagnostic must identify candidates");
    REQUIRE((result.diagnostics.front().contains("100200100") &&
            result.diagnostics.front().contains("category=12")));
    result = resolveAttackEvents(events, 200, index, attacks, 1);
    UNSCOPED_INFO("Schema defaults must resolve");
    REQUIRE((result.diagnostics.empty() && result.hits.size() == 1));
    const auto& hit = result.hits.front();
    UNSCOPED_INFO("Filtered provenance or row override lost");
    REQUIRE((hit.behaviorParamId == 100200100 && hit.behaviorCategory == 1 &&
            hit.atkParamId == 200100 && hit.motionValues.values[0] == 125));
    UNSCOPED_INFO("Unknown MV must differ from known zero");
    REQUIRE((hit.motionValues.known[1] && hit.motionValues.values[1] == 0 &&
            !hit.motionValues.known[2]));
    for (std::size_t i = 5; i != 12; ++i) {
        UNSCOPED_INFO("Each status channel must retain its value and known flag");
        REQUIRE((hit.motionValues.known[i] && hit.motionValues.values[i] == 65));
    }
    UNSCOPED_INFO("No fallback when requested category is absent");
    REQUIRE((resolveAttackEvents(events, 200, index, attacks, 2).hits.empty()));
    const auto alternative = resolveAttackEvents(events, 200, index, attacks, 12);
    UNSCOPED_INFO("Alternative category must remain resolvable");
    REQUIRE((alternative.hits.size() == 1));
    const std::array expectedDamage{151.f, 11.f, 22.f, 33.f, 44.f};
    for (std::size_t i = 0; i != expectedDamage.size(); ++i) {
        UNSCOPED_INFO("Damage channel mapping must preserve distinct values");
        REQUIRE((alternative.hits[0].motionValues.known[i] &&
                alternative.hits[0].motionValues.values[i] == expectedDamage[i]));
    }
    for (int judge : {3, 4, 5, 7, 8, 404}) {
        auto invalid = events;
        invalid.front().behaviorJudgeId = judge;
        const auto failed = resolveAttackEvents(invalid, 200, index, attacks, 1);
        UNSCOPED_INFO("Unsupported, missing, invalid, or same-category duplicate must fail safely");
        REQUIRE((failed.hits.empty() && failed.diagnostics.size() == 1 && failed.unresolved.size() == 1));
    }
    auto duplicated = events;
    duplicated[0].behaviorJudgeId = 8;
    const auto explicitRow = resolveAttackEvents(duplicated, 200, index, attacks, 1, 8);
    UNSCOPED_INFO("Explicit catalog context must resolve a same-category collision");
    REQUIRE((explicitRow.hits.size() == 1 && explicitRow.hits[0].behaviorParamId == 8));
    const auto wrongRow = resolveAttackEvents(duplicated, 200, index, attacks, 1, 6);
    UNSCOPED_INFO("Explicit selection must not bypass the judge or discard candidates");
    REQUIRE((wrongRow.hits.empty() && wrongRow.unresolved[0].requiredBehaviorParamId == 6 &&
            wrongRow.unresolved[0].candidates.size() == 2));
    UNSCOPED_INFO("Explicit selection must still enforce variation and category");
    REQUIRE((resolveAttackEvents(duplicated, 201, index, attacks, 1, 8).hits.empty() &&
            resolveAttackEvents(duplicated, 200, index, attacks, 12, 8).hits.empty()));
    const auto restoredFailure = erdo::json::read<UnresolvedAttack>(erdo::json::write(wrongRow.unresolved[0]));
    UNSCOPED_INFO("Report JSON must retain the explicit row constraint");
    REQUIRE((restoredFailure.requiredBehaviorParamId == 6));

    attacks.loadXml("<param><rows><row id='99'/></rows></param>");
    UNSCOPED_INFO("Reload must discard stale XML nodes/defaults");
    REQUIRE((!attacks.find(200100) && attacks.defaults.empty()));
    UNSCOPED_INFO("Absent fields must remain unknown");
    REQUIRE((std::ranges::none_of(extractMotionValues(attacks, attacks.find(99)).known,
            [](bool known) { return known; })));
}

TEST_CASE("moveset - opposing target flags", "[moveset][unit]")
{
    ParamTable attacks, behaviors;
    behaviors.loadXml("<param><rows><row id='1' variationId='200' behaviorJudgeId='0' refType='0' refId='1'/></rows></param>");
    BehaviorIndex index;
    index.build(behaviors);
    const std::vector<AttackEvent> events{{30000, 1, 0, 0, 0, .1, .2}};
    for (const auto flag : {"0", "1"}) {
        CAPTURE(flag);
        attacks.loadXml(std::format("<param><fields><field name='opposeTarget' defaultValue='{}'/></fields><rows><row id='1'/></rows></param>", flag).c_str());
        auto resolved = resolveAttackEvents(events, 200, index, attacks);
        REQUIRE(resolved.diagnostics.empty());
        REQUIRE(resolved.hits.size() == 1);
        CHECK(resolved.hits[0].opposeTarget == (std::string_view(flag) == "1"));
        CHECK(erdo::json::read<erdo::calculator::ResolvedHit>(erdo::json::write(resolved.hits[0])).opposeTarget == resolved.hits[0].opposeTarget);
    }
    attacks.loadXml("<param><fields><field name='opposeTarget' defaultValue='1'/></fields><rows><row id='1' opposeTarget='0'/></rows></param>");
    CHECK(resolveAttackEvents(events, 200, index, attacks).hits[0].opposeTarget == false);
    attacks.loadXml("<param><rows><row id='1'/></rows></param>");
    CHECK_FALSE(resolveAttackEvents(events, 200, index, attacks).hits[0].opposeTarget);
    for (const auto flag : {"2", "-1", "0.5", "nan", "bad"}) {
        CAPTURE(flag);
        attacks.loadXml(std::format("<param><rows><row id='1' opposeTarget='{}'/></rows></param>", flag).c_str());
        const auto failed = resolveAttackEvents(events, 200, index, attacks);
        CHECK(failed.hits.empty());
        REQUIRE(failed.diagnostics.size() == 1);
        CHECK_THAT(failed.diagnostics[0], Catch::Matchers::ContainsSubstring("opposeTarget"));
    }
}

TEST_CASE("moveset - encoded player behavior namespaces", "[moveset][unit]")
{
    ParamTable behaviors, attacks;
    behaviors.loadXml(R"(<param><rows>
      <row id="300201950" variationId="201" behaviorJudgeId="950" category="12" refType="0" refId="1"/>
      <row id="300200950" variationId="200" behaviorJudgeId="950" category="1" refType="0" refId="2"/>
      <row id="300000950" variationId="0" behaviorJudgeId="950" category="1" refType="0" refId="3"/>
      <row id="100201950" variationId="201" behaviorJudgeId="950" category="1" refType="0" refId="4"/>
      <row id="900000999" variationId="0" behaviorJudgeId="999" category="1" refType="0" refId="3"/>
      <row id="100000000" variationId="0" behaviorJudgeId="0" category="1" refType="0" refId="4"/>
    </rows></param>)");
    attacks.loadXml(R"(<param><rows>
      <row id="1" atkPhysCorrection="101"/><row id="2" atkPhysCorrection="102"/>
      <row id="3" atkPhysCorrection="103"/><row id="4" atkPhysCorrection="104"/>
    </rows></param>)");
    BehaviorIndex index;
    index.build(behaviors);
    const std::vector<AttackEvent> events{{30600, 1, 3950, 0, 0, .1, .2}};
    for (const auto& [variation, behavior, attack] : std::array{
        std::tuple{201, 300201950, 1}, std::tuple{209, 300200950, 2},
        std::tuple{707, 300000950, 3}}) {
        const auto result = resolveAttackEvents(events, variation, index, attacks);
        REQUIRE(result.diagnostics.empty());
        REQUIRE(result.hits.size() == 1);
        CHECK(result.hits[0].behaviorParamId == behavior);
        CHECK(result.hits[0].atkParamId == attack);
        CHECK(result.hits[0].event.behaviorJudgeId == 3950);
    }
    // Constraints apply AFTER selecting the first existing row. They must not
    // search a fallback with a more convenient category, ID or reference type.
    auto constrained = resolveAttackEvents(events, 201, index, attacks, 1);
    REQUIRE(constrained.hits.empty());
    REQUIRE(constrained.unresolved.size() == 1);
    REQUIRE(constrained.unresolved[0].candidates.size() == 1);
    CHECK(constrained.unresolved[0].candidates[0].behaviorParamId == 300201950);
    CHECK(resolveAttackEvents(events, 201, index, attacks, {}, 300200950).hits.empty());
    CHECK(resolveAttackEvents(events, 201, index, attacks, 12, 300201950).hits.size() == 1);
    for (const auto& [judge, behavior] : std::array{
        std::pair{1950, 100201950}, std::pair{1000, 100000000}, std::pair{9999, 900000999}}) {
        auto event = events;
        event[0].behaviorJudgeId = judge;
        auto result = resolveAttackEvents(event, 201, index, attacks);
        REQUIRE(result.hits.size() == 1);
        CHECK(result.hits[0].behaviorParamId == behavior);
    }
    CHECK(index.findEncoded(-1, 3950).empty());
    CHECK(index.findEncoded(std::numeric_limits<int>::max(), 3950).empty());
    CHECK(index.findEncoded(201, 999).empty());
    CHECK(index.findEncoded(201, 10000).empty());
    // Existing but invalid exact rows cannot fall through to valid family rows.
    for (auto ref : {"refType='1' refId='1'", "refType='0' refId='999'"}) {
        const auto xml = std::format("<param><rows><row id='300201950' variationId='201' behaviorJudgeId='950' category='1' {}/><row id='300200950' variationId='200' behaviorJudgeId='950' category='1' refType='0' refId='2'/></rows></param>", ref);
        behaviors.loadXml(xml.c_str());
        index.build(behaviors);
        const auto result = resolveAttackEvents(events, 201, index, attacks);
        CHECK(result.hits.empty());
        REQUIRE(result.unresolved.size() == 1);
        REQUIRE(result.unresolved[0].candidates.size() == 1);
        CHECK(result.unresolved[0].candidates[0].behaviorParamId == 300201950);
    }
    behaviors.loadXml("<param><rows/></param>");
    index.build(behaviors);
    CHECK(index.findEncoded(201, 3950).empty());
    CHECK(resolveAttackEvents(events, 201, index, attacks).hits.empty());
}

TEST_CASE("moveset - ordinary player weapon lookup and fallback", "[moveset][unit]")
{
    ParamTable behaviors, attacks;
    behaviors.loadXml(R"(<param><rows>
      <row id="100201100" variationId="201" behaviorJudgeId="100" category="12" refType="0" refId="1"/>
      <row id="300201100" variationId="201" behaviorJudgeId="100" category="12" refType="0" refId="2"/>
      <row id="100200100" variationId="200" behaviorJudgeId="100" category="1" refType="0" refId="2"/>
      <row id="100000100" variationId="0" behaviorJudgeId="100" category="1" refType="0" refId="3"/>
    </rows></param>)");
    attacks.loadXml(R"(<param><rows><row id="1" atkPhysCorrection="121"/>
      <row id="2" atkPhysCorrection="122"/><row id="3" atkPhysCorrection="123"/></rows></param>)");
    BehaviorIndex index;
    index.build(behaviors);
    const std::vector<AttackEvent> events{{30505, 1, 100, 0, 0, .1, .2}};
    // Field-based inspection still exposes ambiguity, even within one category.
    CHECK(resolveAttackEvents(events, 201, index, attacks, 12).hits.empty());
    for (const auto& [variation, id] : std::array{
        std::pair{201, 100201100}, std::pair{209, 100200100}, std::pair{707, 100000100}}) {
        auto result = resolveAttackEvents(events, variation, index, attacks, {}, {}, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(result.diagnostics.empty());
        REQUIRE(result.hits.size() == 1);
        CHECK(result.hits[0].behaviorParamId == id);
        CHECK(result.hits[0].event.behaviorJudgeId == 100);
    }
    // Namespace choice follows the TAE judge, not category or first matching row.
    auto encoded = events;
    encoded[0].behaviorJudgeId = 3100;
    auto other = resolveAttackEvents(encoded, 201, index, attacks, {}, {}, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(other.hits.size() == 1);
    CHECK(other.hits[0].behaviorParamId == 300201100);
    for (auto constraint : {std::optional{1}, std::optional{2}}) {
        auto failed = resolveAttackEvents(events, 201, index, attacks, constraint, {}, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(failed.unresolved.size() == 1);
        REQUIRE(failed.unresolved[0].candidates.size() == 1);
        CHECK(failed.unresolved[0].candidates[0].behaviorParamId == 100201100);
    }
    CHECK(resolveAttackEvents(events, 201, index, attacks, {}, 300201100, BehaviorLookup::PLAYER_WEAPON).hits.empty());
    for (auto ref : {"refType='1' refId='1'", "refType='0' refId='999'"}) {
        auto xml = std::format("<param><rows><row id='100201100' variationId='201' behaviorJudgeId='100' category='12' {}/><row id='100200100' variationId='200' behaviorJudgeId='100' category='1' refType='0' refId='2'/></rows></param>", ref);
        behaviors.loadXml(xml.c_str());
        index.build(behaviors);
        auto failed = resolveAttackEvents(events, 201, index, attacks, {}, {}, BehaviorLookup::PLAYER_WEAPON);
        CHECK(failed.hits.empty());
        REQUIRE(failed.unresolved.size() == 1);
        CHECK(failed.unresolved[0].candidates.front().behaviorParamId == 100201100);
    }
    CHECK(index.findPlayerWeapon(-1, 100).empty());
    CHECK(index.findPlayerWeapon(201, -1).empty());
    CHECK(index.findPlayerWeapon(201, 10000).empty());
    behaviors.loadXml("<param><rows/></param>");
    index.build(behaviors);
    CHECK(index.findPlayerWeapon(201, 100).empty());
}

TEST_CASE_METHOD(ImporterFixture, "moveset - attack events", "[moveset][unit]")
{
    xml_document tae;
    UNSCOPED_INFO("Fixture XML must parse");
    REQUIRE((static_cast<bool>(tae.load_string(taeXml))));
    const auto extracted = extractAttackEvents(tae, 34000);
    UNSCOPED_INFO("Stable order, repeated judges, type-304 Index, or absent Source lost");
    REQUIRE((extracted.size() == 3 && extracted[0].eventType == 304 &&
            extracted[0].attackIndex == 7 && extracted[0].source == -1 &&
            extracted[1].source == 2 && extracted[2].source == 1));
    CHECK_FALSE(extracted[0].stateInfo);
    CHECK(extracted[1].stateInfo == 187);
    CHECK(extracted[2].stateInfo == -1);
    const auto result = resolveAttackEvents(extracted, 200, index, attacks);
    UNSCOPED_INFO("Repeated and overlapping attack events must survive");
    REQUIRE((result.hits.size() == 3 && result.hits[0].event.endTime > result.hits[1].event.startTime));
    CHECK(result.hits[1].event.stateInfo == 187);
    CHECK(result.hits[2].event.stateInfo == -1);
    for (const auto start : {"nan", "0.1junk", "-1", "2"}) {
        const auto xml = std::format(R"(<anim><events><event><type>1</type>
            <startTime>{}</startTime><endTime>1</endTime><params>
            <param name="BehaviorJudgeID" value="6"/></params></event></events></anim>)", start);
        tae.load_string(xml.c_str());
        CAPTURE(start);
        REQUIRE_THROWS_AS(extractAttackEvents(tae, 30000), std::runtime_error);
    }

}

TEST_CASE("moveset - StateInfo validation and legacy JSON", "[moveset][unit]")
{
    xml_document tae;
    for (const auto value : {"0", "-1", "187", "2147483647", "-2147483648"}) {
        const auto xml = std::format(R"(<anim><events><event><type>1</type>
            <startTime>0</startTime><endTime>1</endTime><params>
            <param name="BehaviorJudgeID" value="6"/><param name="StateInfo" value="{}"/>
            </params></event></events></anim>)", value);
        REQUIRE(tae.load_string(xml.c_str()));
        const auto events = extractAttackEvents(tae, 30000);
        REQUIRE(events.size() == 1);
        REQUIRE(events[0].stateInfo);
        CHECK(std::to_string(*events[0].stateInfo) == value);
        const auto restored = erdo::json::read<AttackEvent>(erdo::json::write(events[0]));
        CHECK(restored.stateInfo == events[0].stateInfo);
    }
    for (const auto value : {"", "bad", "187junk", "2147483648", "-2147483649"}) {
        const auto xml = std::format(R"(<anim><events><event><type>1</type>
            <startTime>0</startTime><endTime>1</endTime><params>
            <param name="BehaviorJudgeID" value="6"/><param name="StateInfo" value="{}"/>
            </params></event></events></anim>)", value);
        REQUIRE(tae.load_string(xml.c_str()));
        CAPTURE(value);
        REQUIRE_THROWS_WITH(extractAttackEvents(tae, 30000), Catch::Matchers::ContainsSubstring("StateInfo"));
    }
    REQUIRE(tae.load_string(R"(<anim><events><event><type>1</type>
        <startTime>0</startTime><endTime>1</endTime><params>
        <param name="BehaviorJudgeID" value="6"/><param name="StateInfo" value="0"/>
        <param name="StateInfo" value="187"/></params></event></events></anim>)"));
    REQUIRE_THROWS_WITH(extractAttackEvents(tae, 30000), Catch::Matchers::ContainsSubstring("duplicate"));
    // Old JSON has no activation field; preserve its existing ungated behavior.
    const auto legacy = erdo::json::read<AttackEvent>(R"({"animationId":30000,"eventType":1,
        "behaviorJudgeId":6,"attackIndex":0,"source":0,"startTime":0.1,"endTime":0.2})");
    CHECK_FALSE(legacy.stateInfo);
    CHECK(*erdo::calculator::attack_event_active(legacy, {}));
}

TEST_CASE_METHOD(ImporterFixture, "moveset - import and serialization", "[moveset][unit]")
{
    xml_document tae;
    UNSCOPED_INFO("Fixture XML must parse");
    REQUIRE((static_cast<bool>(tae.load_string(taeXml))));
    const auto extracted = extractAttackEvents(tae, 34000);

    struct Scratch {
        std::filesystem::path root = std::filesystem::temp_directory_path() /
            std::format("erdo-moveset-tests-{}", std::chrono::steady_clock::now().time_since_epoch().count());
        Scratch()
        {
            UNSCOPED_INFO("Cannot create isolated test directory");
            REQUIRE((std::filesystem::create_directory(root)));
        }
        ~Scratch() { std::error_code error; std::filesystem::remove_all(root, error); }
    } scratch;
    const auto directory = scratch.root / "chr/c0000-anibnd-dcx-wanibnd/INTERROOT_win64/chr/c0000/tae/a23-tae";
    std::filesystem::create_directories(directory);
    auto write = [&](int animation, std::string_view xml) {
        std::ofstream file(directory / std::format("anim-{:06}.xml", animation));
        file << xml;
        UNSCOPED_INFO("Cannot write fixture");
        REQUIRE((static_cast<bool>(file)));
    };
    write(30000, taeXml);
    write(30010, "<anim><events/></anim>");
    write(30020, R"(<anim><events><event><type>1</type><startTime>0</startTime><endTime>1</endTime><params>
        <param name="BehaviorJudgeID" value="100"/></params></event>
        <event><type>1</type><startTime>1</startTime><endTime>2</endTime><params>
        <param name="BehaviorJudgeID" value="6"/></params></event></events></anim>)");
    write(30030, R"(<anim><events><event><type>1</type></event></events></anim>)");
    constexpr std::array catalog{
        MoveDefinition{MoveId::ONE_HAND_R1_1, "missing", 99999},
        MoveDefinition{MoveId::ONE_HAND_R1_1, "valid", 30000},
        MoveDefinition{MoveId::ONE_HAND_R1_2, "empty", 30010},
        MoveDefinition{MoveId::ONE_HAND_R1_3, "ambiguous", 30020},
        MoveDefinition{MoveId::ONE_HAND_R1_4, "malformed", 30030}
    };
    AnimationCache cache(scratch.root);
    UNSCOPED_INFO("Cache must load valid events");
    REQUIRE((cache.get(23, 30000).size() == 3));
    UNSCOPED_INFO("Animation cache must distinguish motion categories");
    REQUIRE_THROWS_AS(cache.get(20, 30000), std::runtime_error);

    auto writeAlias = [&](int category, int animation, std::string_view xml) {
        const auto aliasDirectory = directory.parent_path() / std::format("a{}-tae", category);
        std::filesystem::create_directories(aliasDirectory);
        std::ofstream file(aliasDirectory / std::format("anim-{:06}.xml", animation));
        file << xml;
        UNSCOPED_INFO("Cannot write alias fixture");
        REQUIRE((static_cast<bool>(file)));
    };
    writeAlias(20, 31750, R"(<anim><header><type>ImportOtherAnim</type><animId>21031751</animId></header>
        <events><event><type>129</type></event></events></anim>)");
    writeAlias(21, 31751, R"(<anim><header><type>ImportOtherAnim</type><animId>23030000</animId></header>
        <events><event><type>304</type></event></events></anim>)");
    const auto& aliased = cache.get(20, 31750);
    UNSCOPED_INFO("Alias chains must preserve origin and requested ID without modifying the source cache");
    REQUIRE((aliased.size() == 3 && aliased[0].animationId == 31750 &&
            aliased[0].sourceMotionCategory == 23 && aliased[0].sourceAnimationId == 30000 &&
            cache.get(23, 30000)[0].animationId == 30000));
    CHECK(aliased[1].stateInfo == 187);
    writeAlias(20, 40000, "<anim><header><type>ImportOtherAnim</type><animId>20040001</animId></header></anim>");
    writeAlias(20, 40001, "<anim><header><type>ImportOtherAnim</type><animId>20040000</animId></header></anim>");
    writeAlias(20, 40002, "<anim><header><type>ImportOtherAnim</type><animId>20099999</animId></header></anim>");
    writeAlias(20, 40003, "<anim><header><type>ImportOtherAnim</type><animId>-1</animId></header></anim>");
    writeAlias(20, 40004, "<anim><header><type>ImportOtherAnim</type></header></anim>");
    writeAlias(20, 40005, R"(<anim><header><type>ImportOtherAnim</type><animId>23030000</animId></header>
        <events><event><type>1</type><startTime>0</startTime><endTime>1</endTime>
        <params><param name="BehaviorJudgeID" value="6"/></params></event></events></anim>)");
    const auto& localAttackAlias = cache.get(20, 40005);
    REQUIRE(localAttackAlias.size() == 3);
    for (std::size_t i = 0; i < localAttackAlias.size(); ++i) {
        auto expected = cache.get(23, 30000)[i];
        expected.animationId = 40005;
        CHECK(erdo::json::write(localAttackAlias[i]) == erdo::json::write(expected));
    }
    // Local attacks cannot fill an empty or missing imported event source.
    writeAlias(20, 40006, R"(<anim><header><type>ImportOtherAnim</type><animId>23030010</animId></header>
        <events><event><type>1</type></event></events></anim>)");
    CHECK(cache.get(20, 40006).empty());
    for (const auto [animation, message] : {
        std::pair{40000, "cycle"}, {40002, "File was not found"}, {40003, "Invalid TAE alias"},
        {40004, "Invalid TAE alias"}
    }) {
        CAPTURE(animation);
        REQUIRE_THROWS_WITH(cache.get(20, animation), Catch::Matchers::ContainsSubstring(message));
    }
    // A failed lookup must not poison later requests, including retries.
    writeAlias(20, 40000, "<anim><header><type>ImportOtherAnim</type><animId>23030000</animId></header></anim>");
    UNSCOPED_INFO("A corrected alias must be retryable");
    REQUIRE((cache.get(20, 40000).size() == 3));
    std::size_t published = 0;
    const auto report = importMoveset(cache, 23, 200, catalog, index, attacks,
        [&](const MoveDefinition& move, const std::vector<ResolvedHit>& hits) {
            UNSCOPED_INFO("Only complete moves may be published");
            REQUIRE((move.name == "valid" && hits.size() == 3));
            ++published;
        });
    UNSCOPED_INFO("Import must continue after missing/malformed/ambiguous/empty TAE");
    REQUIRE((published == 1 && report.importedMoves == 1 && report.requestedMoves == 5 &&
            report.failures.size() == 4));
    const auto saved = erdo::json::write(report);
    const auto restored = erdo::json::read<MovesetImportReport>(saved);
    UNSCOPED_INFO("Existing reflection serializer must preserve the diagnostic report");
    REQUIRE((restored.failures[2].unresolved.front().candidates.size() == 2 &&
            restored.motionCategory == 23 && restored.behaviorVariation == 200));

    const std::array validCatalog{catalog[1]};
    const auto persisted = importMoveset(cache, 23, 200, validCatalog, index, attacks);
    UNSCOPED_INFO("Factory must populate existing hit types");
    REQUIRE((persisted.report.failures.empty() && persisted.moveset->moves.size() == 1 &&
            persisted.moveset->moves[0].hits.size() == 3));
    auto phaseXml = std::string(taeXml);
    phaseXml.replace(phaseXml.find("value=\"6\""), 9, "value=\"3\""); // Unrelated non-attack behavior reference.
    write(30040, phaseXml);
    const std::array phaseCatalog{
        MoveDefinition{MoveId::MOUNTED_CHARGED_R2, "selected phase", 30040, 1, std::nullopt, 6},
        MoveDefinition{MoveId::MOUNTED_R2_CHARGING, "missing phase", 30040, 1, std::nullopt, 404},
        MoveDefinition{MoveId::MOUNTED_R2, "wrong category", 30040, 12, std::nullopt, 6},
        MoveDefinition{MoveId::BACKSTAB, "unfiltered", 30040}
    };
    const auto phases = importMoveset(cache, 23, 200, phaseCatalog, index, attacks);
    UNSCOPED_INFO("Phase selection must fail on missing events and still enforce behavior constraints");
    REQUIRE((phases.report.importedMoves == 1 && phases.report.failures.size() == 3 &&
            phases.report.failures[0].diagnostics[0].contains("selected judge 404")));
    const auto& phaseHits = phases.moveset->moves[0].hits;
    UNSCOPED_INFO("Phase selection must preserve repeated events, absolute timing, and the full cached animation");
    REQUIRE((phaseHits.size() == 2 && phaseHits[0].event.startTime == .1 &&
            phaseHits[1].event.startTime == .2 && phaseHits[0].event.eventType == 304 &&
            phaseHits[1].event.source == 1 && cache.get(23, 30040).size() == 3));
    auto criticalXml = phaseXml;
    criticalXml.insert(criticalXml.find("</events>"), R"(<event><type>304</type>
        <startTime>0.8</startTime><endTime>0.9</endTime><params>
        <param name="BehaviorJudgeID" value="6"/><param name="Index" value="8"/></params></event>)");
    write(30050, criticalXml);
    MoveDefinition critical{MoveId::BACKSTAB, "throw victim", 30050};
    critical.eventType = 304;
    const auto victim = importMoveset(cache, 23, 200, std::array{critical}, index, attacks);
    REQUIRE(victim.report.failures.empty());
    const auto& victimHits = victim.moveset->moves[0].hits;
    REQUIRE(victimHits.size() == 2);
    CHECK(victimHits[0].event.startTime == .1);
    CHECK(victimHits[1].event.startTime == .8);
    CHECK(victimHits[0].event.attackIndex == 7);
    CHECK(victimHits[1].event.attackIndex == 8);
    CHECK(cache.get(23, 30050).size() == 4);
    for (const int type : {1, 307}) {
        auto invalid = critical;
        invalid.eventType = type;
        CHECK_FALSE(importMoveset(cache, 23, 200, std::array{invalid}, index, attacks).report.failures.empty());
    }
    auto missingType = critical;
    missingType.animationId = 30000;
    missingType.eventJudgeId = 404;
    CHECK_FALSE(importMoveset(cache, 23, 200, std::array{missingType}, index, attacks).report.failures.empty());
    auto unfilteredCritical = critical;
    unfilteredCritical.eventType.reset();
    CHECK_FALSE(importMoveset(cache, 23, 200, std::array{unfilteredCritical}, index, attacks).report.failures.empty());
    const auto typePos = criticalXml.find("<type>304</type>");
    const auto judgePos = criticalXml.find("value=\"6\"", typePos);
    criticalXml.replace(judgePos, 9, "value=\"3\"");
    write(30060, criticalXml);
    critical.animationId = 30060;
    // A bad selected throw hit invalidates the entire move, even with another
    // valid throw hit. Filtering is context selection, never error recovery.
    const auto brokenVictim = importMoveset(cache, 23, 200, std::array{critical}, index, attacks);
    CHECK(brokenVictim.report.importedMoves == 0);
    CHECK(brokenVictim.report.failures.size() == 1);
    erdo::calculator::GameData gameData{"test", 0,
        {erdo::calculator::Weapon::dummy, erdo::calculator::Weapon::dummy}};
    for (auto& weapon : gameData.weapons)
        weapon.moveset = persisted.moveset;
    const auto restoredData = erdo::json::read<erdo::calculator::GameData>(erdo::json::write(gameData));
    const auto& storedMoveset = restoredData.weapons[0].moveset;
    UNSCOPED_INFO("Existing shared-pointer serializer must preserve moveset identity");
    REQUIRE((storedMoveset && storedMoveset == restoredData.weapons[1].moveset));
    const auto& storedHit = storedMoveset->moves[0].hits[0];
    UNSCOPED_INFO("Moveset JSON round-trip must preserve all MV flags, timing, and provenance");
    REQUIRE((storedHit.motionValues.values == persisted.moveset->moves[0].hits[0].motionValues.values &&
            storedHit.motionValues.known == persisted.moveset->moves[0].hits[0].motionValues.known &&
            storedHit.event.startTime == .1 && storedHit.event.endTime == .3 &&
            storedHit.event.eventType == 304 && storedHit.event.source == -1 &&
            storedHit.behaviorParamId == 6 && storedHit.behaviorCategory == 1 && storedHit.atkParamId == 200100 &&
            storedHit.event.sourceMotionCategory == 23 && storedHit.event.sourceAnimationId == 30000));
    const std::array aliasCatalog{MoveDefinition{MoveId::LARGE_PVE_RIPOSTE, "alias", 31750}};
    const auto aliasMoveset = importMoveset(cache, 20, 200, aliasCatalog, index, attacks);
    const auto aliasRoundTrip = erdo::json::read<erdo::calculator::Moveset>(erdo::json::write(*aliasMoveset.moveset));
    UNSCOPED_INFO("Alias source and requested context must survive serialization");
    REQUIRE((aliasRoundTrip.motionCategory == 20 && aliasRoundTrip.moves[0].hits[0].event.animationId == 31750 &&
            aliasRoundTrip.moves[0].hits[0].event.sourceMotionCategory == 23 &&
            aliasRoundTrip.moves[0].hits[0].event.sourceAnimationId == 30000));
    CHECK(aliasRoundTrip.moves[0].hits[1].event.stateInfo == 187);
    UNSCOPED_INFO("Old event JSON without source-file provenance must remain readable");
    REQUIRE((!erdo::json::read<AttackEvent>(erdo::json::write(extracted[0])).sourceMotionCategory));
    auto oldWeaponJson = erdo::json::write(gameData.weapons[0]);
    const auto movesetField = oldWeaponJson.find(",\"moveset\":");
    UNSCOPED_INFO("Expected final moveset field in serialized weapon");
    REQUIRE((movesetField != std::string::npos));
    oldWeaponJson.erase(movesetField);
    oldWeaponJson += '}';
    UNSCOPED_INFO("Older weapon JSON without movesets must still deserialize");
    REQUIRE((!erdo::json::read<erdo::calculator::Weapon>(oldWeaponJson).moveset));

    std::filesystem::create_directories(scratch.root / AtkParam_PcFile.parent_path());
    UNSCOPED_INFO("Cannot write parameter fixtures");
    REQUIRE((attacks.document.save_file((scratch.root / AtkParam_PcFile).string().c_str()) &&
            behaviors.document.save_file((scratch.root / BehaviorParam_PcFile).string().c_str())));
    const std::map<long long, std::map<std::string, double>> supportedWeapons{
        {1000000, {{"wepmotionCategory", 20}, {"behaviorVariationId", 100},
                   {"wepType", 1}, {"spAtkcategory", 0}, {"isWeaponCatalyst", 0}, {"isDualBlade", 0}, {"enableThrow", 1},
                   {"wepmotionOneHandId", 0}, {"wepmotionBothHandId", 10}}},
        {2000000, {{"wepmotionCategory", 23}, {"behaviorVariationId", 200},
                   {"wepType", 3}, {"spAtkcategory", 0}, {"isWeaponCatalyst", 0}, {"isDualBlade", 0}, {"enableThrow", 1},
                   {"wepmotionOneHandId", 0}, {"wepmotionBothHandId", 10}}}
    };
    std::vector<MovesetImportReport> failures{report};
    REQUIRE_THROWS_WITH(importWeaponMovesets(scratch.root, supportedWeapons, &failures),
        Catch::Matchers::ContainsSubstring("Weapon 1000000") && Catch::Matchers::ContainsSubstring("Weapon 2000000"));
    UNSCOPED_INFO("Supported imports must append all reports then throw a combined error");
    REQUIRE((failures.size() == 3 && !failures[1].failures.empty() && !failures[2].failures.empty()));
    failures.clear();
    REQUIRE_THROWS_WITH(importWeaponMovesets(scratch.root, supportedWeapons, &failures,
        [](auto, auto definitions) {
            return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
        }), Catch::Matchers::ContainsSubstring("preserve complete catalog IDs and order"));
    REQUIRE(failures.size() == 2);
    for (const auto& failure : failures) {
        CHECK(failure.importedMoves == 0);
        CHECK(failure.failures.size() == failure.requestedMoves);
    }
    auto unsupported = supportedWeapons;
    unsupported.at(1000000).at("behaviorVariationId") = 200;
    unsupported.at(2000000).at("wepmotionOneHandId") = 1;
    unsupported.emplace(1100000, supportedWeapons.at(1000000));
    unsupported.at(1100000).at("spAtkcategory") = 104;
    UNSCOPED_INFO("Unverified routing and behavior families must remain unsupported");
    REQUIRE((importWeaponMovesets(scratch.root, unsupported, nullptr).empty()));
    // A matching signature can share the family regardless of its weapon ID.
    auto equivalent = supportedWeapons;
    equivalent.emplace(7770000, supportedWeapons.at(1000000));
    std::vector<long long> resolvedIds;
    REQUIRE_THROWS(importWeaponMovesets(scratch.root, equivalent, nullptr,
        [&](auto ids, auto definitions) {
            resolvedIds.insert(resolvedIds.end(), ids.begin(), ids.end());
            return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
        }));
    CHECK(resolvedIds == std::vector<long long>{1000000, 7770000, 2000000});
    // Exact variations use separate behavior lookup contexts within a family.
    equivalent.emplace(8880000, supportedWeapons.at(1000000));
    equivalent.at(8880000).at("behaviorVariationId") = 104;
    std::vector<std::vector<long long>> groups;
    failures.clear();
    REQUIRE_THROWS(importWeaponMovesets(scratch.root, equivalent, &failures,
        [&](auto ids, auto definitions) {
            groups.emplace_back(ids.begin(), ids.end());
            return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
        }));
    CHECK(groups == std::vector<std::vector<long long>>{{1000000, 7770000}, {8880000}, {2000000}});
    REQUIRE(failures.size() == 3);
    CHECK(failures[0].behaviorVariation == 100);
    CHECK(failures[1].behaviorVariation == 104);
    CHECK(failures[2].behaviorVariation == 200);
    // Hammer membership is also parameter-based, including non-base variations.
    auto hammerRow = supportedWeapons.at(2000000);
    hammerRow.at("wepType") = 21;
    hammerRow.at("wepmotionCategory") = 33;
    hammerRow.at("behaviorVariationId") = 1110;
    const std::map<long long, std::map<std::string, double>> hammerWeapons{
        {1234567, hammerRow}, {7654321, hammerRow}};
    failures.clear();
    groups.clear();
    REQUIRE_THROWS(importWeaponMovesets(scratch.root, hammerWeapons, &failures,
        [&](auto ids, auto definitions) {
            groups.emplace_back(ids.begin(), ids.end());
            return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
        }));
    CHECK(groups == std::vector<std::vector<long long>>{{1234567, 7654321}});
    REQUIRE(failures.size() == 1);
    CHECK(failures[0].motionCategory == 33);
    CHECK(failures[0].behaviorVariation == 1110);
    CHECK(failures[0].requestedMoves == 63);
    auto flailRow = hammerRow;
    flailRow.at("wepType") = 24;
    flailRow.at("wepmotionCategory") = 34;
    flailRow.at("behaviorVariationId") = 1302;
    failures.clear();
    REQUIRE_THROWS(importWeaponMovesets(scratch.root, {{1234567, flailRow}}, &failures,
        [](auto, auto definitions) {
            return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
        }));
    REQUIRE(failures.size() == 1);
    CHECK(failures[0].motionCategory == 34);
    CHECK(failures[0].behaviorVariation == 1302);
    CHECK(failures[0].requestedMoves == 60);
    for (const auto [type, category, variation, position, count] : {
        std::tuple{5, 25, 303, 2, 59}, {19, 32, 1511, 2, 59}, {23, 35, 1201, 2, 59},
        {13, 29, 905, 0, 63}, {11, 40, 804, 2, 59}, {7, 26, 408, 2, 56}, {41, 31, 2312, 2, 56},
        {29, 38, 1800, 3, 59}, {31, 50, 1906, 3, 57}, {25, 36, 1615, 3, 60}, {28, 37, 1708, 3, 57},
        {39, 43, 2003, 0, 51}, {15, 27, 500, 0, 71}, {9, 28, 703, 0, 70}, {94, 61, 6600, 2, 63},
        {93, 60, 6700, 3, 61}, {90, 57, 6100, 3, 52}}) {
        auto heavyRow = hammerRow;
        heavyRow.at("wepType") = type;
        heavyRow.at("wepmotionCategory") = category;
        heavyRow.at("behaviorVariationId") = variation;
        heavyRow.at("wepmotionOneHandId") = position;
        heavyRow.at("wepmotionBothHandId") = category == 60 ? 10 : position + 10;
        heavyRow.at("enableThrow") = category == 43 ? 0 : 1;
        failures.clear();
        REQUIRE_THROWS(importWeaponMovesets(scratch.root, {{1234567, heavyRow}}, &failures,
            [](auto, auto definitions) {
                return std::vector<MoveDefinition>(definitions.begin(), definitions.end() - 1);
            }));
        REQUIRE(failures.size() == 1);
        CHECK(failures[0].motionCategory == category);
        CHECK(failures[0].behaviorVariation == variation);
        CHECK(failures[0].requestedMoves == count);
        // An arbitrary ID qualifies only with every complete signature field.
        for (const auto& [field, value] : heavyRow) {
            CAPTURE(category, field);
            auto changed = heavyRow;
            changed.at(field) = value + (field == "behaviorVariationId" ? 100 : 1);
            CHECK(importWeaponMovesets(scratch.root, {{1234567, changed}}, nullptr).empty());
            changed.erase(field);
            CHECK(importWeaponMovesets(scratch.root, {{1234567, changed}}, nullptr).empty());
        }
    }
    for (const auto& [field, value] : hammerRow) {
        CAPTURE(field);
        auto changed = hammerWeapons;
        changed.erase(7654321);
        changed.at(1234567).at(field) = value + (field == "behaviorVariationId" ? 100 : 1);
        CHECK(importWeaponMovesets(scratch.root, changed, nullptr).empty());
        changed.at(1234567).erase(field);
        CHECK(importWeaponMovesets(scratch.root, changed, nullptr).empty());
    }
    for (const double variation : {99.0, 200.0, 100.5,
            std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        auto invalid = supportedWeapons;
        invalid.erase(2000000);
        invalid.at(1000000).at("behaviorVariationId") = variation;
        CHECK(importWeaponMovesets(scratch.root, invalid, nullptr).empty());
    }
    // Each routing input is required. Missing data is not an implicit default
    // in this already-default-expanded parameter map.
    for (const auto& [field, value] : supportedWeapons.at(1000000)) {
        CAPTURE(field);
        auto changed = supportedWeapons;
        changed.erase(2000000);
        changed.at(1000000).at(field) = value + (field == "behaviorVariationId" ? 100 : 1);
        CHECK(importWeaponMovesets(scratch.root, changed, nullptr).empty());
        changed.at(1000000).erase(field);
        CHECK(importWeaponMovesets(scratch.root, changed, nullptr).empty());
    }
    UNSCOPED_INFO("Imports must preserve retained XML");
    REQUIRE((std::filesystem::exists(directory / "anim-030000.xml")));

}

#ifdef ERDO_MOVESET_XML_ROOT
TEST_CASE("moveset - player weapon lookup with game XML", "[moveset][game_xml]")
{
    const std::filesystem::path root{ERDO_MOVESET_XML_ROOT};
    ParamTable attacks;
    attacks.load((root / AtkParam_PcFile).string().c_str());
    ParamTable behaviors;
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    std::size_t moveCount = 0;
    std::size_t hitCount = 0;
    for (const int category : {20, 23}) {
        const int variation = category == 20 ? 100 : 200;
        const auto reference = category == 20 ? std::span<const MoveDefinition>(daggerCatalog)
                                              : std::span<const MoveDefinition>(straightSwordCatalog);
        const auto expected = importMoveset(cache, category, variation, reference, index, attacks);
        REQUIRE(expected.report.failures.empty());
        std::vector<MoveDefinition> definitions(reference.begin(), reference.end());
        for (auto& definition : definitions) {
            definition.requiredCategory.reset();
            definition.requiredBehaviorParamId.reset();
        }
        const auto actual = importMoveset(cache, category, variation, definitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        CAPTURE(category);
        INFO(erdo::json::write(actual.report));
        REQUIRE(actual.report.failures.empty());
        // Compare complete values, including all channels, timing, phases and alias provenance.
        CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*expected.moveset));
        moveCount += actual.moveset->moves.size();
        for (const auto& move : actual.moveset->moves)
            hitCount += move.hits.size();
    }
    CHECK(moveCount == 129);
    CHECK(hitCount == 149);

    // Broadsword's variation has an ordinary heavy override but no light row.
    // The missing light row inherits the hundred-group row, not another namespace.
    for (const auto [animation, behavior, attack] : {
        std::array{30000, 100200000, 200000}, std::array{30505, 100201100, 201100}
    }) {
        const auto resolved = resolveAttackEvents(cache.get(23, animation), 201, index, attacks,
            std::nullopt, std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        CAPTURE(animation);
        REQUIRE(resolved.diagnostics.empty());
        REQUIRE(resolved.hits.size() == 1);
        CHECK(resolved.hits[0].behaviorParamId == behavior);
        CHECK(resolved.hits[0].atkParamId == attack);
    }
}

TEST_CASE("moveset - game XML", "[moveset][game_xml]")
{
    const std::filesystem::path root{ERDO_MOVESET_XML_ROOT};
    ParamTable attacks;
    attacks.load((root / AtkParam_PcFile).string().c_str());
    ParamTable behaviors;
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);

    testAnimation(root, attacks, index, 20, 34000, 100, {
        {805, 80, 50, 2, .33333334, .43333334},
        {806, 80, 50, 2, .56666666, .63333330},
        {800, 80, 50, 1, .83333330, .93333334}
    });
    testAnimation(root, attacks, index, 23, 30000, 200, {
        {0, 100, 100, 0, .43333334, .53333336}
    });
    testAnimation(root, attacks, index, 23, 34000, 200, {
        {800, 92, 65, 1, .43333334, .53333336},
        {805, 92, 65, 2, .46666667, .6}
    });
    testAnimation(root, attacks, index, 23, 31750, 200, {
        {560, 132, 100, -1, .76666667, .86666667},
        {561, 331, 100, -1, 2.7333333, 2.8333333}
    });

    AnimationCache cache(root);
    struct ProvenanceCase {
        int motionCategory;
        int variation;
        int animation;
        std::vector<int> judges;
        std::vector<int> sources;
        std::vector<int> attackIndices;
        int eventType;
    };
    const std::array provenanceCases{
        ProvenanceCase{20, 100, 34000, {805, 806, 800}, {2, 2, 1}, {1, 1, 0}, 1},
        ProvenanceCase{23, 200, 30000, {0}, {0}, {0}, 1},
        ProvenanceCase{23, 200, 34000, {800, 805}, {1, 2}, {0, 1}, 1},
        ProvenanceCase{23, 200, 31750, {560, 561}, {-1, -1}, {0, 0}, 304}
    };
    for (const auto& expected : provenanceCases) {
        const auto resolved = resolveAttackEvents(cache.get(expected.motionCategory, expected.animation),
            expected.variation, index, attacks);
        UNSCOPED_INFO("Provenance hit count mismatch");
        REQUIRE((resolved.hits.size() == expected.judges.size()));
        for (std::size_t i = 0; i != resolved.hits.size(); ++i) {
            const auto& hit = resolved.hits[i];
            const auto attackId = expected.variation * 1000 + expected.judges[i];
            UNSCOPED_INFO("Ordered TAE/behavior/attack provenance mismatch");
            REQUIRE((hit.event.animationId == expected.animation && hit.event.eventType == expected.eventType &&
                    hit.event.behaviorJudgeId == expected.judges[i] && hit.event.source == expected.sources[i] &&
                    hit.event.attackIndex == expected.attackIndices[i] && hit.atkParamId == attackId &&
                    hit.behaviorParamId == 100000000 + attackId));
        }
    }
    for (const auto [animation, judge, attack, physical] : {
        std::array{30505, 100, 200100, 125},
        std::array{31030, 150, 200150, 107}
    }) {
        const auto& events = cache.get(23, animation);
        const auto unfiltered = resolveAttackEvents(events, 200, index, attacks);
        UNSCOPED_INFO("Unfiltered duplicate must be ambiguous");
        REQUIRE((!unfiltered.diagnostics.empty()));
        const auto filtered = resolveAttackEvents(events, 200, index, attacks, 1);
        UNSCOPED_INFO("Category 1 must resolve ordinary Longsword attack");
        REQUIRE((filtered.diagnostics.empty() && filtered.hits.size() == 1));
        const auto& hit = filtered.hits.front();
        UNSCOPED_INFO("Wrong category-specific attack");
        REQUIRE((hit.behaviorCategory == 1 && hit.atkParamId == attack &&
                hit.event.behaviorJudgeId == judge && hit.motionValues.values[0] == physical));
        const auto special = resolveAttackEvents(events, 200, index, attacks, 12);
        UNSCOPED_INFO("Category 12 must remain independently resolvable");
        REQUIRE((special.diagnostics.empty() && special.hits.size() == 1 &&
                special.hits.front().behaviorCategory == 12));
    }

    std::size_t count = 0;
    const auto report = importMoveset(cache, 23, 200, straightSwordCatalog, index, attacks,
        [&](const MoveDefinition&, const std::vector<ResolvedHit>&) { ++count; });
    UNSCOPED_INFO("Expected all 63 Longsword moves to resolve");
    REQUIRE((count == 63 && report.failures.empty()));
    std::println("Longsword catalog: {}/63 passed", count);

    struct ExpectedHit {
        int judge;
        float physical;
        float status;
        int source;
        int attackIndex;
        int category;
        double start;
        double end;
        int sourceCategory = -1;
        int sourceAnimation = -1;
        int attackId = -1;
        int behaviorId = -1;
        std::array<float, 4> otherDamage{-1, -1, -1, -1};
        int stateInfo = 0;
        bool opposeTarget = true;
    };
    struct ExpectedMove {
        MoveId id;
        int animation;
        std::vector<ExpectedHit> hits;
    };
    const std::array<ExpectedMove, 66> expectedDaggerMoves{{
        #include "expected_dagger_moves.inc"
    }};
    const auto dagger = importMoveset(cache, 20, 100, daggerCatalog, index, attacks);
    UNSCOPED_INFO("Expected all 66 Dagger moves to resolve");
    REQUIRE((dagger.report.failures.empty() && dagger.report.importedMoves == expectedDaggerMoves.size() &&
            dagger.moveset->moves.size() == expectedDaggerMoves.size()));
    const auto checkMoves = [](const erdo::calculator::Moveset& moveset, std::span<const ExpectedMove> expectedMoves) {
      std::size_t hitCount = 0;
      for (const auto& expected : expectedMoves) {
        const bool critical = expected.animation >= 31700 && expected.animation <= 31750;
        const auto move = std::ranges::find(moveset.moves, expected.id, &erdo::calculator::Move::id);
        UNSCOPED_INFO("Move mapping or hit count mismatch");
        REQUIRE((move != moveset.moves.end() && move->animationId == expected.animation &&
                move->hits.size() == expected.hits.size()));
        for (std::size_t i = 0; i < expected.hits.size(); ++i) {
            const auto& hit = move->hits[i];
            const auto& check = expected.hits[i];
            REQUIRE(hit.event.stateInfo.value_or(0) == check.stateInfo);
            REQUIRE(hit.opposeTarget == check.opposeTarget);
            UNSCOPED_INFO(std::format("Category {} animation {:06} hit {} provenance mismatch", moveset.motionCategory, expected.animation, i));
            REQUIRE((hit.event.behaviorJudgeId == check.judge && hit.event.source == check.source &&
                    hit.event.attackIndex == check.attackIndex && hit.behaviorCategory == check.category &&
                    hit.atkParamId == (check.attackId >= 0 ? check.attackId : moveset.behaviorVariation * 1000 + check.judge) &&
                    hit.behaviorParamId == (check.behaviorId >= 0 ? check.behaviorId
                        : 100000000 + moveset.behaviorVariation * 1000 + check.judge) &&
                    hit.event.animationId == expected.animation &&
                    hit.event.eventType == (critical && expected.id != MoveId::BACKSTAB_WHIFF ? 304 : 1)));
            UNSCOPED_INFO("Physical MV or known flags mismatch");
            REQUIRE((hit.motionValues.values[0] == check.physical &&
                    std::ranges::all_of(hit.motionValues.known, [](bool known) { return known; })));
            for (std::size_t channel = 1; channel < 5; ++channel) {
                if (check.otherDamage[channel - 1] >= 0)
                    CHECK(hit.motionValues.values[channel] == check.otherDamage[channel - 1]);
                else if (check.sourceCategory >= 0)
                    CHECK(hit.motionValues.values[channel] == check.physical);
            }
            for (std::size_t channel = 5; channel < hit.motionValues.values.size(); ++channel) {
                UNSCOPED_INFO("Status channel mismatch");
                REQUIRE((hit.motionValues.values[channel] == check.status));
            }
            UNSCOPED_INFO(std::format("Category {} animation {:06} hit {} timing mismatch", moveset.motionCategory, expected.animation, i));
            REQUIRE((std::abs(hit.event.startTime - check.start) < .000001 &&
                    std::abs(hit.event.endTime - check.end) < .000001));
            UNSCOPED_INFO("Alias origin must be retained without changing the behavior variation");
            REQUIRE((hit.event.sourceMotionCategory == (check.sourceCategory >= 0 ? check.sourceCategory : critical ? 23 : moveset.motionCategory) &&
                    hit.event.sourceAnimationId == (check.sourceAnimation >= 0 ? check.sourceAnimation : expected.animation == 31730 ? 31720 : expected.animation)));
            ++hitCount;
        }
      }
      return hitCount;
    };
    const auto daggerHits = checkMoves(*dagger.moveset, expectedDaggerMoves);
    UNSCOPED_INFO("Expected 77 ordered Dagger hits");
    REQUIRE((daggerHits == 77));
    const std::array<ExpectedMove, 29> expectedLongswordAdditions{{
        #include "expected_longsword_extended_moves.inc"
    }};
    const auto longsword = importMoveset(cache, 23, 200, straightSwordCatalog, index, attacks);
    UNSCOPED_INFO("Expected 36 ordered hits across 29 additional Longsword moves");
    REQUIRE((checkMoves(*longsword.moveset, expectedLongswordAdditions) == 36));
    const std::array<ExpectedMove, 63> expectedAxeMoves{{
        #include "expected_axe_moves.inc"
    }};
    const auto axe = importMoveset(cache, 30, 1400, axeCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(axe.report.failures.empty());
    REQUIRE(axe.moveset->moves.size() == expectedAxeMoves.size());
    CHECK(checkMoves(*axe.moveset, expectedAxeMoves) == 77);
    const std::array<ExpectedMove, 63> expectedHammerMoves{{
        #include "expected_hammer_moves.inc"
    }};
    const auto hammer = importMoveset(cache, 33, 1100, hammerCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(hammer.report.failures.empty());
    REQUIRE(hammer.moveset->moves.size() == expectedHammerMoves.size());
    CHECK(checkMoves(*hammer.moveset, expectedHammerMoves) == 77);
    const std::array<ExpectedMove, 60> expectedFlailMoves{{
        #include "expected_flail_moves.inc"
    }};
    const auto flail = importMoveset(cache, 34, 1300, flailCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(flail.report.failures.empty());
    REQUIRE(flail.moveset->moves.size() == expectedFlailMoves.size());
    CHECK(checkMoves(*flail.moveset, expectedFlailMoves) == 78);
    const std::array<ExpectedMove, 59> expectedGreatswordMoves{{
        #include "expected_greatsword_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedAlabasterMoves{{
        #include "expected_alabaster_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedGreataxeMoves{{
        #include "expected_greataxe_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedGreathammerMoves{{
        #include "expected_greathammer_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedBeastclawMoves{{
        #include "expected_beastclaw_moves.inc"
    }};
    const auto greatsword = importMoveset(cache, 25, 300, greatswordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto alabaster = importMoveset(cache, 25, 303, greatswordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto greataxe = importMoveset(cache, 32, 1500, greataxeCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto greathammer = importMoveset(cache, 35, 1200, greathammerCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto beastclaw = importMoveset(cache, 35, 1201, greathammerCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    for (const auto* imported : {&greatsword, &alabaster, &greataxe, &greathammer, &beastclaw}) {
        REQUIRE(imported->report.failures.empty());
        REQUIRE(imported->moveset->moves.size() == 59);
    }
    CHECK(checkMoves(*greatsword.moveset, expectedGreatswordMoves) == 67);
    CHECK(checkMoves(*alabaster.moveset, expectedAlabasterMoves) == 67);
    CHECK(checkMoves(*greataxe.moveset, expectedGreataxeMoves) == 71);
    CHECK(checkMoves(*greathammer.moveset, expectedGreathammerMoves) == 81);
    CHECK(checkMoves(*beastclaw.moveset, expectedBeastclawMoves) == 81);
    const std::array<ExpectedMove, 63> expectedKatanaMoves{{
        #include "expected_katana_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedCurvedGreatswordMoves{{
        #include "expected_curved_greatsword_moves.inc"
    }};
    const std::array<ExpectedMove, 59> expectedBloodhoundMoves{{
        #include "expected_bloodhound_moves.inc"
    }};
    const auto katana = importMoveset(cache, 29, 900, katanaCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto curvedGreatsword = importMoveset(cache, 40, 800, curvedGreatswordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto bloodhound = importMoveset(cache, 40, 804, curvedGreatswordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    for (const auto* imported : {&katana, &curvedGreatsword, &bloodhound})
        REQUIRE(imported->report.failures.empty());
    REQUIRE(katana.moveset->moves.size() == 63);
    REQUIRE(curvedGreatsword.moveset->moves.size() == 59);
    REQUIRE(bloodhound.moveset->moves.size() == 59);
    CHECK(checkMoves(*katana.moveset, expectedKatanaMoves) == 72);
    CHECK(checkMoves(*curvedGreatsword.moveset, expectedCurvedGreatswordMoves) == 73);
    CHECK(checkMoves(*bloodhound.moveset, expectedBloodhoundMoves) == 73);
    const std::array<ExpectedMove, 56> expectedColossalSwordMoves{{
        #include "expected_colossal_sword_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2300{{
        #include "expected_colossal_weapon_2300_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2301{{
        #include "expected_colossal_weapon_2301_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2306{{
        #include "expected_colossal_weapon_2306_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2307{{
        #include "expected_colossal_weapon_2307_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2308{{
        #include "expected_colossal_weapon_2308_moves.inc"
    }};
    const std::array<ExpectedMove, 56> expectedColossalWeapon2312{{
        #include "expected_colossal_weapon_2312_moves.inc"
    }};
    struct ColossalExpectation { int category; int variation; std::span<const ExpectedMove> moves; int hits; };
    const std::array colossalExpectations{
        ColossalExpectation{26, 400, expectedColossalSwordMoves, 97},
        ColossalExpectation{31, 2300, expectedColossalWeapon2300, 112},
        ColossalExpectation{31, 2301, expectedColossalWeapon2301, 112},
        ColossalExpectation{31, 2306, expectedColossalWeapon2306, 112},
        ColossalExpectation{31, 2307, expectedColossalWeapon2307, 112},
        ColossalExpectation{31, 2308, expectedColossalWeapon2308, 112},
        ColossalExpectation{31, 2312, expectedColossalWeapon2312, 112}};
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> colossalReferences;
    for (const auto& [category, variation, expectedMoves, hits] : colossalExpectations) {
        CAPTURE(category, variation);
        const auto imported = importMoveset(cache, category, variation,
            category == 26 ? colossalSwordCatalog : colossalWeaponCatalog, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(imported.report.failures.empty());
        REQUIRE(imported.moveset->moves.size() == 56);
        CHECK(checkMoves(*imported.moveset, expectedMoves) == hits);
        colossalReferences.emplace(variation, imported.moveset);
    }
    const std::array<ExpectedMove, 59> expectedHalberdMoves{{
        #include "expected_halberd_moves.inc"
    }};
    const std::array<ExpectedMove, 57> expectedScytheMoves{{
        #include "expected_scythe_1900_moves.inc"
    }};
    const auto halberd = importMoveset(cache, 38, 1800, halberdCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto scythe = importMoveset(cache, 50, 1900, scytheCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(halberd.report.failures.empty());
    REQUIRE(scythe.report.failures.empty());
    CHECK(checkMoves(*halberd.moveset, expectedHalberdMoves) == 69);
    CHECK(checkMoves(*scythe.moveset, expectedScytheMoves) == 67);
    const std::array<ExpectedMove, 60> expectedSpearMoves{{
        #include "expected_spear_1600_moves.inc"
    }};
    const std::array<ExpectedMove, 60> expectedSpear1615Moves{{
        #include "expected_spear_1615_moves.inc"
    }};
    const auto spear = importMoveset(cache, 36, 1600, spearCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto spear1615 = importMoveset(cache, 36, 1615, spearCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(spear.report.failures.empty());
    REQUIRE(spear1615.report.failures.empty());
    CHECK(checkMoves(*spear.moveset, expectedSpearMoves) == 69);
    CHECK(checkMoves(*spear1615.moveset, expectedSpear1615Moves) == 69);
    const std::array<ExpectedMove, 57> expectedGreatSpearMoves{{
        #include "expected_greatspear_1700_moves.inc"
    }};
    const std::array<ExpectedMove, 57> expectedGreatSpear1708Moves{{
        #include "expected_greatspear_1708_moves.inc"
    }};
    const auto greatSpear = importMoveset(cache, 37, 1700, greatSpearCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto greatSpear1708 = importMoveset(cache, 37, 1708, greatSpearCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(greatSpear.report.failures.empty());
    REQUIRE(greatSpear1708.report.failures.empty());
    CHECK(checkMoves(*greatSpear.moveset, expectedGreatSpearMoves) == 115);
    CHECK(checkMoves(*greatSpear1708.moveset, expectedGreatSpear1708Moves) == 115);

    const std::array<ExpectedMove, 51> expectedWhipMoves{{
        #include "expected_whip_2000_moves.inc"
    }};
    const auto whip = importMoveset(cache, 43, 2000, whipCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(whip.report.failures.empty());
    CHECK(checkMoves(*whip.moveset, expectedWhipMoves) == 58);

    const std::array<ExpectedMove, 71> expectedThrustingMoves{{
        #include "expected_thrusting_500_moves.inc"
    }};
    const auto thrusting = importMoveset(cache, 27, 500, thrustingSwordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(thrusting.report.failures.empty());
    CHECK(checkMoves(*thrusting.moveset, expectedThrustingMoves) == 83);
    const std::array<ExpectedMove, 70> expectedCurvedSwordMoves{{
        #include "expected_curved_sword_700_moves.inc"
    }};
    const auto curvedSword = importMoveset(cache, 28, 700, curvedSwordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(curvedSword.report.failures.empty());
    CHECK(checkMoves(*curvedSword.moveset, expectedCurvedSwordMoves) == 87);
    const auto curvedSword703 = importMoveset(cache, 28, 703, curvedSwordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(curvedSword703.report.failures.empty());
    CHECK(checkMoves(*curvedSword703.moveset, expectedCurvedSwordMoves) == 87);
    const std::array<ExpectedMove, 63> expectedGreatKatanaMoves{{
        #include "expected_great_katana_6600_moves.inc"
    }};
    const auto greatKatana = importMoveset(cache, 61, 6600, greatKatanaCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(greatKatana.report.failures.empty());
    CHECK(checkMoves(*greatKatana.moveset, expectedGreatKatanaMoves) == 75);
    const std::array<ExpectedMove, 61> expectedLightGreatswordMoves{{
        #include "expected_light_greatsword_6700_moves.inc"
    }};
    const auto lightGreatsword = importMoveset(cache, 60, 6700, lightGreatswordCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(lightGreatsword.report.failures.empty());
    CHECK(checkMoves(*lightGreatsword.moveset, expectedLightGreatswordMoves) == 153);
    const std::array<ExpectedMove, 52> expectedThrustingShieldMoves{{
        #include "expected_thrusting_shield_6100_moves.inc"
    }};
    const auto thrustingShield = importMoveset(cache, 57, 6100, thrustingShieldCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    REQUIRE(thrustingShield.report.failures.empty());
    CHECK(checkMoves(*thrustingShield.moveset, expectedThrustingShieldMoves) == 55);
    for (const auto [animation, judge] : {std::pair{38100, 638}, std::pair{39100, 738}}) {
        const auto& raw = cache.get(60, animation);
        REQUIRE(raw.size() == 2);
        CHECK(raw[0].behaviorJudgeId == 999);
        CHECK(raw[1].behaviorJudgeId == judge);
    }
    const std::array<ExpectedMove, 4> expectedHeavyThrustingFeints{{
        #include "expected_heavy_thrusting_feints_600_moves.inc"
    }};
    std::vector<MoveDefinition> feints;
    for (const auto& definition : thrustingSwordCatalog)
        if (definition.id == MoveId::ONE_HAND_CHARGED_R2_1_FEINT || definition.id == MoveId::ONE_HAND_CHARGED_R2_2_FEINT
            || definition.id == MoveId::TWO_HAND_CHARGED_R2_1_FEINT || definition.id == MoveId::TWO_HAND_CHARGED_R2_2_FEINT)
            feints.push_back(definition);
    for (const int variation : {600, 601, 602}) {
        const auto imported = importMoveset(cache, 39, variation, feints, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(imported.report.failures.empty());
        CHECK(checkMoves(*imported.moveset, expectedHeavyThrustingFeints) == 4);
    }
    // Non-damaging mounted charge phases are omitted deliberately. Explicitly
    // requesting them still fails; it cannot turn into a fabricated zero hit.
    for (const auto [id, animation, chargingJudge, releaseJudge] : {
            std::tuple{MoveId::MOUNTED_R2_CHARGING, 38100, 630, 635},
            std::tuple{MoveId::MOUNTED_L2_CHARGING, 39100, 730, 735}}) {
        CAPTURE(animation);
        const auto& events = cache.get(43, animation);
        REQUIRE(events.size() == 1);
        CHECK(events.front().behaviorJudgeId == releaseJudge);
        const auto missingPhase = importMoveset(cache, 43, 2000,
            std::array{MoveDefinition{id, "absent whip charge phase", animation,
                std::nullopt, std::nullopt, chargingJudge}}, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        CHECK(missingPhase.report.importedMoves == 0);
        REQUIRE(missingPhase.report.failures.size() == 1);
    }
    // These scythe clips exist and contain release hits, but no charging hits.
    // Keep absent-phase rejection strict; membership omits non-damaging phases.
    for (const auto [animation, releaseJudge] : {std::pair{38100, 635}, {39100, 735}}) {
        const auto events = cache.get(50, animation);
        REQUIRE(events.size() == 1);
        CHECK(events[0].behaviorJudgeId == releaseJudge);
    }
    for (const auto id : {MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING}) {
        const auto charging = std::ranges::find(halberdCatalog, id, &MoveDefinition::id);
        REQUIRE(charging != halberdCatalog.end());
        const auto absentPhase = importMoveset(cache, 50, 1900, std::span(&*charging, 1), index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        CHECK(absentPhase.report.importedMoves == 0);
        REQUIRE(absentPhase.report.failures.size() == 1);
        CHECK(absentPhase.report.failures[0].moveId == id);
    }
    // The raw backstab clip also contains collision judge 592, which has no
    // matching behavior row. Explicit victim selection retains both throw hits;
    // the generic unfiltered importer still rejects the unresolved collision.
    CHECK(cache.get(30, 31710).size() == 3);
    auto unfilteredAxe = *std::ranges::find(axeCatalog, MoveId::BACKSTAB, &MoveDefinition::id);
    unfilteredAxe.eventType.reset();
    CHECK_FALSE(importMoveset(cache, 30, 1400, std::array{unfilteredAxe}, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON).report.failures.empty());
    // Both phases are retained in the cache, even after separate catalog imports.
    for (int category : {20, 23})
        for (int animation : {38100, 39100, 31719}) {
            UNSCOPED_INFO("Phase selection must not mutate cached events");
            REQUIRE((cache.get(category, animation).size() == 2));
        }
    for (const auto [animation, judge, physical, bloodTaxPhysical] : {
        std::array{32515, 310, 125, 77}, std::array{32510, 315, 155, 56}
    }) {
        const auto& events = cache.get(20, animation);
        const auto ambiguous = resolveAttackEvents(events, 100, index, attacks, 12);
        UNSCOPED_INFO("Dagger and Blood Tax must remain ambiguous without explicit context");
        REQUIRE((ambiguous.hits.empty() && ambiguous.unresolved.size() == 1 &&
                ambiguous.unresolved[0].candidates.size() == 2));
        const auto ordinary = resolveAttackEvents(events, 100, index, attacks, 12, 100100000 + judge);
        const auto bloodTax = resolveAttackEvents(events, 100, index, attacks, 12, 300100000 + judge);
        UNSCOPED_INFO("Explicit context must select the intended Dagger/Blood Tax behavior");
        REQUIRE((ordinary.hits.size() == 1 && ordinary.hits[0].motionValues.values[0] == physical &&
                bloodTax.hits.size() == 1 && bloodTax.hits[0].motionValues.values[0] == bloodTaxPhysical));
    }
    UNSCOPED_INFO("Dagger jumping R1 must remain ambiguous without category context");
    REQUIRE((!resolveAttackEvents(cache.get(20, 31030), 100, index, attacks).diagnostics.empty()));
    const auto ambiguousCritical = resolveAttackEvents(cache.get(20, 31720), 100, index, attacks, 12);
    UNSCOPED_INFO("Dagger critical and Repeating Thrust must remain ambiguous without explicit context");
    REQUIRE((ambiguousCritical.hits.empty() && ambiguousCritical.unresolved.size() == 1 &&
            ambiguousCritical.unresolved[0].candidates.size() == 2));
    std::println("Dagger catalog: 66/66 moves and {}/77 hits passed", daggerHits);

    std::vector<MovesetImportReport> reports;
    auto gameData = load_game_data(root, &reports);
    UNSCOPED_INFO("Game-data importer must report every exported behavior group, excluding unused parameter rows");
    REQUIRE(reports.size() == 75);
    for (const auto& report : reports) REQUIRE(report.failures.empty());
    std::shared_ptr<erdo::calculator::Moveset> sharedLongsword;
    std::shared_ptr<erdo::calculator::Moveset> sharedDagger;
    std::size_t longswords = 0;
    std::size_t daggers = 0;
    const std::set<std::string> daggerNames{"Dagger", "Parrying Dagger", "Crystal Knife", "Cinquedea", "Bloodstained Dagger"};
    const std::set<std::string> swordNames{"Longsword", "Lordsworn's Straight Sword", "Weathered Straight Sword",
        "Crystal Sword", "Noble's Slender Sword", "Velvet Sword of St. Trina", "Sword of Light", "Sword of Darkness"};
    const std::map<std::string, int> uniqueVariations{{"Reduvia", 104}, {"Coded Sword", 204},
        {"Sword of St. Trina", 205}, {"Rotten Crystal Sword", 206}, {"Golden Epitaph", 207},
        {"Sword of Night and Flame", 214}, {"Regalia of Eochaid", 215}};
    const std::map<std::string, int> axeVariations{{"Battle Axe", 1400}, {"Jawbone Axe", 1400},
        {"Highland Axe", 1400}, {"Sacrificial Axe", 1400}, {"Rosus' Axe", 1412}, {"Stormhawk Axe", 1414}};
    const std::map<std::string, int> hammerVariations{{"Curved Club", 1100}, {"Warpick", 1100},
        {"Varré's Bouquet", 1100}, {"Hammer", 1100}, {"Flowerstone Gavel", 1100},
        {"Envoy's Horn", 1110}, {"Nox Flowing Hammer", 1112}, {"Ringed Finger", 1113}, {"Marika's Hammer", 1115}};
    const std::map<std::string, int> flailVariations{{"Nightrider Flail", 1300}, {"Flail", 1300},
        {"Chainlink Flail", 1300}, {"Family Heads", 1302}, {"Bastard's Stars", 1301},
        {"Serpent Flail", 1320}, {"Golden Order Flail", 1330}};
    const std::map<std::string, std::pair<int, int>> heavyVariations{
        {"Bastard Sword", {25, 300}}, {"Forked Greatsword", {25, 300}}, {"Iron Greatsword", {25, 300}},
        {"Lordsworn's Greatsword", {25, 300}}, {"Ordovis's Greatsword", {25, 306}},
        {"Alabaster Lord's Sword", {25, 303}}, {"Sacred Relic Sword", {25, 310}},
        {"Helphen's Steeple", {25, 319}}, {"Blasphemous Blade", {25, 314}},
        {"Golden Order Greatsword", {25, 317}}, {"Gargoyle's Greatsword", {25, 300}},
        {"Gargoyle's Blackblade", {25, 311}}, {"Greatsword of Solitude", {25, 300}},
        {"Greataxe", {32, 1500}}, {"Great Omenkiller Cleaver", {32, 1500}}, {"Longhaft Axe", {32, 1500}},
        {"Rusted Anchor", {32, 1500}}, {"Executioner's Greataxe", {32, 1500}}, {"Winged Greathorn", {32, 1511}},
        {"Gargoyle's Great Axe", {32, 1500}}, {"Gargoyle's Black Axe", {32, 1500}},
        {"Large Club", {35, 1200}}, {"Greathorn Hammer", {35, 1200}}, {"Battle Hammer", {35, 1200}},
        {"Great Mace", {35, 1200}}, {"Curved Great Club", {35, 1200}}, {"Celebrant's Skull", {35, 1200}},
        {"Beastclaw Greathammer", {35, 1201}}, {"Envoy's Long Horn", {35, 1216}},
        {"Cranial Vessel Candlestand", {35, 1217}}, {"Great Stars", {35, 1200}}, {"Brick Hammer", {35, 1200}},
        {"Devourer's Scepter", {35, 1202}}, {"Rotten Battle Hammer", {35, 1200}}};
    const std::map<std::string, std::pair<int, int>> bladeVariations{
        {"Star-Lined Sword", {29, 900}}, {"Uchigatana", {29, 900}}, {"Hand of Malenia", {29, 902}},
        {"Rivers of Blood", {29, 906}}, {"Moonveil", {29, 905}}, {"Dragonscale Blade", {29, 910}},
        {"Onyx Lord's Greatsword", {40, 805}}, {"Dismounter", {40, 800}}, {"Bloodhound's Fang", {40, 804}},
        {"Monk's Flameblade", {40, 800}}, {"Beastman's Cleaver", {40, 800}}, {"Morgott's Cursed Sword", {40, 810}},
        {"Freyja's Greatsword", {40, 800}}, {"Horned Warrior's Greatsword", {40, 800}}, {"Hefty Scimitar", {40, 800}}};
    const std::map<std::string, std::pair<int, int>> colossalVariations{
        {"Watchdog's Greatsword", {26, 400}}, {"Troll's Golden Sword", {26, 400}},
        {"Maliketh's Black Blade", {26, 408}}, {"Royal Greatsword", {26, 406}}, {"Grafted Blade Greatsword", {26, 407}},
        {"Anvil Hammer", {31, 2300}}, {"Watchdog's Staff", {31, 2301}}, {"Envoy's Greathorn", {31, 2306}},
        {"Duelist Greataxe", {31, 2312}}, {"Dragon Greatclaw", {31, 2300}}, {"Staff of the Avatar", {31, 2307}},
        {"Fallingstar Beast Jaw", {31, 2308}}, {"Troll's Hammer", {31, 2300}}, {"Rotten Staff", {31, 2307}},
        {"Rotten Greataxe", {31, 2312}}, {"Devonia's Hammer", {31, 2300}}};
    const std::map<std::string, std::pair<int, int>> poleVariations{
        {"Pest's Glaive", {38, 1800}}, {"Ripple Crescent Halberd", {38, 1800}},
        {"Vulgar Militia Saw", {38, 1800}}, {"Glaive", {38, 1800}}, {"Vulgar Militia Shotel", {38, 1800}},
        {"Gargoyle's Halberd", {38, 1800}}, {"Gargoyle's Black Halberd", {38, 1800}}, {"Spirit Glaive", {38, 1800}},
        {"Grave Scythe", {50, 1900}}, {"Halo Scythe", {50, 1902}}, {"Winged Scythe", {50, 1906}}};
    const std::map<std::string, int> spearVariations{
        {"Spear", 1615}, {"Crystal Spear", 1600}, {"Clayman's Harpoon", 1600}, {"Celebrant's Rib-Rake", 1600},
        {"Torchpole", 1600}, {"Bolt of Gransax", 1602}, {"Inquisitor's Girandole", 1600},
        {"Iron Spear", 1600}, {"Rotten Crystal Spear", 1600}, {"Bloodfiend's Fork", 1600}};
    const std::map<std::string, int> greatSpearVariations{
        {"Bloodfiend's Sacred Spear", 1700}, {"Lance", 1708}, {"Mohgwyn's Sacred Spear", 1711}};
    const std::map<std::string, int> whipVariations{
        {"Whip", 2000}, {"Thorned Whip", 2000}, {"Hoslow's Petal Whip", 2000}, {"Tooth Whip", 2000},
        {"Magma Whip Candlestick", 2003}, {"Giant's Red Braid", 2006}};
    const std::set<std::string> thrustingNames{"Cleanrot Knight's Sword", "Rapier", "Antspur Rapier"};
    const std::map<std::string, int> curvedSwordVariations{{"Falchion", 700}, {"Bandit's Curved Sword", 700},
        {"Magma Blade", 703}, {"Scavenger's Curved Sword", 700}, {"Serpent-God's Curved Sword", 700},
        {"Grossmesser", 700}, {"Spirit Sword", 700}};
    const std::set<std::string> greatKatanaNames{"Great Katana", "Dragon-Hunter's Great Katana", "Reed Great Katana"};
    const std::set<std::string> lightGreatswordNames{"Milady", "Leda's Sword", "Idus Sword"};
    const std::set<std::string> thrustingShieldNames{"Dueling Shield", "Carian Thrusting Shield", "Ritual Thrusting Shield"};
    erdo::calculator::GameData supportedData{gameData.erdo_version, gameData.game_version};
    for (const auto& weapon : gameData.weapons) {
        if (swordNames.contains(weapon.base_name)) {
            UNSCOPED_INFO("Longsword affinities must receive the validated catalog");
            REQUIRE((weapon.moveset && weapon.moveset->moves.size() == 63));
            if (sharedLongsword) {
                UNSCOPED_INFO("Equivalent Longsword variants must share their moveset");
                REQUIRE((sharedLongsword == weapon.moveset));
            }
            sharedLongsword = weapon.moveset;
            ++longswords;
        } else if (daggerNames.contains(weapon.base_name)) {
            UNSCOPED_INFO("Dagger affinities must receive the validated catalog");
            REQUIRE((weapon.moveset && weapon.moveset->moves.size() == 66));
            if (sharedDagger) {
                UNSCOPED_INFO("Equivalent Dagger variants must share their moveset");
                REQUIRE((sharedDagger == weapon.moveset));
            }
            sharedDagger = weapon.moveset;
            ++daggers;
        } else if (const auto variation = uniqueVariations.find(weapon.base_name); variation != uniqueVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == (variation->second < 200 ? 20 : 23));
            CHECK(weapon.moveset->moves.size() == (variation->second < 200 ? 66 : 63));
        } else if (const auto variation = axeVariations.find(weapon.base_name); variation != axeVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 30);
            CHECK(weapon.moveset->moves.size() == 63);
        } else if (const auto variation = hammerVariations.find(weapon.base_name); variation != hammerVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 33);
            CHECK(weapon.moveset->moves.size() == 63);
        } else if (const auto variation = flailVariations.find(weapon.base_name); variation != flailVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 34);
            CHECK(weapon.moveset->moves.size() == 60);
        } else if (const auto variation = heavyVariations.find(weapon.base_name); variation != heavyVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second.second);
            CHECK(weapon.moveset->motionCategory == variation->second.first);
            CHECK(weapon.moveset->moves.size() == 59);
        } else if (const auto variation = bladeVariations.find(weapon.base_name); variation != bladeVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second.second);
            CHECK(weapon.moveset->motionCategory == variation->second.first);
            CHECK(weapon.moveset->moves.size() == (variation->second.first == 29 ? 63 : 59));
        } else if (const auto variation = colossalVariations.find(weapon.base_name); variation != colossalVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second.second);
            CHECK(weapon.moveset->motionCategory == variation->second.first);
            CHECK(weapon.moveset->moves.size() == 56);
        } else if (const auto variation = poleVariations.find(weapon.base_name); variation != poleVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second.second);
            CHECK(weapon.moveset->motionCategory == variation->second.first);
            CHECK(weapon.moveset->moves.size() == (variation->second.first == 38 ? 59 : 57));
        } else if (const auto variation = spearVariations.find(weapon.base_name); variation != spearVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 36);
            CHECK(weapon.moveset->moves.size() == 60);
        } else if (const auto variation = greatSpearVariations.find(weapon.base_name); variation != greatSpearVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 37);
            CHECK(weapon.moveset->moves.size() == 57);
        } else if (const auto variation = whipVariations.find(weapon.base_name); variation != whipVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 43);
            CHECK(weapon.moveset->moves.size() == 51);
        } else if (thrustingNames.contains(weapon.base_name)) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == 500);
            CHECK(weapon.moveset->motionCategory == 27);
            CHECK(weapon.moveset->moves.size() == 71);
        } else if (const auto variation = curvedSwordVariations.find(weapon.base_name); variation != curvedSwordVariations.end()) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == variation->second);
            CHECK(weapon.moveset->motionCategory == 28);
            CHECK(weapon.moveset->moves.size() == 70);
        } else if (greatKatanaNames.contains(weapon.base_name)) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == 6600);
            CHECK(weapon.moveset->motionCategory == 61);
            CHECK(weapon.moveset->moves.size() == 63);
        } else if (lightGreatswordNames.contains(weapon.base_name)) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == 6700);
            CHECK(weapon.moveset->motionCategory == 60);
            CHECK(weapon.moveset->moves.size() == 61);
        } else if (thrustingShieldNames.contains(weapon.base_name)) {
            REQUIRE(weapon.moveset);
            CHECK(weapon.moveset->behaviorVariation == 6100);
            CHECK(weapon.moveset->motionCategory == 57);
            CHECK(weapon.moveset->moves.size() == 52);
        } else {
            UNSCOPED_INFO("Unvalidated weapons must not inherit a supported weapon's catalog");
            REQUIRE((!weapon.moveset));
        }
        if (weapon.moveset)
            supportedData.weapons.push_back(weapon);
    }
    UNSCOPED_INFO("Base behavior groups retain sharing alongside seven exact variations");
    REQUIRE((longswords == 56 && daggers == 41 && sharedLongsword != sharedDagger));
    REQUIRE(supportedData.weapons.size() == 1080);
    const auto roundTrip = erdo::json::read<erdo::calculator::GameData>(erdo::json::write(supportedData));
    std::map<std::string, std::shared_ptr<erdo::calculator::Moveset>> restoredSets;
    for (const auto& weapon : roundTrip.weapons) {
        auto& restored = restoredSets[weapon.base_name];
        if (restored) {
            UNSCOPED_INFO("Real-data round-trip must retain sharing across affinities");
            REQUIRE((restored == weapon.moveset));
        }
        restored = weapon.moveset;
    }
    UNSCOPED_INFO("Distinct weapon movesets must remain distinct through JSON");
    REQUIRE((restoredSets.size() == 156 && restoredSets.at("Dagger") != restoredSets.at("Longsword")));
    for (const auto& name : daggerNames) CHECK(restoredSets.at(name) == restoredSets.at("Dagger"));
    for (const auto& name : swordNames) CHECK(restoredSets.at(name) == restoredSets.at("Longsword"));
    std::set<const erdo::calculator::Moveset*> distinctSets{
        restoredSets.at("Dagger").get(), restoredSets.at("Longsword").get()};
    for (const auto& [name, variation] : uniqueVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        REQUIRE(distinctSets.insert(actual.get()).second);
        CHECK(actual->behaviorVariation == variation);
        // These variations have skill rows, but no ordinary namespace-1 rows.
        // Every ordinary hit must therefore match the independently snapshotted
        // base catalog, including attack IDs, channels, sources, and timing.
        auto normalized = *actual;
        const auto& base = variation < 200 ? sharedDagger : sharedLongsword;
        normalized.behaviorVariation = base->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*base));
    }
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> axeSets;
    for (const auto& [name, variation] : axeVariations) {
        const auto& actual = restoredSets.at(name);
        auto& shared = axeSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->behaviorVariation == variation);
        auto normalized = *actual;
        normalized.behaviorVariation = 1400;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*axe.moveset));
    }
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> hammerSets;
    for (const auto& [name, variation] : hammerVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = hammerSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->behaviorVariation == variation);
        auto normalized = *actual;
        normalized.behaviorVariation = 1100;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*hammer.moveset));
    }
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> flailSets;
    for (const auto& [name, variation] : flailVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = flailSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->behaviorVariation == variation);
        auto normalized = *actual;
        normalized.behaviorVariation = 1300;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*flail.moveset));
    }
    std::map<std::pair<int, int>, std::shared_ptr<erdo::calculator::Moveset>> heavySets;
    for (const auto& [name, context] : heavyVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = heavySets[context];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        const auto [category, variation] = context;
        CHECK(actual->behaviorVariation == variation);
        CHECK(actual->motionCategory == category);
        const auto& expected = category == 25 ? (variation == 303 ? alabaster : greatsword)
            : category == 32 ? greataxe : variation == 1201 ? beastclaw : greathammer;
        auto normalized = *actual;
        normalized.behaviorVariation = expected.moveset->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected.moveset));
    }
    CHECK(heavySets.size() == 15);
    std::map<std::pair<int, int>, std::shared_ptr<erdo::calculator::Moveset>> bladeSets;
    for (const auto& [name, context] : bladeVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = bladeSets[context];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        const auto [category, variation] = context;
        CHECK(actual->behaviorVariation == variation);
        CHECK(actual->motionCategory == category);
        const auto& expected = category == 29 ? katana : variation == 804 ? bloodhound : curvedGreatsword;
        auto normalized = *actual;
        normalized.behaviorVariation = expected.moveset->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected.moveset));
    }
    CHECK(bladeSets.size() == 9);
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> colossalSets;
    for (const auto& [name, context] : colossalVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        const auto [category, variation] = context;
        auto& shared = colossalSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->motionCategory == category);
        CHECK(actual->behaviorVariation == variation);
        const auto& expected = colossalReferences.at(category == 26 ? 400 : variation);
        auto normalized = *actual;
        normalized.behaviorVariation = expected->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected));
    }
    CHECK(colossalSets.size() == 10);
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> poleSets;
    for (const auto& [name, context] : poleVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        const auto [category, variation] = context;
        auto& shared = poleSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->motionCategory == category);
        CHECK(actual->behaviorVariation == variation);
        // Scythe variations 1902/1906 have no ordinary overrides in this data.
        const auto& expected = category == 38 ? halberd : scythe;
        auto normalized = *actual;
        normalized.behaviorVariation = expected.moveset->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected.moveset));
    }
    CHECK(poleSets.size() == 4);
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> spearSets;
    for (const auto& [name, variation] : spearVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = spearSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        CHECK(actual->motionCategory == 36);
        CHECK(actual->behaviorVariation == variation);
        const auto& expected = variation == 1615 ? spear1615 : spear;
        auto normalized = *actual;
        normalized.behaviorVariation = expected.moveset->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected.moveset));
    }
    CHECK(spearSets.size() == 3);
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> greatSpearSets;
    for (const auto& [name, variation] : greatSpearVariations) {
        const auto& actual = restoredSets.at(name);
        auto& shared = greatSpearSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        const auto& expected = variation == 1708 ? greatSpear1708 : greatSpear;
        auto normalized = *actual;
        normalized.behaviorVariation = expected.moveset->behaviorVariation;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*expected.moveset));
    }
    CHECK(greatSpearSets.size() == 3);
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> whipSets;
    for (const auto& [name, variation] : whipVariations) {
        CAPTURE(name);
        const auto& actual = restoredSets.at(name);
        auto& shared = whipSets[variation];
        if (shared) CHECK(shared == actual);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        auto normalized = *actual;
        normalized.behaviorVariation = 2000;
        CHECK(erdo::json::write(normalized) == erdo::json::write(*whip.moveset));
        for (auto id : {MoveId::BACKSTAB_WHIFF, MoveId::BACKSTAB, MoveId::RIPOSTE,
                MoveId::SMALL_PVE_BACKSTAB, MoveId::SMALL_PVE_RIPOSTE, MoveId::LARGE_PVE_RIPOSTE})
            CHECK(std::ranges::find(actual->moves, id, &erdo::calculator::Move::id) == actual->moves.end());
    }
    CHECK(whipSets.size() == 3);
    const auto& sharedThrusting = restoredSets.at("Rapier");
    CHECK(distinctSets.insert(sharedThrusting.get()).second);
    for (const auto& name : thrustingNames) {
        const auto& actual = restoredSets.at(name);
        CHECK(actual == sharedThrusting);
        CHECK(erdo::json::write(*actual) == erdo::json::write(*thrusting.moveset));
    }
    std::map<int, std::shared_ptr<erdo::calculator::Moveset>> curvedSwordSets;
    for (const auto& [name, variation] : curvedSwordVariations) {
        const auto& actual = restoredSets.at(name);
        auto& shared = curvedSwordSets[variation];
        if (shared) CHECK(actual == shared);
        else {
            shared = actual;
            CHECK(distinctSets.insert(actual.get()).second);
        }
        const auto& expected = variation == 703 ? curvedSword703 : curvedSword;
        CHECK(erdo::json::write(*actual) == erdo::json::write(*expected.moveset));
        CHECK(std::ranges::find(actual->moves, MoveId::SHIELDPOKE, &erdo::calculator::Move::id) == actual->moves.end());
    }
    CHECK(curvedSwordSets.size() == 2);
    CHECK(curvedSwordSets.at(700) != curvedSwordSets.at(703));
    const auto& sharedGreatKatana = restoredSets.at("Great Katana");
    CHECK(distinctSets.insert(sharedGreatKatana.get()).second);
    for (const auto& name : greatKatanaNames) {
        const auto& actual = restoredSets.at(name);
        CHECK(actual == sharedGreatKatana);
        CHECK(erdo::json::write(*actual) == erdo::json::write(*greatKatana.moveset));
    }
    const auto& sharedLightGreatsword = restoredSets.at("Milady");
    CHECK(distinctSets.insert(sharedLightGreatsword.get()).second);
    for (const auto& name : lightGreatswordNames) {
        const auto& actual = restoredSets.at(name);
        CHECK(actual == sharedLightGreatsword);
        CHECK(erdo::json::write(*actual) == erdo::json::write(*lightGreatsword.moveset));
    }
    const auto& sharedThrustingShield = restoredSets.at("Dueling Shield");
    CHECK(distinctSets.insert(sharedThrustingShield.get()).second);
    for (const auto& name : thrustingShieldNames) {
        const auto& actual = restoredSets.at(name);
        CHECK(actual == sharedThrustingShield);
        CHECK(erdo::json::write(*actual) == erdo::json::write(*thrustingShield.moveset));
    }
    CHECK(distinctSets.size() == 75);
    const auto crit = std::ranges::find(restoredSets.at("Dagger")->moves,
        MoveId::LARGE_PVE_RIPOSTE, &erdo::calculator::Move::id);
    UNSCOPED_INFO("Real-data JSON must preserve Dagger's aliased critical attack");
    REQUIRE((crit != restoredSets.at("Dagger")->moves.end() && crit->hits[0].event.sourceMotionCategory == 23 &&
            crit->hits[0].atkParamId == 100560));
    std::println("Game-data integration: {} weapon/affinity variants share {} movesets",
        supportedData.weapons.size(), distinctSets.size());
    // Exercise evaluation with actual imported data and the existing weapon AR
    // calculation, including every affinity, stance, source, and alias.
    const auto stats = erdo::calculator::get_character_starting_class_attributes("Wretch");
    for (const auto& weapon : supportedData.weapons) {
        CAPTURE(weapon.full_name);
        const erdo::calculator::MoveEvaluator evaluator(weapon, stats, {0, 25, 10}, &weapon,
            erdo::calculator::move_context(erdo::calculator::MoveEnvironment::ORDINARY_COMBAT));
        const auto evaluated = evaluator.evaluate_all();
        REQUIRE(evaluated.size() == weapon.moveset->moves.size());
        for (const auto& move : evaluated) {
            CAPTURE(std::to_underlying(move.id));
            if (!move.result)
                FAIL(move.result.error());
            CHECK(move.result->id == move.id);
            CHECK(move.result->hits.size() == erdo::calculator::find_move(weapon, move.id)->hits.size());
        }
        if (weapon.moveset->motionCategory == 60 || weapon.moveset->motionCategory == 35) {
            std::size_t inactive = 0, active = 0;
            for (const auto& move : evaluated) {
                for (const auto& hit : move.result->hits) {
                    if (hit.active) {
                        ++active;
                        CHECK(hit.hit.opposeTarget == true);
                    } else {
                        ++inactive;
                        CHECK(hit.hit.opposeTarget == false);
                        CHECK(hit.powers.attack_powers == erdo::calculator::AttackPowers{});
                        CHECK(hit.powers.total_attack_power == erdo::calculator::AttackPower{});
                        if (weapon.moveset->motionCategory == 60) {
                            CHECK((hit.hit.atkParamId == 6700992 || hit.hit.atkParamId == 6700997 ||
                                hit.hit.atkParamId == 6700998 || hit.hit.atkParamId == 6700999));
                            CHECK(hit.hit.motionValues.values[0] == 100);
                        }
                    }
                }
            }
            CHECK(inactive == (weapon.moveset->motionCategory == 60 ? 26 : 10));
            CHECK(active == (weapon.moveset->motionCategory == 60 ? 127 : 71));
        }
        if (weapon.moveset->motionCategory == 37) {
            using namespace erdo::calculator;
            const MoveEvaluator arena(weapon, stats, {0, 25, 10}, &weapon,
                move_context(MoveEnvironment::RYKARD_ARENA_EFFECT));
            const auto enabled = arena.evaluate_all();
            REQUIRE(enabled.size() == 57);
            std::size_t inactive = 0, active = 0;
            for (std::size_t i = 0; i < enabled.size(); ++i) {
                REQUIRE(enabled[i].result);
                const auto& normal = *evaluated[i].result;
                const auto& special = *enabled[i].result;
                REQUIRE(normal.hits.size() == special.hits.size());
                for (std::size_t hit = 0; hit < normal.hits.size(); ++hit) {
                    REQUIRE(special.hits[hit].active);
                    ++active;
                    if (!normal.hits[hit].active) {
                        ++inactive;
                        CHECK(normal.hits[hit].hit.event.stateInfo == 187);
                        CHECK(normal.hits[hit].powers.total_attack_power == AttackPower{});
                        CHECK(normal.hits[hit].powers.attack_powers == AttackPowers{});
                    }
                }
            }
            CHECK(inactive == 42);
            CHECK(active == 115);
        }
    }
}

TEST_CASE("moveset - great spear conditional events", "[moveset][game_xml]")
{
    using namespace erdo::calculator;
    const auto root = std::filesystem::path(ERDO_MOVESET_XML_ROOT);
    ParamTable attacks, behaviors;
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    const std::array catalog{
        MoveDefinition{MoveId::ONE_HAND_R1_1, "Light 1", 30000},
        MoveDefinition{MoveId::ONE_HAND_R1_2, "Light 2", 30010},
        MoveDefinition{MoveId::SHIELDPOKE, "Normal guard", 30900}};
    ParamTable effects;
    effects.load((root / SpEffectParamFile).string().c_str());
    std::vector<int> setters;
    for (const auto& [id, row] : effects.rows)
        if (effects.integer(row, "stateInfo") == 187) setters.push_back(id);
    CHECK(setters == std::vector<int>{1908});
    // Independent raw a37/AtkParam/BehaviorParam expectations, including the
    // native condition setter used by the named calculator environments.
    const std::array judges{0, 10, 170};
    const std::array physical{100.f, 102.f, 85.f};
    const std::array conditionalPhysical{200.f, 300.f, 300.f};
    const std::array starts{.6, .7, .53333336};
    const std::array ends{.7, .8333333, .6333333};
    AttackRating rating{};
    for (auto& channel : rating.attack_powers)
        channel = {100, 200};
    for (const int variation : {1700, 1708, 1711}) {
        CAPTURE(variation);
        const auto imported = importMoveset(cache, 37, variation, catalog, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(imported.report.failures.empty());
        const auto restored = erdo::json::read<Moveset>(erdo::json::write(*imported.moveset));
        REQUIRE(restored.moves.size() == 3);
        for (std::size_t i = 0; i < restored.moves.size(); ++i) {
            const auto& move = restored.moves[i];
            REQUIRE(move.hits.size() == 2);
            const auto& gated = move.hits[0];
            const auto& ordinary = move.hits[1];
            CHECK(gated.event.stateInfo == 187);
            CHECK(ordinary.event.stateInfo == 0);
            CHECK(gated.event.behaviorJudgeId == (i == 0 ? 5000 : 5001));
            CHECK(gated.behaviorParamId == (i == 0 ? 500000000 : 500000001));
            CHECK(gated.atkParamId == (i == 0 ? 1703001 : 1703002));
            const int effectiveVariation = variation == 1708 ? 1708 : 1700;
            CHECK(ordinary.event.behaviorJudgeId == judges[i]);
            CHECK(ordinary.behaviorParamId == 100000000 + effectiveVariation * 1000 + judges[i]);
            CHECK(ordinary.atkParamId == effectiveVariation * 1000 + judges[i]);
            for (const auto* hit : {&gated, &ordinary}) {
                CHECK(hit->event.source == 0);
                CHECK(hit->event.attackIndex == (hit == &gated ? 2 : 1));
                CHECK(hit->event.animationId == catalog[i].animationId);
                CHECK(hit->event.sourceMotionCategory == 37);
                CHECK(hit->event.sourceAnimationId == catalog[i].animationId);
                CHECK(hit->event.startTime == starts[i]);
                CHECK(hit->event.endTime == ends[i]);
                REQUIRE(std::ranges::all_of(hit->motionValues.known, [](bool v) { return v; }));
                for (std::size_t channel = 0; channel < hit->motionValues.values.size(); ++channel) {
                    const float expected = channel >= 5 ? 100.f : hit == &ordinary ? physical[i]
                        : channel == 0 ? conditionalPhysical[i] : 100.f;
                    CHECK(hit->motionValues.values[channel] == expected);
                }
            }
            const auto unknown = calculate_move(move, rating);
            REQUIRE_FALSE(unknown);
            CHECK_THAT(unknown.error(), Catch::Matchers::ContainsSubstring("condition 187"));
            for (const bool active : {false, true}) {
                const auto evaluated = calculate_move(move, rating, nullptr, {{{187, active}}});
                REQUIRE(evaluated);
                REQUIRE(evaluated->hits.size() == 2);
                CHECK(evaluated->hits[0].active == active);
                CHECK(evaluated->hits[1].active);
                CHECK(std::abs(evaluated->total.attack_powers[0][1] -
                    2. * (physical[i] + (active ? conditionalPhysical[i] : 0.))) < 1e-6);
                CHECK(evaluated->total.attack_powers[7][1] == (active ? 400 : 200));
            }
        }
    }
}

#endif
