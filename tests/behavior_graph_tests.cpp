#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <QTemporaryDir>

import std;
import erdo;

using namespace erdo::witchy;
using namespace erdo::calculator;

namespace {
// Deliberately unrelated object/type IDs and a selector name unrelated to the
// state. Discovery must follow types/references, not names from one game export.
const std::string fixture = R"(<hktagfile version="3">
 <type id="stateType"><name value="hkbStateMachine::StateInfo"/></type>
 <type id="selectorType"><name value="CustomManualSelectorGenerator"/></type>
 <type id="clipType"><name value="hkbClipGenerator"/></type>
 <object id="state" typeid="stateType"><record>
  <field name="name"><string value="AttackRightLight1"/></field>
  <field name="enable"><bool value="true"/></field>
  <field name="variableBindingSet"><pointer id="object0"/></field>
  <field name="generator"><pointer id="selector"/></field>
 </record></object>
 <object id="selector" typeid="selectorType"><record>
  <field name="name"><string value="ArbitrarySelectorName"/></field>
  <field name="variableBindingSet"><pointer id="object0"/></field>
  <field name="enableTae"><bool value="true"/></field>
  <field name="offsetType"><integer value="13"/></field>
  <field name="animId"><integer value="77"/></field>
  <field name="generators"><array count="2"><pointer id="base"/><pointer id="special"/></array></field>
 </record></object>
 <object id="base" typeid="clipType"><record>
  <field name="variableBindingSet"><pointer id="object0"/></field>
  <field name="animationName"><string value="a023_000077"/></field>
 </record></object>
 <object id="special" typeid="clipType"><record>
  <field name="variableBindingSet"><pointer id="object0"/></field>
  <field name="animationName"><string value="a110_000077"/></field>
 </record></object>
</hktagfile>)";

std::string changed(std::string_view before, std::string_view after)
{
    auto xml = fixture;
    const auto pos = xml.find(before);
    REQUIRE(pos != std::string::npos);
    xml.replace(pos, before.size(), after);
    return xml;
}

void replaceText(std::string& text, std::string_view before, std::string_view after)
{
    for (std::size_t pos = 0; (pos = text.find(before, pos)) != std::string::npos; pos += after.size())
        text.replace(pos, before.size(), after);
}

std::string selectorFixture(bool bound)
{
    auto xml = changed("id=\"selector\"/>", "id=\"chooser\"/>");
    xml.resize(xml.rfind("</hktagfile>"));
    auto alternate = fixture.substr(fixture.find(" <object id=\"selector\""));
    alternate.resize(alternate.rfind("</hktagfile>"));
    for (const auto* id : {"selector", "base", "special"})
        replaceText(alternate, std::format("id=\"{}\"", id), std::format("id=\"{}Alt\"", id));
    replaceText(alternate, "000077", "000088");
    replaceText(alternate, "value=\"77\"", "value=\"88\"");
    xml += alternate;
    xml += R"(
 <type id="manualType"><name value="hkbManualSelectorGenerator"/></type>
 <type id="bindingType"><name value="hkbVariableBindingSet"/></type>
 <type id="graphType"><name value="hkbBehaviorGraph"/></type>
 <type id="dataType"><name value="hkbBehaviorGraphData"/></type>
 <type id="stringsType"><name value="hkbBehaviorGraphStringData"/></type>
 <object id="chooser" typeid="manualType"><record>
  <field name="variableBindingSet"><pointer id="BINDING"/></field>
  <field name="selectedGeneratorIndex"><integer value="1"/></field>
  <field name="indexSelector"><pointer id="object0"/></field>
  <field name="generatorChangedTransitionEffect"><pointer id="object0"/></field>
  <field name="generators"><array count="2"><pointer id="selector"/><pointer id="selectorAlt"/></array></field>
 </record></object>
 <object id="binding" typeid="bindingType"><record>
  <field name="indexOfBindingToEnable"><integer value="-1"/></field>
  <field name="bindings"><array count="1"><record>
   <field name="memberPath"><string value="selectedGeneratorIndex"/></field>
   <field name="variableIndex"><integer value="1"/></field>
   <field name="bitIndex"><integer value="-1"/></field>
   <field name="bindingType"><integer value="0"/></field>
  </record></array></field>
 </record></object>
 <object id="graph" typeid="graphType"><record><field name="data"><pointer id="data"/></field></record></object>
 <object id="data" typeid="dataType"><record><field name="stringData"><pointer id="strings"/></field></record></object>
 <object id="strings" typeid="stringsType"><record>
  <field name="variableNames"><array count="2"><string value="Unrelated"/><string value="AttackVariant"/></array></field>
 </record></object>
</hktagfile>)";
    replaceText(xml, "BINDING", bound ? "binding" : "object0");
    return xml;
}

const std::array jumpingIds{MoveId::ONE_HAND_JUMPING_R1, MoveId::ONE_HAND_JUMPING_R2,
    MoveId::POWERSTANCE_JUMPING_L1, MoveId::TWO_HAND_JUMPING_R1, MoveId::TWO_HAND_JUMPING_R2};

std::string throwFixture()
{
    auto xml = selectorFixture(true);
    replaceText(xml, "AttackRightLight1", "ThrowAtk");
    replaceText(xml, "AttackVariant", "ThrowID");
    replaceText(xml, "hkbManualSelectorGenerator", "hkbBlenderGenerator");
    replaceText(xml, "value=\"selectedGeneratorIndex\"", "value=\"blendParameter\"");
    replaceText(xml, "value=\"13\"", "value=\"16\"");
    replaceText(xml, "id=\"chooser\" typeid=\"manualType\"><record>", R"(id="chooser" typeid="manualType"><record>
      <field name="flags"><integer value="16"/></field>
      <field name="indexOfSyncMasterChild"><integer value="-1"/></field>
      <field name="subtractLastChild"><bool value="false"/></field>
      <field name="referencePoseWeightThreshold"><real dec="0"/></field>
      <field name="children"><array count="2"><pointer id="childA"/><pointer id="childB"/></array></field>)");
    xml.resize(xml.rfind("</hktagfile>"));
    xml += R"(<type id="childType"><name value="hkbBlenderGeneratorChild"/></type>)";
    for (const auto [id, weight, generator] : {std::tuple{"childA", 77, "selector"}, {"childB", 88, "selectorAlt"}})
        xml += std::format(R"(<object id="{}" typeid="childType"><record>
          <field name="variableBindingSet"><pointer id="object0"/></field>
          <field name="generator"><pointer id="{}"/></field>
          <field name="boneWeights"><pointer id="object0"/></field>
          <field name="weight"><real dec="{}"/></field>
          <field name="worldFromModelWeight"><real dec="1"/></field>
          </record></object>)", id, generator, weight);
    return xml + "</hktagfile>";
}

std::string whiffFixture()
{
    auto xml = changed("AttackRightLight1", "ThrowBackStab");
    replaceText(xml, "value=\"13\"", "value=\"16\"");
    replaceText(xml, "name=\"generator\"><pointer id=\"selector\"", "name=\"generator\"><pointer id=\"script\"");
    xml.resize(xml.rfind("</hktagfile>"));
    xml += R"xml(<type id="scriptType"><name value="hkbScriptGenerator"/></type>
      <object id="script" typeid="scriptType"><record>
      <field name="variableBindingSet"><pointer id="object0"/></field>
      <field name="child"><pointer id="selector"/></field>
      <field name="onActivateScript"><string value="ThrowBackStab_Activate()"/></field>
      <field name="onPreUpdateScript"><string value=""/></field>
      <field name="onGenerateScript"><string value=""/></field>
      <field name="onHandleEventScript"><string value=""/></field>
      <field name="onDeactivateScript"><string value=""/></field>
      </record></object></hktagfile>)xml";
    return xml;
}

std::string jumpingFixture()
{
    auto xml = selectorFixture(true);
    replaceText(xml, "AttackRightLight1", "JumpAttack_Start_Falling");
    replaceText(xml, "<pointer id=\"selector\"/><pointer id=\"selectorAlt\"/>",
        "<pointer id=\"request0\"/><pointer id=\"request1\"/>");
    replaceText(xml, "<array count=\"2\"><string value=\"Unrelated\"/><string value=\"AttackVariant\"/></array>",
        "<array count=\"3\"><string value=\"JumpAttackFormRequest\"/><string value=\"JumpAttack_HandCondition\"/><string value=\"JumpAttack_Land\"/></array>");
    xml.resize(xml.rfind("</hktagfile>"));
    auto addSelector = [&](std::string_view id, int variable, std::span<const std::string> children, bool transition) {
        xml += std::format(R"(<object id="{0}" typeid="manualType"><record>
          <field name="variableBindingSet"><pointer id="{0}Binding"/></field>
          <field name="selectedGeneratorIndex"><integer value="99"/></field>
          <field name="indexSelector"><pointer id="object0"/></field>
          <field name="generatorChangedTransitionEffect"><pointer id="{1}"/></field>
          <field name="generators"><array count="{2}">)", id, transition ? "instant" : "object0", children.size());
        for (const auto& child : children) xml += std::format(R"(<pointer id="{}"/>)", child);
        xml += std::format(R"(</array></field></record></object>
          <object id="{0}Binding" typeid="bindingType"><record>
          <field name="indexOfBindingToEnable"><integer value="-1"/></field>
          <field name="bindings"><array count="1"><record>
          <field name="memberPath"><string value="selectedGeneratorIndex"/></field>
          <field name="variableIndex"><integer value="{1}"/></field>
          <field name="bitIndex"><integer value="-1"/></field>
          <field name="bindingType"><integer value="0"/></field>
          </record></array></field></record></object>)", id, variable);
    };
    addSelector("request0", 0, std::array{std::string("land0"), std::string("land1"), std::string("land2")}, false);
    addSelector("request1", 0, std::array{std::string("land3"), std::string("land4")}, false);
    for (int i = 0; i < 5; ++i) {
        auto leaf = fixture.substr(fixture.find(" <object id=\"selector\""));
        leaf.resize(leaf.rfind("</hktagfile>"));
        for (const auto* id : {"selector", "base", "special"})
            replaceText(leaf, std::format("id=\"{}\"", id), std::format("id=\"{}Jump{}\"", id, i));
        replaceText(leaf, "000077", std::format("{:06}", 77 + i));
        replaceText(leaf, "value=\"77\"", std::format("value=\"{}\"", 77 + i));
        if (i >= 3) replaceText(leaf, "value=\"13\"", "value=\"16\"");
        xml += leaf;
        addSelector(std::format("land{}", i), 2,
            std::array{std::format("selectorJump{}", i), std::string("landing")}, true);
    }
    auto landing = fixture.substr(fixture.find(" <object id=\"selector\""));
    landing.resize(landing.rfind("</hktagfile>"));
    for (const auto* id : {"selector", "base", "special"})
        replaceText(landing, std::format("id=\"{}\"", id), std::format("id=\"{}Landing\"", id));
    replaceText(landing, "selectorLanding", "landing");
    const auto clipsStart = landing.find(" <object id=\"baseLanding\"");
    xml += landing.substr(0, clipsStart);
    auto landingClips = landing.substr(clipsStart);
    replaceText(landingClips, "name=\"variableBindingSet\"><pointer id=\"object0\"", "name=\"variableBindingSet\"><pointer id=\"landingTimeBinding\"");
    xml += landingClips;
    xml += R"(<type id="instantType"><name value="CustomTransitionEffect"/></type>
      <object id="instant" typeid="instantType"><record>
      <field name="variableBindingSet"><pointer id="object0"/></field>
      <field name="duration"><real dec="0"/></field>
      <field name="toGeneratorStartTimeFraction"><real dec="0"/></field>
      <field name="selfTransitionMode"><integer value="0"/></field>
      <field name="eventMode"><integer value="0"/></field>
      <field name="flags"><integer value="0"/></field>
      <field name="endMode"><integer value="0"/></field>
      <field name="blendCurve"><integer value="0"/></field>
      <field name="alignmentBone"><integer value="-1"/></field>
      </record></object></hktagfile>)";
    return xml;
}
}

TEST_CASE("behavior graph - typed references and explicit priority", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(fixture);
    const auto selector = graph.inspect("AttackRightLight1");
    REQUIRE(selector.has_value());
    CHECK(selector->animationId == 77);
    REQUIRE(selector->clips.size() == 2);
    MoveDefinition seed{MoveId::ONE_HAND_R1_1, "Light", -1, 1, 99, 2};
    const auto special = graph.resolve(seed, "AttackRightLight1", 13, std::array{110, 23});
    REQUIRE(special.has_value());
    CHECK(special->animationId == 77);
    CHECK(special->motionCategory == 110);
    CHECK(special->id == seed.id);
    CHECK(special->requiredCategory == seed.requiredCategory);
    CHECK(special->requiredBehaviorParamId == seed.requiredBehaviorParamId);
    CHECK(special->eventJudgeId == seed.eventJudgeId);
    REQUIRE(graph.resolveMove(seed, std::array{110, 23}).has_value());
    auto unsupported = seed;
    unsupported.id = MoveId::MOUNTED_R1_1;
    REQUIRE_FALSE(graph.resolveMove(unsupported, std::array{23}));
    const auto base = graph.resolve(seed, "AttackRightLight1", 13, std::array{111, 23});
    REQUIRE(base.has_value());
    CHECK(base->motionCategory == 23);
    REQUIRE_FALSE(graph.resolve(seed, "AttackRightLight1", 16, std::array{23}));
    REQUIRE_FALSE(graph.resolve(seed, "AttackRightLight1", 13, std::array{20}));
    REQUIRE_FALSE(graph.resolve(seed, "AttackRightLight1", 13, std::span<const int>{}));
    REQUIRE_FALSE(graph.inspect("Missing"));
    // A failed reload must not leave dangling indices into the previous document.
    REQUIRE_THROWS(graph.loadXml("bad XML"));
    REQUIRE_FALSE(graph.inspect("AttackRightLight1"));
}

TEST_CASE("behavior graph - unsupported paths fail explicitly", "[moveset][unit][behavior_graph]")
{
    std::string xml;
    SECTION("Missing reference") { xml = changed("id=\"base\"/>", "id=\"missing\"/>"); }
    SECTION("Selector cycle") { xml = changed("id=\"base\"/>", "id=\"selector\"/>"); }
    SECTION("Unimplemented generator") { xml = changed("CustomManualSelectorGenerator", "hkbBlenderGenerator"); }
    SECTION("Variable binding") { xml = changed("id=\"object0\"/>", "id=\"binding\"/>"); }
    SECTION("Disabled state") { xml = changed("value=\"true\"", "value=\"false\""); }
    SECTION("Invalid animation") { xml = changed("a110_000077", "a110_000078"); }
    SECTION("Invalid number") { xml = changed("value=\"77\"", "value=\"77junk\""); }
    SECTION("Array count") { xml = changed("count=\"2\"", "count=\"3\""); }
    BehaviorGraph graph;
    graph.loadXml(xml);
    REQUIRE_FALSE(graph.inspect("AttackRightLight1"));
}

TEST_CASE("behavior graph - script light chain limits", "[moveset][unit][behavior_graph]")
{
    using enum MoveId;
    // Cases from the decompiled local script, including DLC exceptions whose
    // single-weapon limits differ by stance, and independent powerstance limits.
    struct Example { int category, special, one, two, paired; };
    for (const auto& e : std::array{
        Example{20, 0, 6, 6, 4}, Example{23, 0, 5, 5, 4},
        Example{25, 0, 4, 4, 3}, Example{26, 0, 3, 3, 3},
        Example{38, 0, 4, 4, 3}, Example{50, 0, 4, 4, 3},
        Example{34, 0, 4, 4, 4}, Example{42, 0, 6, 6, 6},
        Example{53, 0, 6, 6, 6}, Example{55, 0, 6, 6, 6},
        Example{56, 0, 4, 4, 4}, Example{58, 0, 6, 6, 6},
        Example{60, 0, 5, 5, 4}, Example{61, 0, 4, 4, 3},
        Example{62, 0, 4, 4, 4}, Example{60, 249, 5, 4, 4},
        Example{62, 255, 6, 4, 4}, Example{61, 257, 6, 4, 3},
        Example{61, 258, 5, 4, 3}}) {
        CAPTURE(e.category, e.special);
        for (const auto& [first, count] : std::array{
            std::pair{ONE_HAND_R1_1, e.one}, std::pair{TWO_HAND_R1_1, e.two},
            std::pair{OFFHAND_R1_1, e.one},
            std::pair{POWERSTANCE_L1_1, e.paired}}) {
            auto chain = lightAttackChain(e.category, e.special, first);
            REQUIRE(chain.has_value());
            CHECK(chain->size() == count);
            CHECK(chain->front().id == first);
            for (const auto& definition : *chain) {
                CHECK(definition.animationId == -1);
                CHECK_FALSE(definition.motionCategory.has_value());
                REQUIRE(directAttackState(definition.id).has_value());
            }
        }
    }
    for (int category : {0, 3, 41, 44, 47, 48, 49, 52, 999})
        CHECK_FALSE(lightAttackChain(category, 0, ONE_HAND_R1_1));
    CHECK_FALSE(lightAttackChain(23, -1, ONE_HAND_R1_1));
    CHECK_FALSE(lightAttackChain(23, 0, ONE_HAND_R1_2));
    CHECK_FALSE(lightAttackChain(23, 0, ONE_HAND_R2_1));
    CHECK_FALSE(lightAttackChain(21, 0, POWERSTANCE_L1_1));
    CHECK_FALSE(lightAttackChain(57, 0, POWERSTANCE_L1_1));
}

TEST_CASE("behavior graph - heavy effects follow HKS precedence and hand", "[moveset][unit][behavior_graph]")
{
    using enum MoveId;
    for (auto id : {ONE_HAND_R2_1, ONE_HAND_R2_2, ONE_HAND_CHARGED_R2_1, ONE_HAND_CHARGED_R2_2,
                   TWO_HAND_R2_1, TWO_HAND_R2_2, TWO_HAND_CHARGED_R2_1, TWO_HAND_CHARGED_R2_2}) {
        const auto route = heavyAttackRoute(id, {});
        const auto ordinary = directAttackState(id);
        REQUIRE(route.has_value());
        REQUIRE(ordinary.has_value());
        CHECK(route->stateName == ordinary->first);
        CHECK(route->offsetType == ordinary->second);
        CHECK(route->variables.empty());
    }
    struct Example { MoveId id; bool left; std::vector<int> effects; std::string_view state; std::optional<int> variant; };
    for (const auto& e : std::vector<Example>{
        {ONE_HAND_CHARGED_R2_1, false, {1681}, "AttackRightHeavySpecial1Start", 0},
        {ONE_HAND_R2_1, false, {1811}, "AttackRightHeavySpecial1End", 1},
        {ONE_HAND_R2_2, false, {1686}, "AttackRightHeavySpecial2End", 0},
        {ONE_HAND_CHARGED_R2_2, false, {1816}, "AttackRightHeavySpecial2Start", 1},
        {ONE_HAND_R2_1, false, {1681, 1811}, "AttackRightHeavySpecial1End", 1},
        {ONE_HAND_R2_1, false, {1811, 1716}, "AttackRightHeavySpecial1End", 0},
        {ONE_HAND_R2_1, false, {1811, 1721}, "AttackRightHeavySpecial1End", 0},
        {ONE_HAND_R2_1, false, {1811, 102101}, "AttackRightHeavySpecial1End", 0},
        {ONE_HAND_R2_1, false, {19912}, "AttackRightHeavy2End", {}},
        {ONE_HAND_R2_1, false, {1716, 19912}, "AttackRightHeavy2End", {}},
        {ONE_HAND_R2_1, false, {1811, 19912}, "AttackRightHeavySpecial1End", 1},
        {ONE_HAND_R2_1, false, {1861}, "AttackRightHeavy1End", {}},
        {TWO_HAND_R2_1, false, {1861}, "AttackBothHeavySpecial1End", 1},
        {TWO_HAND_R2_1, false, {1813}, "AttackBothHeavy1End", {}},
        {TWO_HAND_R2_1, true, {1811}, "AttackBothHeavy1End", {}},
        {TWO_HAND_CHARGED_R2_2, true, {1813}, "AttackBothHeavySpecial2Start", 1},
        {TWO_HAND_R2_1, true, {1683}, "AttackBothHeavySpecial1End", 0},
        {TWO_HAND_R2_1, true, {1688}, "AttackBothHeavySpecial1End", 0},
        {TWO_HAND_R2_1, true, {1818}, "AttackBothHeavySpecial1End", 1},
        {TWO_HAND_R2_1, true, {1863}, "AttackBothHeavySpecial1End", 1},
        {TWO_HAND_R2_1, true, {1813, 1718}, "AttackBothHeavySpecial1End", 0},
        {TWO_HAND_R2_1, true, {1813, 1723}, "AttackBothHeavySpecial1End", 0},
        {TWO_HAND_R2_1, true, {1813, 102106}, "AttackBothHeavySpecial1End", 0},
        // The final HKS reset tests right-hand effects even in left-both style.
        {TWO_HAND_R2_1, true, {1813, 19912}, "AttackBothHeavy2End", {}},
        {TWO_HAND_R2_1, true, {1813, 1811, 19912}, "AttackBothHeavySpecial1End", 1}}) {
        CAPTURE(e.state, e.effects, e.left);
        auto route = heavyAttackRoute(e.id, e.effects, e.left);
        REQUIRE(route.has_value());
        CHECK(route->stateName == e.state);
        if (e.variant) CHECK(route->variables.at("AttackRightHeavySpecialType") == *e.variant);
        else CHECK(route->variables.empty());
    }
    CHECK_FALSE(heavyAttackRoute(ONE_HAND_R1_1, {}));
    CHECK_FALSE(heavyAttackRoute(ONE_HAND_R2_1, {}, true));
    CHECK_FALSE(heavyAttackRoute(ONE_HAND_R2_1, std::array{-1}));
    for (int effect : {173, 174, 102020})
        CHECK_FALSE(heavyAttackRoute(ONE_HAND_R2_1, std::array{effect}));
}

TEST_CASE("behavior graph - heavy effect context drives existing resolver", "[moveset][unit][behavior_graph]")
{
    auto xml = selectorFixture(true);
    replaceText(xml, "AttackRightLight1", "AttackRightHeavySpecial1Start");
    replaceText(xml, "AttackVariant", "AttackRightHeavySpecialType");
    BehaviorGraph graph;
    graph.loadXml(xml);
    const MoveDefinition seed{MoveId::ONE_HAND_CHARGED_R2_1, "Charged", -1};
    for (const auto& [effect, animation] : std::array{std::pair{1681, 77}, std::pair{1811, 88}}) {
        auto definition = graph.resolveHeavyMove(seed, std::array{110, 23}, std::array{effect});
        REQUIRE(definition.has_value());
        CHECK(definition->id == seed.id);
        CHECK(definition->animationId == animation);
        CHECK(definition->motionCategory == 110);
    }
    // Missing ordinary/special states cannot substitute another buff's route.
    CHECK_FALSE(graph.resolveHeavyMove(seed, std::array{23}, {}));
    replaceText(xml, "AttackRightHeavySpecial1Start", "UnsupportedState");
    graph.loadXml(xml);
    CHECK_FALSE(graph.resolveHeavyMove(seed, std::array{23}, std::array{1681}));
}

TEST_CASE("behavior graph - nested selectors use graph bindings", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(selectorFixture(false));
    auto fixed = graph.inspect("AttackRightLight1");
    REQUIRE(fixed.has_value());
    CHECK(fixed->animationId == 88); // Serialized index 1, not the first child.
    graph.loadXml(selectorFixture(true));
    auto missing = graph.inspect("AttackRightLight1");
    REQUIRE_FALSE(missing);
    CHECK_THAT(missing.error(), Catch::Matchers::ContainsSubstring("AttackVariant"));
    const MoveDefinition seed{MoveId::ONE_HAND_R1_1, "Light", -1, 1, 99, 2};
    for (const auto& [index, animation] : std::array{std::pair{0, 77}, std::pair{1, 88}}) {
        auto resolved = graph.resolveMove(seed, std::array{110, 23}, {{"AttackVariant", index}});
        REQUIRE(resolved.has_value());
        CHECK(resolved->animationId == animation);
        CHECK(resolved->motionCategory == 110);
        CHECK(resolved->requiredCategory == 1);
        CHECK(resolved->requiredBehaviorParamId == 99);
        CHECK(resolved->eventJudgeId == 2);
    }
    // Another manual selector can wrap the bound selector without any naming
    // convention or weapon-specific branch table in the resolver.
    auto xml = selectorFixture(true);
    replaceText(xml, "id=\"chooser\"/>", "id=\"outer\"/>");
    xml.insert(xml.rfind("</hktagfile>"), R"(
      <object id="outer" typeid="manualType"><record>
       <field name="variableBindingSet"><pointer id="object0"/></field>
       <field name="selectedGeneratorIndex"><integer value="0"/></field>
       <field name="indexSelector"><pointer id="object0"/></field>
       <field name="generatorChangedTransitionEffect"><pointer id="object0"/></field>
       <field name="generators"><array count="1"><pointer id="chooser"/></array></field>
      </record></object>)");
    graph.loadXml(xml);
    auto nested = graph.inspect("AttackRightLight1", {{"AttackVariant", 0}});
    REQUIRE(nested.has_value());
    CHECK(nested->animationId == 77);
    // A reload cannot retain variable-name indices from the previous graph.
    replaceText(xml, "AttackVariant", "ReplacementVariant");
    graph.loadXml(xml);
    CHECK_FALSE(graph.inspect("AttackRightLight1", {{"AttackVariant", 0}}));
    CHECK(graph.inspect("AttackRightLight1", {{"ReplacementVariant", 0}}).has_value());
}

TEST_CASE("behavior graph - selector context must be explicit and valid", "[moveset][unit][behavior_graph]")
{
    auto xml = selectorFixture(true);
    BehaviorVariables variables{{"AttackVariant", 0}};
    SECTION("Missing value") { variables.clear(); }
    SECTION("Negative index") { variables["AttackVariant"] = -1; }
    SECTION("Index beyond children") { variables["AttackVariant"] = 2; }
    SECTION("Unsupported binding member") { replaceText(xml, "value=\"selectedGeneratorIndex\"", "value=\"enable\""); }
    SECTION("Bit binding") { replaceText(xml, "name=\"bitIndex\"><integer value=\"-1\"", "name=\"bitIndex\"><integer value=\"0\""); }
    SECTION("Property binding") { replaceText(xml, "name=\"bindingType\"><integer value=\"0\"", "name=\"bindingType\"><integer value=\"1\""); }
    SECTION("Bad variable index") { replaceText(xml, "name=\"variableIndex\"><integer value=\"1\"", "name=\"variableIndex\"><integer value=\"3\""); }
    SECTION("Duplicate variable name") { replaceText(xml, "Unrelated", "AttackVariant"); }
    SECTION("Wrong binding count") { replaceText(xml, "name=\"bindings\"><array count=\"1\"", "name=\"bindings\"><array count=\"2\""); }
    SECTION("Missing graph data") { replaceText(xml, "id=\"data\"/>", "id=\"missing\"/>"); }
    SECTION("Dynamic index selector") { replaceText(xml, "name=\"indexSelector\"><pointer id=\"object0\"", "name=\"indexSelector\"><pointer id=\"dynamic\""); }
    SECTION("Transition effect") { replaceText(xml, "name=\"generatorChangedTransitionEffect\"><pointer id=\"object0\"", "name=\"generatorChangedTransitionEffect\"><pointer id=\"effect\""); }
    SECTION("Selected missing reference") { replaceText(xml, "id=\"selector\"/>", "id=\"missing\"/>"); }
    SECTION("Cycle") { replaceText(xml, "id=\"selector\"/>", "id=\"chooser\"/>"); }
    BehaviorGraph graph;
    graph.loadXml(xml);
    CHECK_FALSE(graph.inspect("AttackRightLight1", variables));
}

TEST_CASE("behavior graph - generate bounded chains from weapon parameters", "[moveset][unit][behavior_graph]")
{
    auto replace = [](std::string& text, const std::string& before, const std::string& after) {
        for (std::size_t pos = 0; (pos = text.find(before, pos)) != std::string::npos; pos += after.size())
            text.replace(pos, before.size(), after);
    };
    const std::array animations{81, 3, 900, 123, 42, 777};
    auto xml = fixture.substr(0, fixture.find(" <object"));
    const auto objects = fixture.substr(fixture.find(" <object"), fixture.rfind("</hktagfile>") - fixture.find(" <object"));
    for (int i = 0; i < 6; ++i) {
        auto part = objects;
        replace(part, "AttackRightLight1", std::format("AttackRightLight{}", i + 1));
        for (const auto* id : {"state", "selector", "base", "special"})
            replace(part, std::format("id=\"{}\"", id), std::format("id=\"{}{}\"", id, i));
        replace(part, "000077", std::format("{:06}", animations[i]));
        replace(part, "value=\"77\"", std::format("value=\"{}\"", animations[i]));
        xml += part;
    }
    xml += "</hktagfile>";
    BehaviorGraph graph;
    graph.loadXml(xml);
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="isWeaponCatalyst" defaultValue="0"/>
      </fields><rows>
      <row id="7" wepmotionCategory="23" wepType="999"/>
      <row id="8" wepmotionCategory="62" spAtkcategory="255"/>
      <row id="9" wepmotionCategory="bad"/>
      <row id="10"/>
      <row id="11" wepmotionCategory="23" spAtkcategory="110"/>
      <row id="12" wepmotionCategory="23" spAtkcategory="111"/>
      </rows></param>)");
    const auto chain = graph.resolveLightChain(weapons, 7, MoveId::ONE_HAND_R1_1, std::array{23});
    REQUIRE(chain.has_value());
    REQUIRE(chain->size() == 5); // Sixth registered clip is not part of this chain.
    CHECK(graph.inspect("AttackRightLight6").has_value());
    for (std::size_t i = 0; i < chain->size(); ++i) {
        CHECK((*chain)[i].animationId == animations[i]);
        CHECK((*chain)[i].motionCategory == 23);
    }
    auto override = graph.resolveLightChain(weapons, 8, MoveId::ONE_HAND_R1_1, std::array{110, 23});
    REQUIRE(override.has_value());
    CHECK(override->size() == 6);
    CHECK(override->back().motionCategory == 110);
    for (const auto [weapon, expectedCategory] : {std::pair{7, 23}, {11, 110}, {12, 23}}) {
        const auto automatic = graph.resolveLightChain(weapons, weapon, MoveId::ONE_HAND_R1_1);
        REQUIRE(automatic);
        REQUIRE(automatic->size() == 5);
        for (const auto& definition : *automatic) CHECK(definition.motionCategory == expectedCategory);
    }
    CHECK_FALSE(graph.resolveLightChain(weapons, 7, MoveId::POWERSTANCE_L1_1));
    for (int id : {9, 10, 999})
        CHECK_FALSE(graph.resolveLightChain(weapons, id, MoveId::ONE_HAND_R1_1, std::array{23}));
    // No partial chain may escape if an intermediate state cannot resolve.
    replace(xml, "AttackRightLight3", "UnsupportedState");
    graph.loadXml(xml);
    const auto missing = graph.resolveLightChain(weapons, 7, MoveId::ONE_HAND_R1_1, std::array{23});
    REQUIRE_FALSE(missing);
    CHECK_THAT(missing.error(), Catch::Matchers::ContainsSubstring("1H R1 #3"));
    CHECK_FALSE(graph.resolveLightChain(weapons, 11, MoveId::ONE_HAND_R1_1));
}

TEST_CASE("behavior graph - ordinary weapon category policy", "[moveset][unit][behavior_graph]")
{
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="110"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/></fields><rows>
      <row id="1"/><row id="2" spAtkcategory="0"/><row id="3" spAtkcategory="23"/>
      <row id="4" spAtkcategory="-1"/><row id="5" spAtkcategory="1000"/>
      <row id="6" wepmotionCategory="bad"/><row id="7" wepmotionCategory="0"/>
      <row id="8" wepmotionOneHandId="1"/><row id="9" wepmotionBothHandId="11"/>
      <row id="10" isWeaponCatalyst="1"/><row id="11" wepmotionCategory="44"/>
      <row id="12" wepmotionCategory="25" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="13" wepmotionCategory="32" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="14" wepmotionCategory="35" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="15" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="16" wepmotionCategory="25" spAtkcategory="0"/>
      <row id="17" wepmotionCategory="25" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="18" wepmotionCategory="40" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="19" wepmotionCategory="40" spAtkcategory="215" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="20" wepmotionCategory="29" spAtkcategory="0"/>
      <row id="21" wepmotionCategory="29" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="22" wepmotionCategory="26" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="23" wepmotionCategory="31" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="24" wepmotionCategory="26" spAtkcategory="135" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="25" wepmotionCategory="24" spAtkcategory="0" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="26" wepmotionCategory="38" spAtkcategory="0" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="27" wepmotionCategory="50" spAtkcategory="0" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="28" wepmotionCategory="38" spAtkcategory="210" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="29" wepmotionCategory="50" spAtkcategory="225" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="30" wepmotionCategory="38" spAtkcategory="0"/>
      <row id="31" wepmotionCategory="50" spAtkcategory="0" wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="32" wepmotionCategory="36" spAtkcategory="0" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="33" wepmotionCategory="36" spAtkcategory="200" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="34" wepmotionCategory="36" spAtkcategory="0"/>
      </rows></param>)");
    CHECK(ordinaryWeaponCategories(weapons, 1, false) == std::vector{110, 23});
    CHECK(ordinaryWeaponCategories(weapons, 1, true) == std::vector{110, 23});
    CHECK(ordinaryWeaponCategories(weapons, 2, false) == std::vector{23});
    CHECK(ordinaryWeaponCategories(weapons, 3, false) == std::vector{23});
    for (int id : {4, 5, 6, 7, 10, 11, 999})
        CHECK_FALSE(ordinaryWeaponCategories(weapons, id, false));
    CHECK_FALSE(ordinaryWeaponCategories(weapons, 8, false));
    CHECK_FALSE(ordinaryWeaponCategories(weapons, 9, true));
    for (const auto [id, category] : {std::pair{12, 25}, {13, 32}, {14, 35}, {18, 40}, {20, 29}, {22, 26}, {23, 31}, {26, 38}, {27, 50}, {32, 36}}) {
        CHECK(ordinaryWeaponCategories(weapons, id, false) == std::vector{category});
        CHECK(ordinaryWeaponCategories(weapons, id, true) == std::vector{category});
    }
    for (int id : {15, 16, 17, 19, 21, 24, 25, 28, 29, 30, 31, 33, 34}) {
        CHECK_FALSE(ordinaryWeaponCategories(weapons, id, false));
        CHECK_FALSE(ordinaryWeaponCategories(weapons, id, true));
    }
    BehaviorGraph graph;
    graph.loadXml(changed("AttackRightLight1", "AttackRightHeavy1End"));
    const MoveDefinition seed{MoveId::ONE_HAND_R2_1, "Heavy", -1};
    const auto special = graph.resolveWeaponHeavyMove(seed, weapons, 1);
    REQUIRE(special);
    CHECK(special->motionCategory == 110);
    CHECK(special->animationId == 77);
    // A missing special registration may fall back; a malformed one may not.
    auto fallback = changed("AttackRightLight1", "AttackRightHeavy1End");
    replaceText(fallback, "a110_000077", "a111_000077");
    graph.loadXml(fallback);
    const auto base = graph.resolveWeaponHeavyMove(seed, weapons, 1);
    REQUIRE(base);
    CHECK(base->motionCategory == 23);
    auto invalid = changed("AttackRightLight1", "AttackRightHeavy1End");
    replaceText(invalid, "a110_000077", "a110_000078");
    graph.loadXml(invalid);
    CHECK_FALSE(graph.resolveWeaponHeavyMove(seed, weapons, 1));
    weapons.loadXml("<param><rows><row id='1' wepmotionCategory='23'/></rows></param>");
    CHECK_FALSE(ordinaryWeaponCategories(weapons, 1, false));
}

TEST_CASE("behavior graph - selected category uses existing importer", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(fixture);
    const MoveDefinition seed{MoveId::ONE_HAND_R1_1, "Light", -1, 1, 99, 2};
    const auto definition = graph.resolve(seed, "AttackRightLight1", 13, std::array{110, 23});
    REQUIRE(definition.has_value());
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const std::filesystem::path root = temp.path().toStdString();
    const auto path = root / "chr/c0000-anibnd-dcx-wanibnd/INTERROOT_win64/chr/c0000/tae/a110-tae/anim-000077.xml";
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << R"(<anim><events><event><type>1</type><startTime>0.1</startTime><endTime>0.2</endTime>
      <params><param name="BehaviorJudgeID" value="2"/><param name="Source" value="0"/></params>
      </event></events></anim>)";
    ParamTable attacks, behaviors;
    attacks.loadXml(R"(<param><rows><row id="123" atkPhysCorrection="130"/></rows></param>)");
    behaviors.loadXml(R"(<param><rows><row id="99" variationId="7" behaviorJudgeId="2" category="1" refType="0" refId="123"/></rows></param>)");
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    const auto imported = importMoveset(cache, 23, 7, std::array{*definition}, index, attacks);
    REQUIRE(imported.report.failures.empty());
    REQUIRE(imported.moveset->moves.size() == 1);
    const auto& hit = imported.moveset->moves[0].hits.at(0);
    CHECK(hit.event.sourceMotionCategory == 110);
    CHECK(hit.atkParamId == 123);
    CHECK(hit.motionValues.values[0] == 130);
    const auto restored = erdo::json::read<Moveset>(erdo::json::write(*imported.moveset));
    CHECK(restored.moves[0].hits[0].event.sourceMotionCategory == 110);

    // Once an override is selected, missing TAE data is an import failure. It
    // must not silently substitute the base clip even if that file is present.
    const auto base_path = root / "chr/c0000-anibnd-dcx-wanibnd/INTERROOT_win64/chr/c0000/tae/a23-tae/anim-000077.xml";
    std::filesystem::create_directories(base_path.parent_path());
    std::filesystem::rename(path, base_path);
    AnimationCache fresh_cache(root);
    const auto missing = importMoveset(fresh_cache, 23, 7, std::array{*definition}, index, attacks);
    CHECK(missing.report.failures.size() == 1);
    CHECK(missing.moveset->moves.empty());

    // Ambiguous registration must not choose the first matching clip.
    graph.loadXml(changed("a110_000077", "a023_000077"));
    REQUIRE_FALSE(graph.resolve(seed, "AttackRightLight1", 13, std::array{23}));
}

TEST_CASE("behavior graph - complete catalog bridge rejects hidden fallbacks", "[moveset][unit][behavior_graph]")
{
    auto xml = fixture.substr(0, fixture.find(" <object"));
    const auto objects = fixture.substr(fixture.find(" <object"), fixture.rfind("</hktagfile>") - fixture.find(" <object"));
    std::set<MoveId> covered;
    std::set<std::string_view> states;
    std::vector<MoveDefinition> allDefinitions(clawCatalog.begin(), clawCatalog.end());
    allDefinitions.insert(allDefinitions.end(), beastClawCatalog.begin(), beastClawCatalog.end());
    for (const auto& definition : allDefinitions) {
        const auto state = directAttackState(definition.id).or_else([&] { return mountedAttackState(definition.id); })
            .or_else([&] { return feintAttackState(definition.id); });
        if (!state) continue;
        covered.insert(definition.id);
        if (!states.insert(state->first).second) continue;
        auto part = objects;
        replaceText(part, "AttackRightLight1", state->first);
        replaceText(part, "value=\"13\"", std::format("value=\"{}\"", state->second));
        for (const auto* id : {"state", "selector", "base", "special"})
            replaceText(part, std::format("id=\"{}\"", id), std::format("id=\"{}{}\"", id, covered.size()));
        xml += part;
    }
    auto whiff = whiffFixture();
    whiff = whiff.substr(whiff.find('>') + 1);
    whiff.resize(whiff.rfind("</hktagfile>"));
    for (const auto* id : {"state", "selector", "base", "special"})
        replaceText(whiff, std::format("id=\"{}\"", id), std::format("id=\"{}Whiff\"", id));
    for (const auto* type : {"stateType", "selectorType", "clipType"})
        replaceText(whiff, type, std::string(type) + "Whiff");
    xml += whiff;
    covered.insert(MoveId::BACKSTAB_WHIFF);
    auto jumps = jumpingFixture();
    jumps = jumps.substr(jumps.find('>') + 1);
    jumps.resize(jumps.rfind("</hktagfile>"));
    for (const auto* type : {"stateType", "selectorType", "clipType"})
        replaceText(jumps, type, std::string(type) + "Jump");
    xml += jumps + "</hktagfile>";
    covered.insert(jumpingIds.begin(), jumpingIds.end());
    BehaviorGraph graph;
    graph.loadXml(xml);
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/><field name="behaviorVariationId" defaultValue="200"/>
      <field name="wepType" defaultValue="3"/><field name="isDualBlade" defaultValue="0"/><field name="enableThrow" defaultValue="1"/>
      </fields><rows><row id="1"/><row id="2"/><row id="3" spAtkcategory="111"/>
      <row id="4" wepmotionCategory="57" wepmotionOneHandId="3" wepmotionBothHandId="13"
        behaviorVariationId="6100" wepType="90"/>
      <row id="5" wepmotionCategory="21" behaviorVariationId="2400" wepType="87" enableThrow="0"/>
      <row id="6" wepmotionCategory="22" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="2200" wepType="37" bothHandEquipable="1"/>
      <row id="7" wepmotionCategory="42" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="2121" wepType="35" bothHandEquipable="1"/>
      <row id="8" wepmotionCategory="42" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="2121" wepType="35" bothHandEquipable="0"/>
      <row id="9" wepmotionCategory="42" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="2121" wepType="35"/>
      <row id="10" wepmotionCategory="62" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6800" wepType="95" bothHandEquipable="1"/>
      <row id="11" wepmotionCategory="62" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6800" wepType="95" bothHandEquipable="0"/>
      <row id="12" wepmotionCategory="62" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6800" wepType="95"/>
      <row id="13" wepmotionCategory="58" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6400" wepType="92" bothHandEquipable="1"/>
      <row id="14" wepmotionCategory="58" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6400" wepType="92" bothHandEquipable="0"/>
      <row id="15" wepmotionCategory="58" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6400" wepType="92"/>
      <row id="16" wepmotionCategory="55" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6300" wepType="88" bothHandEquipable="1"/>
      <row id="17" wepmotionCategory="55" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6300" wepType="88" bothHandEquipable="0"/>
      <row id="18" wepmotionCategory="55" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6300" wepType="88"/>
      <row id="19" wepmotionCategory="55" spAtkcategory="246" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6301" wepType="88" bothHandEquipable="1"/>
      <row id="20" wepmotionCategory="55" spAtkcategory="247" wepmotionBothHandId="0" isDualBlade="1" behaviorVariationId="6301" wepType="88" bothHandEquipable="1"/>
      <row id="21" spAtkcategory="110" behaviorVariationId="201"/>
      <row id="22" spAtkcategory="110" behaviorVariationId="201"/>
      </rows></param>)");
    const auto resolved = graph.resolveSupportedCatalog(weapons, std::array{1LL, 2LL}, straightSwordCatalog);
    REQUIRE(resolved.size() == straightSwordCatalog.size());
    CHECK(covered.size() == 67);
    for (std::size_t i = 0; i < resolved.size(); ++i) {
        auto expected = straightSwordCatalog[i];
        if (covered.contains(expected.id)) {
            expected.animationId = 77; // Deliberately different from every catalog animation.
            expected.motionCategory = 23;
            const auto jump = std::ranges::find(jumpingIds, expected.id);
            if (jump != jumpingIds.end()) expected.animationId += static_cast<int>(jump - jumpingIds.begin());
        }
        CHECK(erdo::json::write(resolved[i]) == erdo::json::write(expected));
    }
    auto components = std::vector<MoveDefinition>(straightSwordCatalog.begin(), straightSwordCatalog.end());
    components.front().additionalEventSelections = std::vector{AttackEventSelection{2, 102, 1}};
    const auto composedDefinitions = graph.resolveSupportedCatalog(weapons, std::array{1LL}, components);
    REQUIRE(composedDefinitions.front().additionalEventSelections);
    CHECK(erdo::json::write(*composedDefinitions.front().additionalEventSelections)
        == erdo::json::write(*components.front().additionalEventSelections));
    CHECK(composedDefinitions.front().animationId == 77);
    // Native class exclusion must work without any paired graph registration.
    // Ordinary mappings still resolve non-arithmetic clips and stay complete.
    for (const auto& [weaponId, category, catalog] : {
            std::tuple{4LL, 57, std::span<const MoveDefinition>(thrustingShieldCatalog)},
            std::tuple{5LL, 21, std::span<const MoveDefinition>(torchCatalog)}}) {
        CAPTURE(category);
        auto shieldXml = xml;
        replaceText(shieldXml, "a023_", std::format("a{:03}_", category));
        replaceText(shieldXml, "AttackDual", "ForbiddenPairedState");
        graph.loadXml(shieldXml);
        const auto shield = graph.resolveSupportedCatalog(weapons, std::array{weaponId}, catalog);
        REQUIRE(shield.size() == catalog.size());
        for (std::size_t i = 0; i < shield.size(); ++i) {
            auto expected = catalog[i];
            if (covered.contains(expected.id)) {
                expected.animationId = 77;
                expected.motionCategory = category;
                const auto jump = std::ranges::find(jumpingIds, expected.id);
                if (jump != jumpingIds.end()) expected.animationId += static_cast<int>(jump - jumpingIds.begin());
            }
            CHECK(erdo::json::write(shield[i]) == erdo::json::write(expected));
        }
        CHECK_FALSE(graph.resolveLightChain(weapons, static_cast<int>(weaponId), MoveId::POWERSTANCE_L1_1));
        for (const auto id : {MoveId::POWERSTANCE_L1_1, MoveId::POWERSTANCE_RUNNING_L1,
                MoveId::POWERSTANCE_ROLLING_L1, MoveId::POWERSTANCE_BACKSTEP_L1, MoveId::POWERSTANCE_JUMPING_L1}) {
            auto invalid = std::vector(catalog.begin(), catalog.end());
            invalid.push_back({id, "Forbidden pair", 999});
            REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{weaponId}, invalid));
        }
        auto incompleteShield = std::vector(catalog.begin(), catalog.end());
        incompleteShield.erase(incompleteShield.begin());
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{weaponId}, incompleteShield),
            Catch::Matchers::ContainsSubstring("absent from the supported catalog"));
        for (const auto state : {"AttackLeftHeavy3", "AttackRightLightDash", "RideAttack_R_Hard1_Start"}) {
            auto missingShield = shieldXml;
            replaceText(missingShield, state, "MissingShieldState");
            graph.loadXml(missingShield);
            REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{weaponId}, catalog));
        }
    }
    for (const auto& [weaponId, category, catalog] : {
            std::tuple{6LL, 22, std::span<const MoveDefinition>(clawCatalog)},
            std::tuple{7LL, 42, std::span<const MoveDefinition>(fistCatalog)},
            std::tuple{10LL, 62, std::span<const MoveDefinition>(beastClawCatalog)},
            std::tuple{13LL, 58, std::span<const MoveDefinition>(backhandBladeCatalog)},
            std::tuple{16LL, 55, std::span<const MoveDefinition>(martialArtsCatalog)}}) {
        CAPTURE(category);
        auto pairedXml = xml;
        replaceText(pairedXml, "a023_", std::format("a{:03}_", category));
        graph.loadXml(pairedXml);
        const auto pairedMoves = graph.resolveSupportedCatalog(weapons, std::array{weaponId}, catalog);
        REQUIRE(pairedMoves.size() == catalog.size());
        for (std::size_t i = 0; i < pairedMoves.size(); ++i) {
            auto expected = catalog[i];
            if (covered.contains(expected.id)) {
                expected.animationId = 77;
                expected.motionCategory = category;
                const auto jump = std::ranges::find(jumpingIds, expected.id);
                if (jump != jumpingIds.end()) expected.animationId += static_cast<int>(jump - jumpingIds.begin());
            }
            CHECK(erdo::json::write(pairedMoves[i]) == erdo::json::write(expected));
        }
        for (const auto state : {"AttackRightLight", "AttackBothLight", "AttackLeftHeavy", "AttackDualLight"}) {
            auto missingPaired = pairedXml;
            replaceText(missingPaired, std::format("{}{}", state, category == 62 ? 4 : 6), "MissingPairedState");
            graph.loadXml(missingPaired);
            REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{weaponId}, catalog));
        }
    }
    // Special110 uses registrations rather than the representative seed scope.
    // This fixture deliberately gives every covered state a special clip and
    // non-arithmetic animation ID, unlike the six native overrides.
    graph.loadXml(xml);
    const auto straightSpecial = graph.resolveSupportedCatalog(weapons, std::array{21LL, 22LL}, straightSword110Catalog);
    REQUIRE(straightSpecial.size() == straightSword110Catalog.size());
    for (std::size_t i = 0; i < straightSpecial.size(); ++i) {
        auto expected = straightSword110Catalog[i];
        if (covered.contains(expected.id)) {
            expected.animationId = 77;
            expected.motionCategory = 110;
            const auto jump = std::ranges::find(jumpingIds, expected.id);
            if (jump != jumpingIds.end()) expected.animationId += static_cast<int>(jump - jumpingIds.begin());
        }
        CHECK(erdo::json::write(straightSpecial[i]) == erdo::json::write(expected));
    }
    auto absentSpecial = xml;
    replaceText(absentSpecial, "a110_", "a109_");
    graph.loadXml(absentSpecial);
    const auto baseFallback = graph.resolveSupportedCatalog(weapons, std::array{21LL}, straightSword110Catalog);
    for (const auto& move : baseFallback)
        if (covered.contains(move.id)) CHECK(move.motionCategory == 23);
    auto invalidSpecial = xml;
    replaceText(invalidSpecial, "a110_000077", "a110_000078");
    graph.loadXml(invalidSpecial);
    REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{21LL}, straightSword110Catalog));

    // An audited partial special set selects overrides by graph registration,
    // including offhand, and retains base clips where the special is absent.
    auto specialXml = xml;
    replaceText(specialXml, "a023_", "a055_");
    replaceText(specialXml, "a110_", "a246_");
    graph.loadXml(specialXml);
    const auto specialMoves = graph.resolveSupportedCatalog(weapons, std::array{19LL}, footworkCatalog);
    REQUIRE(specialMoves.size() == footworkCatalog.size());
    for (std::size_t i = 0; i < specialMoves.size(); ++i) {
        auto expected = footworkCatalog[i];
        if (covered.contains(expected.id)) {
            expected.animationId = 77;
            expected.motionCategory = 246;
            const auto jump = std::ranges::find(jumpingIds, expected.id);
            if (jump != jumpingIds.end()) expected.animationId += static_cast<int>(jump - jumpingIds.begin());
        }
        CHECK(erdo::json::write(specialMoves[i]) == erdo::json::write(expected));
    }
    CHECK_FALSE(offhandOrMatchingPairCategories(weapons, 20, false));
    CHECK_FALSE(offhandOrMatchingPairCategories(weapons, 20, true));
    REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{20LL}, footworkCatalog));
    replaceText(specialXml, "a246_", "a245_");
    graph.loadXml(specialXml);
    const auto fallbackMoves = graph.resolveSupportedCatalog(weapons, std::array{19LL}, footworkCatalog);
    for (const auto& move : fallbackMoves)
        if (covered.contains(move.id)) CHECK(move.motionCategory == 55);

    // Equal motion/behavior fields cannot hide a missing or disabled native
    // both-hand flag on a second member of a shared paired catalog.
    for (const auto invalidId : {8LL, 9LL}) {
        REQUIRE_FALSE(ordinaryWeaponCategories(weapons, static_cast<int>(invalidId), true));
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{7LL, invalidId}, fistCatalog),
            Catch::Matchers::ContainsSubstring("both-hand availability"));
    }
    for (const auto invalidId : {11LL, 12LL}) {
        REQUIRE_FALSE(ordinaryWeaponCategories(weapons, static_cast<int>(invalidId), true));
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{10LL, invalidId}, beastClawCatalog),
            Catch::Matchers::ContainsSubstring("both-hand availability"));
    }
    for (const auto invalidId : {17LL, 18LL}) {
        REQUIRE_FALSE(ordinaryWeaponCategories(weapons, static_cast<int>(invalidId), true));
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{16LL, invalidId}, martialArtsCatalog),
            Catch::Matchers::ContainsSubstring("both-hand availability"));
    }
    for (const auto invalidId : {14LL, 15LL}) {
        REQUIRE_FALSE(ordinaryWeaponCategories(weapons, static_cast<int>(invalidId), true));
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{13LL, invalidId}, backhandBladeCatalog),
            Catch::Matchers::ContainsSubstring("both-hand availability"));
    }
    graph.loadXml(xml);
    REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL, 3LL}, straightSwordCatalog),
        Catch::Matchers::ContainsSubstring("different graph inputs"));
    REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, std::array{999LL}, straightSwordCatalog));
    REQUIRE_THROWS(graph.resolveSupportedCatalog(weapons, {}, straightSwordCatalog));
    auto incomplete = std::vector(straightSwordCatalog.begin(), straightSwordCatalog.end());
    incomplete.erase(incomplete.begin());
    REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL}, incomplete),
        Catch::Matchers::ContainsSubstring("absent from the supported catalog"));
    incomplete = std::vector(straightSwordCatalog.begin(), straightSwordCatalog.end());
    incomplete.push_back({MoveId::ONE_HAND_R1_6, "extra light", 999});
    REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL}, incomplete),
        Catch::Matchers::ContainsSubstring("disagrees with weapon/script limits"));
    for (const auto [state, label] : {std::pair{"AttackRightLightDash", "1H Running R1"},
                                    {"AttackRightHeavyCounter", "1H Guard Counter"},
                                    {"AttackLeftHeavy3", "Offhand R1 #3"},
                                    {"AttackDualLight3", "Powerstance L1 #3"},
                                    {"AttackDualDash", "Powerstance Running L1"},
                                    {"JumpAttack_Start_Falling", "1H Jumping R1"},
                                    {"RideAttack_R_Top03", "Mounted R1 #3"},
                                    {"RideAttack_L_Hard1_Start", "Mounted L2 Charging"},
                                    {"ThrowBackStab", "Backstab Whiff"}}) {
        auto missing = xml;
        replaceText(missing, state, "MissingMovementState");
        graph.loadXml(missing);
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL}, straightSwordCatalog),
            Catch::Matchers::ContainsSubstring(label));
    }
    replaceText(xml, "AttackRightLight3", "MissingLight");
    graph.loadXml(xml);
    REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL}, straightSwordCatalog),
        Catch::Matchers::ContainsSubstring("1H R1 #3"));
}

TEST_CASE("behavior graph - exact parametric samples refuse interpolation", "[moveset][unit][behavior_graph]")
{
    auto xml = throwFixture();
    BehaviorGraph graph;
    graph.loadXml(xml);
    for (const int animation : {77, 88}) {
        const auto result = graph.inspect("ThrowAtk", {{"ThrowID", animation}});
        REQUIRE(result);
        CHECK(result->animationId == animation);
    }
    CHECK_FALSE(graph.inspect("ThrowAtk"));
    for (const int value : {76, 80, 89}) CHECK_FALSE(graph.inspect("ThrowAtk", {{"ThrowID", value}}));
    SECTION("Duplicate samples") { replaceText(xml, "real dec=\"88\"", "real dec=\"77\""); }
    SECTION("Nonfinite sample") { replaceText(xml, "real dec=\"88\"", "real dec=\"nan\""); }
    SECTION("Bad count") { replaceText(xml, "name=\"children\"><array count=\"2\"", "name=\"children\"><array count=\"3\""); }
    SECTION("Cyclic blend") { replaceText(xml, "name=\"flags\"><integer value=\"16\"", "name=\"flags\"><integer value=\"48\""); }
    SECTION("Bone weights") { replaceText(xml, "name=\"boneWeights\"><pointer id=\"object0\"", "name=\"boneWeights\"><pointer id=\"weights\""); }
    SECTION("Motion weights") { replaceText(xml, "name=\"worldFromModelWeight\"><real dec=\"1\"", "name=\"worldFromModelWeight\"><real dec=\"0\""); }
    SECTION("Wrong binding") { replaceText(xml, "value=\"blendParameter\"", "value=\"flags\""); }
    SECTION("Cycle") { replaceText(xml, "name=\"generator\"><pointer id=\"selectorAlt\"", "name=\"generator\"><pointer id=\"chooser\""); }
    graph.loadXml(xml);
    CHECK_FALSE(graph.inspect("ThrowAtk", {{"ThrowID", 88}}));
}

TEST_CASE("behavior graph - explicit player throw row controls animation", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(throwFixture());
    ParamTable throws;
    throws.loadXml(R"(<param><fields><field name="AtkChrId" defaultValue="0"/>
      <field name="DefChrId" defaultValue="2010"/><field name="throwTypeId" defaultValue="0"/>
      <field name="isSkipWepCate" defaultValue="0"/><field name="atkAnimId" defaultValue="77"/></fields>
      <rows><row id="1"/><row id="2" atkAnimId="88"/><row id="3" atkAnimId="80"/>
      <row id="4" AtkChrId="2010"/><row id="5" isSkipWepCate="1"/><row id="6" throwTypeId="20"/>
      <row id="7" DefChrId="-1"/><row id="8" atkAnimId="-1"/></rows></param>)");
    const MoveDefinition seed{MoveId::RIPOSTE, "Selected target interaction", 999, 12, {}, 530};
    for (const auto [row, animation] : {std::pair{1, 77}, {2, 88}}) {
        const auto result = graph.resolvePlayerThrow(seed, throws, row, std::array{23});
        REQUIRE(result);
        auto expected = seed;
        expected.animationId = animation;
        expected.motionCategory = 23;
        expected.eventType = 304;
        CHECK(erdo::json::write(*result) == erdo::json::write(expected));
    }
    auto collision = seed;
    collision.eventType = 1;
    CHECK_FALSE(graph.resolvePlayerThrow(collision, throws, 1, std::array{23}));
    for (const int row : {-1, 3, 4, 5, 6, 7, 8, 999}) CHECK_FALSE(graph.resolvePlayerThrow(seed, throws, row, std::array{23}));
    CHECK_FALSE(graph.resolvePlayerThrow(seed, throws, 1, std::array{999}));
    auto substitute = throwFixture();
    replaceText(substitute, "real dec=\"77\"", "real dec=\"76\"");
    replaceText(substitute, "real dec=\"88\"", "real dec=\"77\"");
    graph.loadXml(substitute); // Selected sample 77 leads to animation 88: refuse fallback.
    const auto result = graph.resolvePlayerThrow(seed, throws, 1, std::array{23});
    REQUIRE_FALSE(result);
    CHECK_THAT(result.error(), Catch::Matchers::ContainsSubstring("substitute animation"));
}

TEST_CASE("behavior graph - throw query preserves all matching rows", "[moveset][unit][behavior_graph]")
{
    ParamTable throws;
    throws.loadXml(R"(<param><fields><field name="AtkChrId" defaultValue="0"/>
      <field name="DefChrId" defaultValue="2010"/><field name="throwType" defaultValue="20"/>
      <field name="throwTypeId" defaultValue="0"/><field name="AtkEnableState" defaultValue="0"/></fields>
      <rows><row id="50"/><row id="10" isSkipWepCate="1"/><row id="90" AtkChrId="2010"/>
      <row id="60" DefChrId="0"/><row id="70" throwType="1"/><row id="80" throwTypeId="10"/>
      <row id="40" AtkEnableState="1"/></rows></param>)");
    const auto rows = playerThrowCandidates(throws, {2010, 20});
    REQUIRE(rows);
    CHECK(*rows == std::vector{10, 50}); // Sorted; unsupported matching rows are retained.
    for (const auto [query, expected] : {std::pair{PlayerThrowQuery{0, 20}, 60},
        {PlayerThrowQuery{2010, 1}, 70}, {PlayerThrowQuery{2010, 20, 10}, 80},
        {PlayerThrowQuery{2010, 20, 0, 1}, 40}}) {
        const auto result = playerThrowCandidates(throws, query);
        REQUIRE(result);
        CHECK(*result == std::vector{expected});
    }
    CHECK_FALSE(playerThrowCandidates(throws, {9999, 20})); // No fallback to defender 0.
    for (const auto query : {PlayerThrowQuery{-1, 20}, {2010, -1}, {2010, 256},
                             {2010, 20, -1}, {2010, 20, 0, 256}})
        CHECK_FALSE(playerThrowCandidates(throws, query));
    throws.defaults.erase("AtkEnableState");
    CHECK_FALSE(playerThrowCandidates(throws, {2010, 20}));
}

TEST_CASE("behavior graph - target throws require common supported hit data", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(throwFixture());
    ParamTable throws, weapons;
    throws.loadXml(R"(<param><fields><field name="AtkChrId" defaultValue="0"/>
      <field name="DefChrId" defaultValue="2010"/><field name="throwType" defaultValue="20"/>
      <field name="throwTypeId" defaultValue="0"/><field name="AtkEnableState" defaultValue="0"/>
      <field name="isSkipWepCate" defaultValue="0"/><field name="atkAnimId" defaultValue="77"/></fields>
      <rows><row id="10" Dist="1"/><row id="20" Dist="3"/>
      <row id="30" throwType="25"/><row id="40" throwType="25" atkAnimId="88"/>
      <row id="50" throwType="24"/><row id="60" throwType="24" isSkipWepCate="1"/>
      <row id="70" throwType="22"/><row id="80" throwType="22" atkAnimId="80"/>
      </rows></param>)");
    weapons.loadXml(R"(<param><fields><field name="wepmotionCategory" defaultValue="23"/>
      <field name="spAtkcategory" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/><field name="enableThrow" defaultValue="1"/>
      </fields><rows><row id="1"/><row id="2" enableThrow="0"/><row id="3" spAtkcategory="114"/>
      <row id="4" isWeaponCatalyst="1"/><row id="5" wepmotionBothHandId="0"/>
      <row id="6" enableThrow="2"/></rows></param>)");
    const MoveDefinition seed{MoveId::RIPOSTE, "Caller label", -1, {}, {}, 530};
    const auto result = graph.resolveWeaponThrow(seed, weapons, 1, throws, {2010, 20});
    REQUIRE(result);
    auto expected = seed;
    expected.animationId = 77;
    expected.motionCategory = 23;
    expected.eventType = 304;
    CHECK(erdo::json::write(*result) == erdo::json::write(expected));
    const auto ambiguous = graph.resolveWeaponThrow(seed, weapons, 1, throws, {2010, 25});
    REQUIRE_FALSE(ambiguous);
    CHECK_THAT(ambiguous.error(), Catch::Matchers::ContainsSubstring("Ambiguous throw animations"));
    for (const int type : {22, 24}) { // Never discard a matching unsupported row.
        const auto unsupported = graph.resolveWeaponThrow(seed, weapons, 1, throws, {2010, type});
        REQUIRE_FALSE(unsupported);
        CHECK_THAT(unsupported.error(), Catch::Matchers::ContainsSubstring("Throw row"));
    }
    for (const int weapon : {2, 3, 4, 5, 6, 999})
        CHECK_FALSE(graph.resolveWeaponThrow(seed, weapons, weapon, throws, {2010, 20}));
    CHECK_FALSE(graph.resolveWeaponThrow(seed, weapons, 1, throws, {9999, 20}));
    weapons.defaults.erase("enableThrow");
    CHECK_FALSE(graph.resolveWeaponThrow(seed, weapons, 1, throws, {2010, 20}));
}

TEST_CASE("behavior graph - neutral feints follow weapon category and stance", "[moveset][unit][behavior_graph]")
{
    const std::array ids{MoveId::ONE_HAND_CHARGED_R2_1_FEINT, MoveId::ONE_HAND_CHARGED_R2_2_FEINT,
        MoveId::TWO_HAND_CHARGED_R2_1_FEINT, MoveId::TWO_HAND_CHARGED_R2_2_FEINT};
    const std::array animations{81, 3, 900, 123}; // No clip arithmetic.
    auto xml = fixture.substr(0, fixture.find(" <object"));
    const auto objects = fixture.substr(fixture.find(" <object"), fixture.rfind("</hktagfile>") - fixture.find(" <object"));
    int next = 0;
    auto add = [&](std::string_view state, int offset, int animation) {
        auto part = objects;
        replaceText(part, "AttackRightLight1", state);
        replaceText(part, "value=\"13\"", std::format("value=\"{}\"", offset));
        replaceText(part, "a023_", "a027_");
        replaceText(part, "a110_", "a039_");
        auto curved = part.substr(part.find(" <object id=\"base\""),
            part.find(" <object id=\"special\"") - part.find(" <object id=\"base\""));
        replaceText(curved, "id=\"base\"", "id=\"curved\"");
        replaceText(curved, "a027_", "a028_");
        auto katana = curved;
        replaceText(katana, "id=\"curved\"", "id=\"katana\"");
        replaceText(katana, "a028_", "a061_");
        part += curved;
        part += katana;
        replaceText(part, "array count=\"2\"", "array count=\"4\"");
        replaceText(part, "<pointer id=\"special\"/>", "<pointer id=\"special\"/><pointer id=\"curved\"/><pointer id=\"katana\"/>");
        replaceText(part, "000077", std::format("{:06}", animation));
        replaceText(part, "value=\"77\"", std::format("value=\"{}\"", animation));
        for (const auto* id : {"state", "selector", "base", "special", "curved", "katana"})
            replaceText(part, std::format("id=\"{}\"", id), std::format("id=\"{}{}\"", id, next));
        ++next;
        xml += part;
    };
    std::vector<MoveDefinition> catalog;
    for (auto first : {MoveId::ONE_HAND_R1_1, MoveId::TWO_HAND_R1_1, MoveId::OFFHAND_R1_1, MoveId::POWERSTANCE_L1_1}) {
        const auto chain = lightAttackChain(27, 0, first);
        REQUIRE(chain);
        for (auto definition : *chain) {
            const auto state = directAttackState(definition.id);
            REQUIRE(state);
            add(state->first, state->second, next + 1000);
            definition.animationId = 7777;
            catalog.push_back(definition);
        }
    }
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto state = feintAttackState(ids[i]);
        REQUIRE(state);
        add(state->first, state->second, animations[i]);
        catalog.push_back({ids[i], "Feint", 7777, 12, 456, 7, {}, 1});
    }
    xml += "</hktagfile>";
    BehaviorGraph graph;
    graph.loadXml(xml);
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="27"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/><field name="isDualBlade" defaultValue="0"/>
      <field name="behaviorVariationId" defaultValue="500"/><field name="wepType" defaultValue="15"/>
      <field name="enableThrow" defaultValue="1"/></fields><rows><row id="1"/>
      <row id="2" wepmotionCategory="39" wepmotionOneHandId="3"/>
      <row id="3" spAtkcategory="145"/><row id="4" wepmotionCategory="23"/>
      <row id="5" isWeaponCatalyst="1"/><row id="6" wepmotionCategory="39"/>
      <row id="7" wepmotionCategory="39" wepmotionOneHandId="3" wepmotionBothHandId="13"/>
      <row id="8" wepmotionCategory="28" behaviorVariationId="700" wepType="9"/>
      <row id="9" wepmotionCategory="28" spAtkcategory="155"/>
      <row id="10" wepmotionCategory="61" behaviorVariationId="6600" wepType="94"
        wepmotionOneHandId="2" wepmotionBothHandId="12"/>
      <row id="11" wepmotionCategory="61" wepmotionOneHandId="2" wepmotionBothHandId="12" spAtkcategory="953"/>
      <row id="12" wepmotionCategory="61" wepmotionBothHandId="12"/>
      <row id="13" wepmotionCategory="61" wepmotionOneHandId="2"/>
      </rows></param>)");
    const auto bridged = graph.resolveSupportedCatalog(weapons, std::array{1LL}, catalog);
    REQUIRE(bridged.size() == catalog.size());
    const auto bridgedCurved = graph.resolveSupportedCatalog(weapons, std::array{8LL}, catalog);
    REQUIRE(bridgedCurved.size() == catalog.size());
    auto katanaCatalog = catalog;
    std::erase_if(katanaCatalog, [](const auto& definition) {
        using enum MoveId;
        return definition.id == ONE_HAND_R1_5 || definition.id == ONE_HAND_R1_6
            || definition.id == TWO_HAND_R1_5 || definition.id == TWO_HAND_R1_6
            || definition.id == OFFHAND_R1_5 || definition.id == OFFHAND_R1_6
            || definition.id == POWERSTANCE_L1_4;
    });
    const auto bridgedKatana = graph.resolveSupportedCatalog(weapons, std::array{10LL}, katanaCatalog);
    REQUIRE(bridgedKatana.size() == 19);
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto& seed = catalog[catalog.size() - ids.size() + i];
        for (const int weapon : {1, 2, 8, 10}) {
            const auto result = graph.resolveWeaponFeintMove(seed, weapons, weapon);
            REQUIRE(result);
            auto expected = seed;
            expected.animationId = animations[i];
            expected.motionCategory = weapon == 1 ? 27 : weapon == 8 ? 28 : weapon == 10 ? 61 : 39;
            CHECK(erdo::json::write(*result) == erdo::json::write(expected));
            if (weapon == 1) CHECK(erdo::json::write(bridged[catalog.size() - ids.size() + i]) == erdo::json::write(expected));
            if (weapon == 8) CHECK(erdo::json::write(bridgedCurved[catalog.size() - ids.size() + i]) == erdo::json::write(expected));
            if (weapon == 10) CHECK(erdo::json::write(bridgedKatana[bridgedKatana.size() - ids.size() + i]) == erdo::json::write(expected));
        }
        for (const int weapon : {3, 4, 5, 9, 11, 999}) CHECK_FALSE(graph.resolveWeaponFeintMove(seed, weapons, weapon));
        CHECK_FALSE(graph.resolveWeaponFeintMove(seed, weapons, i < 2 ? 6 : 7));
        CHECK_FALSE(graph.resolveWeaponFeintMove(seed, weapons, i < 2 ? 12 : 13));
    }
    CHECK_FALSE(feintAttackState(MoveId::ONE_HAND_R2_1));
    CHECK_FALSE(graph.resolveWeaponFeintMove({MoveId::ONE_HAND_R2_1, "Heavy", -1}, weapons, 1));
    // A valid seed cannot conceal a missing graph state or a wrong selector offset.
    auto broken = xml;
    replaceText(broken, "AttackBothSpecial2", "MissingFeint");
    graph.loadXml(broken);
    CHECK_FALSE(graph.resolveWeaponFeintMove(catalog.back(), weapons, 1));
    REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{1LL}, catalog),
        Catch::Matchers::ContainsSubstring("Missing behavior state"));
    replaceText(xml, "value=\"16\"", "value=\"13\"");
    graph.loadXml(xml);
    CHECK_FALSE(graph.resolveWeaponFeintMove(catalog.back(), weapons, 1));
}

TEST_CASE("behavior graph - backstab whiff audits script and preserves strike phase", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(whiffFixture());
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields><field name="wepmotionCategory" defaultValue="23"/>
      <field name="spAtkcategory" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/></fields><rows><row id="1"/>
      <row id="2" spAtkcategory="114"/></rows></param>)");
    const MoveDefinition seed{MoveId::BACKSTAB_WHIFF, "Whiff", 999, {}, {}, 509};
    const auto result = graph.resolveWeaponBackstabWhiff(seed, weapons, 1);
    REQUIRE(result);
    CHECK(result->animationId == 77);
    CHECK(result->motionCategory == 23);
    CHECK(result->eventJudgeId == 509);
    CHECK_FALSE(graph.resolveWeaponBackstabWhiff(seed, weapons, 2));
    auto missing = seed;
    missing.eventJudgeId.reset();
    CHECK_FALSE(graph.resolveWeaponBackstabWhiff(missing, weapons, 1));
    missing = seed;
    missing.id = MoveId::BACKSTAB;
    CHECK_FALSE(graph.resolveWeaponBackstabWhiff(missing, weapons, 1));
    auto xml = whiffFixture();
    SECTION("Unknown activation") { replaceText(xml, "ThrowBackStab_Activate()", "Throw_Activate()"); }
    SECTION("Update script") { replaceText(xml, "name=\"onPreUpdateScript\"><string value=\"\"", "name=\"onPreUpdateScript\"><string value=\"Unknown()\""); }
    SECTION("Cycle") { replaceText(xml, "name=\"child\"><pointer id=\"selector\"", "name=\"child\"><pointer id=\"script\""); }
    graph.loadXml(xml);
    CHECK_FALSE(graph.resolveWeaponBackstabWhiff(seed, weapons, 1));
}

TEST_CASE("behavior graph - shieldpoke requires guard context", "[moveset][unit][behavior_graph]")
{
    auto xml = selectorFixture(true);
    replaceText(xml, "AttackRightLight1", "AttackRightWhileGuard");
    replaceText(xml, "AttackVariant", "IndexAttackWhileGuard");
    replaceText(xml, "a023_", "a036_");
    replaceText(xml, "id=\"chooser\"/>", "id=\"guardScript\"/>");
    xml.resize(xml.rfind("</hktagfile>"));
    xml += R"xml(<type id="scriptType"><name value="hkbScriptGenerator"/></type>
      <object id="guardScript" typeid="scriptType"><record>
      <field name="variableBindingSet"><pointer id="object0"/></field>
      <field name="child"><pointer id="chooser"/></field>
      <field name="onActivateScript"><string value="AttackRightWhileGuard_Activate()"/></field>
      <field name="onPreUpdateScript"><string value=""/></field>
      <field name="onGenerateScript"><string value=""/></field>
      <field name="onHandleEventScript"><string value=""/></field>
      <field name="onDeactivateScript"><string value=""/></field>
      </record></object></hktagfile>)xml";
    BehaviorGraph graph;
    graph.loadXml(xml);
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="36"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="3"/><field name="isWeaponCatalyst" defaultValue="0"/>
      </fields><rows><row id="1"/><row id="2" spAtkcategory="200"/>
      <row id="3" wepmotionCategory="38"/><row id="4" isWeaponCatalyst="1"/>
      <row id="5" wepmotionOneHandId="0"/></rows></param>)");
    const MoveDefinition seed{MoveId::SHIELDPOKE, "Guard attack", 999, {}, {}, 170};
    for (const auto [style, animation] : {std::pair{0, 77}, {1, 88}}) {
        const auto result = graph.resolveWeaponShieldpoke(seed, weapons, 1, style, false);
        REQUIRE(result);
        CHECK(result->animationId == animation);
        CHECK(result->motionCategory == 36);
        CHECK(result->eventJudgeId == seed.eventJudgeId);
        CHECK(result->name == seed.name);
    }
    CHECK_FALSE(graph.inspect("AttackRightWhileGuard"));
    for (const int id : {2, 3, 4, 5, 999})
        CHECK_FALSE(graph.resolveWeaponShieldpoke(seed, weapons, id, 0, false));
    for (const int style : {-1, 4})
        CHECK_FALSE(graph.resolveWeaponShieldpoke(seed, weapons, 1, style, false));
    CHECK_FALSE(graph.resolveWeaponShieldpoke(seed, weapons, 1, 0, true));
    CHECK_FALSE(graph.resolveWeaponShieldpoke({MoveId::ONE_HAND_R1_1, "Other", -1}, weapons, 1, 0, false));
    SECTION("Unknown activation") { replaceText(xml, "AttackRightWhileGuard_Activate()", "Unknown()"); }
    SECTION("Update script") { replaceText(xml, "name=\"onPreUpdateScript\"><string value=\"\"", "name=\"onPreUpdateScript\"><string value=\"Unknown()\""); }
    SECTION("Missing state") { replaceText(xml, "AttackRightWhileGuard", "UnrelatedState"); }
    SECTION("Wrong offset") { replaceText(xml, "value=\"13\"", "value=\"16\""); }
    SECTION("Cycle") { replaceText(xml, "name=\"child\"><pointer id=\"chooser\"", "name=\"child\"><pointer id=\"guardScript\""); }
    graph.loadXml(xml);
    CHECK_FALSE(graph.resolveWeaponShieldpoke(seed, weapons, 1, 0, false));
}

TEST_CASE("behavior graph - mounted sides and explicit heavy phases", "[moveset][unit][behavior_graph]")
{
    using enum MoveId;
    struct Entry { MoveId id; std::string_view state; std::optional<int> judge; };
    const std::array entries{
        Entry{MOUNTED_R1_1, "RideAttack_R_Top", {}},
        Entry{MOUNTED_R1_2, "RideAttack_R_Top02", {}},
        Entry{MOUNTED_R1_3, "RideAttack_R_Top03", {}},
        Entry{MOUNTED_L1_1, "RideAttack_L_Top", {}},
        Entry{MOUNTED_L1_2, "RideAttack_L_Top02", {}},
        Entry{MOUNTED_L1_3, "RideAttack_L_Top03", {}},
        Entry{MOUNTED_R2, "RideAttack_R_Hard1_End", {}},
        Entry{MOUNTED_L2, "RideAttack_L_Hard1_End", {}},
        Entry{MOUNTED_R2_CHARGING, "RideAttack_R_Hard1_Start", 630},
        Entry{MOUNTED_CHARGED_R2, "RideAttack_R_Hard1_Start", 635},
        Entry{MOUNTED_L2_CHARGING, "RideAttack_L_Hard1_Start", 730},
        Entry{MOUNTED_CHARGED_L2, "RideAttack_L_Hard1_Start", 735}};
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionBothHandId" defaultValue="10"/><field name="isWeaponCatalyst" defaultValue="0"/>
      </fields><rows><row id="1"/><row id="2" spAtkcategory="114"/>
      <row id="3" isWeaponCatalyst="1"/><row id="4" wepmotionBothHandId="0"/>
      <row id="5" wepmotionCategory="999"/></rows></param>)");
    BehaviorGraph graph;
    for (const auto& entry : entries) {
        CAPTURE(entry.state, entry.judge);
        auto xml = changed("AttackRightLight1", entry.state);
        replaceText(xml, "value=\"13\"", "value=\"16\"");
        graph.loadXml(xml);
        const MoveDefinition seed{entry.id, "Mounted", 99999, 12, 100200630, entry.judge, 999};
        const auto result = graph.resolveWeaponMountedMove(seed, weapons, 1);
        REQUIRE(result);
        auto expected = seed;
        expected.animationId = 77; // Graph wins over the deliberately wrong seed.
        expected.motionCategory = 23;
        CHECK(erdo::json::write(*result) == erdo::json::write(expected));
        for (const int unsupported : {2, 3, 4, 5, 999})
            CHECK_FALSE(graph.resolveWeaponMountedMove(seed, weapons, unsupported));
        if (entry.judge) {
            auto unfiltered = seed;
            unfiltered.eventJudgeId.reset();
            const auto missing = graph.resolveWeaponMountedMove(unfiltered, weapons, 1);
            REQUIRE_FALSE(missing);
            CHECK_THAT(missing.error(), Catch::Matchers::ContainsSubstring("event phase filter"));
        }
        // Mounted L is still the selected weapon's hand-style selector, not offhand (14).
        replaceText(xml, "value=\"16\"", "value=\"14\"");
        graph.loadXml(xml);
        CHECK_FALSE(graph.resolveWeaponMountedMove(seed, weapons, 1));
        replaceText(xml, entry.state, "UnrelatedState");
        graph.loadXml(xml);
        CHECK_FALSE(graph.resolveWeaponMountedMove(seed, weapons, 1));
    }
    CHECK_FALSE(mountedAttackState(ONE_HAND_JUMPING_R1));
    CHECK_FALSE(graph.resolveWeaponMountedMove({OFFHAND_R1_1, "Other", -1}, weapons, 1));
}

TEST_CASE("behavior graph - airborne jump selectors preserve context", "[moveset][unit][behavior_graph]")
{
    BehaviorGraph graph;
    graph.loadXml(jumpingFixture());
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/></fields><rows>
      <row id="1"/><row id="2" spAtkcategory="110"/><row id="3" spAtkcategory="293"/>
      <row id="4" isWeaponCatalyst="1"/></rows></param>)");
    for (std::size_t i = 0; i < jumpingIds.size(); ++i) {
        CAPTURE(i);
        const MoveDefinition seed{jumpingIds[i], "Jump", -1, {}, {}, 321};
        const auto resolved = graph.resolveWeaponJumpingMove(seed, weapons, 1);
        REQUIRE(resolved);
        CHECK(resolved->animationId == 77 + i);
        CHECK(resolved->motionCategory == 23);
        CHECK(resolved->eventJudgeId == 321);
        CHECK(resolved->name == seed.name);
        CHECK(resolved->id == seed.id);
        const auto special = graph.resolveWeaponJumpingMove(seed, weapons, 2);
        REQUIRE(special);
        CHECK(special->motionCategory == 110);
        CHECK_FALSE(graph.resolveWeaponJumpingMove(seed, weapons, 4));
        auto route = *jumpingAttackRoute(seed.id);
        route.variables["JumpAttack_Land"] = 1;
        const auto landed = graph.resolve(seed, route.stateName, route.offsetType, std::array{23}, route.variables);
        REQUIRE_FALSE(landed);
        CHECK_THAT(landed.error(), Catch::Matchers::ContainsSubstring("Dynamic graph variable bindings"));
    }
    CHECK_FALSE(graph.resolveWeaponJumpingMove({MoveId::ONE_HAND_JUMPING_R2, "Magic", -1}, weapons, 3));
    CHECK_FALSE(graph.resolveWeaponJumpingMove({MoveId::ONE_HAND_R1_1, "Other", -1}, weapons, 1));
    CHECK_FALSE(jumpingAttackRoute(MoveId::MOUNTED_R1_1));
    CHECK_FALSE(graph.inspect("JumpAttack_Start_Falling"));
}

TEST_CASE("behavior graph - only inert selector transitions are supported", "[moveset][unit][behavior_graph]")
{
    auto xml = jumpingFixture();
    SECTION("Duration") { replaceText(xml, "name=\"duration\"><real dec=\"0\"", "name=\"duration\"><real dec=\"0.1\""); }
    SECTION("Start fraction") { replaceText(xml, "name=\"toGeneratorStartTimeFraction\"><real dec=\"0\"", "name=\"toGeneratorStartTimeFraction\"><real dec=\"0.5\""); }
    SECTION("Invalid real") { replaceText(xml, "name=\"duration\"><real dec=\"0\"", "name=\"duration\"><real dec=\"nan\""); }
    SECTION("Flags") { replaceText(xml, "name=\"flags\"><integer value=\"0\"", "name=\"flags\"><integer value=\"1\""); }
    SECTION("Alignment") { replaceText(xml, "name=\"alignmentBone\"><integer value=\"-1\"", "name=\"alignmentBone\"><integer value=\"0\""); }
    SECTION("Wrong type") { replaceText(xml, "CustomTransitionEffect", "hkbBlendingTransitionEffect"); }
    SECTION("Bound transition") { replaceText(xml, "id=\"instant\" typeid=\"instantType\"><record>\n      <field name=\"variableBindingSet\"><pointer id=\"object0\"", "id=\"instant\" typeid=\"instantType\"><record>\n      <field name=\"variableBindingSet\"><pointer id=\"binding\""); }
    BehaviorGraph graph;
    graph.loadXml(xml);
    const auto route = *jumpingAttackRoute(MoveId::ONE_HAND_JUMPING_R1);
    CHECK_FALSE(graph.inspect(route.stateName, route.variables));
}

TEST_CASE("behavior graph - movement and guard-counter entry states", "[moveset][unit][behavior_graph]")
{
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="110"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="wepmotionBothHandId" defaultValue="10"/>
      <field name="isWeaponCatalyst" defaultValue="0"/></fields><rows><row id="1"/></rows></param>)");
    struct Entry { MoveId id; std::string_view state; int offset; };
    const std::array entries{
        Entry{MoveId::ONE_HAND_RUNNING_R1, "AttackRightLightDash", 13},
        Entry{MoveId::ONE_HAND_RUNNING_R2, "AttackRightHeavyDash", 13},
        Entry{MoveId::ONE_HAND_ROLLING_R1, "AttackRightLightStep", 13},
        Entry{MoveId::ONE_HAND_BACKSTEP_R1, "AttackRightBackstep", 13},
        Entry{MoveId::ONE_HAND_GUARD_COUNTER, "AttackRightHeavyCounter", 13},
        Entry{MoveId::TWO_HAND_RUNNING_R1, "AttackBothDash", 16},
        Entry{MoveId::TWO_HAND_RUNNING_R2, "AttackBothHeavyDash", 16},
        Entry{MoveId::TWO_HAND_ROLLING_R1, "AttackBothLightStep", 16},
        Entry{MoveId::TWO_HAND_BACKSTEP_R1, "AttackBothBackstep", 16},
        Entry{MoveId::TWO_HAND_GUARD_COUNTER, "AttackBothHeavyCounter", 16}
    };
    BehaviorGraph graph;
    for (const auto& entry : entries) {
        CAPTURE(entry.state);
        auto xml = changed("AttackRightLight1", entry.state);
        replaceText(xml, "value=\"13\"", std::format("value=\"{}\"", entry.offset));
        graph.loadXml(xml);
        const MoveDefinition seed{entry.id, "Movement", -1, {}, {}, 321};
        const auto result = graph.resolveWeaponMovementMove(seed, weapons, 1);
        REQUIRE(result);
        CHECK(result->animationId == 77);
        CHECK(result->motionCategory == 110);
        CHECK(result->id == entry.id);
        CHECK(result->eventJudgeId == 321);
        const auto direct = graph.resolveMove(seed, std::array{110, 23});
        REQUIRE(direct);
        CHECK(erdo::json::write(*direct) == erdo::json::write(*result));
        replaceText(xml, "a110_000077", "a111_000077");
        graph.loadXml(xml);
        const auto fallback = graph.resolveWeaponMovementMove(seed, weapons, 1);
        REQUIRE(fallback);
        CHECK(fallback->motionCategory == 23);
        replaceText(xml, std::format("value=\"{}\"", entry.offset), "value=\"14\"");
        graph.loadXml(xml);
        CHECK_FALSE(graph.resolveWeaponMovementMove(seed, weapons, 1));
    }
    // These misleadingly named states share clips in the real graph, but HKS
    // dispatches guard-counter R2 to HeavyCounter. Never substitute LightCounter.
    for (const auto [id, lightState] : {
        std::pair{MoveId::ONE_HAND_GUARD_COUNTER, "AttackRightLightCounter"},
        {MoveId::TWO_HAND_GUARD_COUNTER, "AttackBothLightCounter"}}) {
        graph.loadXml(changed("AttackRightLight1", lightState));
        CHECK_FALSE(graph.resolveWeaponMovementMove({id, "Counter", 30700}, weapons, 1));
    }
    for (const auto id : {MoveId::ONE_HAND_R1_1, MoveId::ONE_HAND_JUMPING_R1,
                         MoveId::POWERSTANCE_RUNNING_L1, MoveId::OFFHAND_R1_1})
        CHECK_FALSE(graph.resolveWeaponMovementMove({id, "Other", -1}, weapons, 1));
}

TEST_CASE("behavior graph - offhand and matching powerstance contexts", "[moveset][unit][behavior_graph]")
{
    ParamTable weapons;
    weapons.loadXml(R"(<param><fields>
      <field name="wepmotionCategory" defaultValue="23"/><field name="spAtkcategory" defaultValue="0"/>
      <field name="wepmotionOneHandId" defaultValue="0"/><field name="isWeaponCatalyst" defaultValue="0"/>
      </fields><rows><row id="1"/><row id="2" spAtkcategory="110"/>
      <row id="3" wepmotionCategory="21"/><row id="4" wepmotionCategory="57"/>
      <row id="5" wepmotionCategory="999"/><row id="6" isWeaponCatalyst="1"/>
      <row id="7" spAtkcategory="114"/>
      </rows></param>)");
    auto xml = fixture.substr(0, fixture.find(" <object"));
    const auto objects = fixture.substr(fixture.find(" <object"), fixture.rfind("</hktagfile>") - fixture.find(" <object"));
    const std::array animations{81, 3, 900, 123, 42, 777};
    int ordinal = 0;
    auto add = [&](std::string_view name, int offset, int animation) {
        auto part = objects;
        replaceText(part, "AttackRightLight1", name);
        replaceText(part, "value=\"13\"", std::format("value=\"{}\"", offset));
        replaceText(part, "value=\"77\"", std::format("value=\"{}\"", animation));
        replaceText(part, "000077", std::format("{:06}", animation));
        for (const auto* id : {"state", "selector", "base", "special"})
            replaceText(part, std::format("id=\"{}\"", id), std::format("id=\"{}{}\"", id, ordinal));
        ++ordinal;
        xml += part;
    };
    for (std::size_t i = 0; i < animations.size(); ++i) {
        add(std::format("AttackLeftHeavy{}", i + 1), 14, animations[i]);
        add(std::format("AttackDualLight{}", i + 1), 13, animations[i]);
    }
    add("AttackDualDash", 13, 711);
    add("AttackDualRolling", 13, 812);
    add("AttackDualBackStep", 13, 913);
    xml += "</hktagfile>";
    BehaviorGraph graph;
    graph.loadXml(xml);
    for (auto first : {MoveId::OFFHAND_R1_1, MoveId::POWERSTANCE_L1_1}) {
        const auto chain = graph.resolveLightChain(weapons, 1, first);
        REQUIRE(chain);
        REQUIRE(chain->size() == (first == MoveId::OFFHAND_R1_1 ? 5 : 4));
        for (std::size_t i = 0; i < chain->size(); ++i) {
            CHECK((*chain)[i].animationId == animations[i]);
            CHECK((*chain)[i].motionCategory == 23);
        }
        const auto special = graph.resolveLightChain(weapons, 2, first);
        REQUIRE(special);
        CHECK(special->front().motionCategory == 110);
        const auto unaudited = graph.resolveLightChain(weapons, 7, first);
        REQUIRE_FALSE(unaudited);
        CHECK_THAT(unaudited.error(), Catch::Matchers::ContainsSubstring("requires explicit context"));
        const auto explicitContext = graph.resolveLightChain(weapons, 2, first, std::array{110, 23});
        REQUIRE(explicitContext);
        CHECK(explicitContext->front().motionCategory == 110);
    }
    for (int id : {3, 4, 5, 6, 999}) CHECK_FALSE(offhandOrMatchingPairCategories(weapons, id, true));
    CHECK(offhandOrMatchingPairCategories(weapons, 3, false) == std::vector{21});
    for (const auto [id, animation] : {std::pair{MoveId::POWERSTANCE_RUNNING_L1, 711},
        {MoveId::POWERSTANCE_ROLLING_L1, 812}, {MoveId::POWERSTANCE_BACKSTEP_L1, 913}}) {
        const auto result = graph.resolveMatchingPowerstanceMovementMove({id, "Movement", -1}, weapons, 1);
        REQUIRE(result);
        CHECK(result->animationId == animation);
        CHECK(result->motionCategory == 23);
        const auto special = graph.resolveMatchingPowerstanceMovementMove({id, "Movement", -1}, weapons, 2);
        REQUIRE(special);
        CHECK(special->motionCategory == 110);
        CHECK_FALSE(graph.resolveMatchingPowerstanceMovementMove({id, "Movement", -1}, weapons, 7));
    }
    CHECK_FALSE(graph.resolveMatchingPowerstanceMovementMove({MoveId::POWERSTANCE_JUMPING_L1, "Jump", -1}, weapons, 1));
    replaceText(xml, "AttackLeftHeavy3", "MissingOffhand");
    replaceText(xml, "AttackDualLight3", "MissingDual");
    graph.loadXml(xml);
    CHECK_FALSE(graph.resolveLightChain(weapons, 1, MoveId::OFFHAND_R1_1));
    CHECK_FALSE(graph.resolveLightChain(weapons, 1, MoveId::POWERSTANCE_L1_1));
}

#if defined(ERDO_MOVESET_XML_ROOT) && defined(ERDO_MOVESET_BEHAVIOR_XML)
TEST_CASE("behavior graph - game thrusting feints and incomplete heavy chain", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable weapons, attacks, behaviors;
    weapons.load((root / EquipParamWeaponFile).string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    std::vector<MoveDefinition> expected;
    for (const auto& definition : thrustingSwordCatalog)
        if (feintAttackState(definition.id)) expected.push_back(definition);
    REQUIRE(expected.size() == 4);
    for (const int weapon : {5010000, 5020000, 5040000, 6000000, 6040000, 6500000,
            7000000, 7040000, 7050000, 7080000, 7110000, 7150000, 7500000,
            66500000, 66510000, 66530000}) {
        CAPTURE(weapon);
        const auto row = weapons.find(weapon);
        const int category = *weapons.integer(row, "wepmotionCategory");
        const int variation = *weapons.integer(row, "behaviorVariationId");
        for (const bool twoHanded : {false, true})
            CHECK(ordinaryWeaponMotionPosition(category, twoHanded) ==
                weapons.integer(row, twoHanded ? "wepmotionBothHandId" : "wepmotionOneHandId"));
        std::vector<MoveDefinition> definitions;
        for (const auto& reference : expected) {
            const auto result = graph.resolveWeaponFeintMove({reference.id, reference.name, -1}, weapons, weapon);
            REQUIRE(result);
            CHECK(result->animationId == reference.animationId);
            CHECK(result->motionCategory == category);
            definitions.push_back(*result);
        }
        const auto actual = importMoveset(cache, category, variation, definitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        const auto reference = importMoveset(cache, category, variation, expected, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(actual.report.failures.empty());
        REQUIRE(reference.report.failures.empty());
        CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*reference.moveset));
        if (category != 39) continue;
        for (auto first : {MoveId::ONE_HAND_R1_1, MoveId::TWO_HAND_R1_1, MoveId::OFFHAND_R1_1}) {
            const auto chain = graph.resolveLightChain(weapons, weapon, first);
            REQUIRE(chain);
            REQUIRE(chain->size() == 5);
            CHECK(importMoveset(cache, category, variation, *chain, index, attacks,
                std::nullopt, BehaviorLookup::PLAYER_WEAPON).report.failures.empty());
        }
        const auto pair = graph.resolveLightChain(weapons, weapon, MoveId::POWERSTANCE_L1_1);
        REQUIRE_FALSE(pair);
        CHECK_THAT(pair.error(), Catch::Matchers::ContainsSubstring("Powerstance L1 #4"));
        CHECK_THAT(pair.error(), Catch::Matchers::ContainsSubstring("No clip matches"));
        REQUIRE_THROWS_WITH(graph.resolveSupportedCatalog(weapons, std::array{static_cast<long long>(weapon)}, thrustingSwordCatalog),
            Catch::Matchers::ContainsSubstring("Powerstance L1 #4"));
    }
    const auto fourth = graph.inspect("AttackDualLight4");
    REQUIRE(fourth);
    CHECK(fourth->animationId == 34030);
    CHECK(std::ranges::find(fourth->clips, 39, &BehaviorClip::motionCategory) == fourth->clips.end());
    CHECK_FALSE(graph.resolveMove({MoveId::POWERSTANCE_L1_4, "Heavy thrusting", 34020}, std::array{39}));
    REQUIRE_THROWS(cache.get(39, 34030));
}

TEST_CASE("behavior graph - selected game throw rows match critical hits", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable throws, attacks, behaviors, weapons;
    weapons.load((root / "regulation-bin/EquipParamWeapon.param.xml").string().c_str());
    throws.load((root / "regulation-bin/ThrowParam.param.xml").string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    // Representative target interactions are explicit TEST contexts, not a
    // production lookup table or an assertion that all small/large foes match.
    struct Context { MoveId id; int row; PlayerThrowQuery query; int count; };
    const std::array contexts{Context{MoveId::BACKSTAB, 10000000, {0, 1}, 1},
        Context{MoveId::RIPOSTE, 13201000, {2010, 20}, 1},
        Context{MoveId::SMALL_PVE_BACKSTAB, 11317000, {3170, 1}, 1},
        Context{MoveId::SMALL_PVE_RIPOSTE, 16395000, {3950, 22}, 8},
        Context{MoveId::LARGE_PVE_RIPOSTE, 13212000, {2120, 20}, 1}};
    for (const auto weaponId : {24000000, 24020000, 24060000, 24070000})
        for (const auto& context : contexts)
            CHECK_FALSE(graph.resolveWeaponThrow({context.id, "Disabled torch throw", -1},
                weapons, weaponId, throws, context.query));
    struct Profile { int category; int variation; int weaponId; std::span<const MoveDefinition> catalog; };
    const std::array profiles{
        Profile{20, 100, 1000000, daggerCatalog}, Profile{23, 200, 2000000, straightSwordCatalog},
        Profile{30, 1400, 14000000, axeCatalog}, Profile{33, 1100, 11030000, hammerCatalog},
        Profile{34, 1300, 13010000, flailCatalog}, Profile{25, 300, 3000000, greatswordCatalog},
        Profile{32, 1500, 15000000, greataxeCatalog}, Profile{35, 1200, 12000000, greathammerCatalog},
        Profile{29, 900, 9000000, katanaCatalog}, Profile{40, 800, 8020000, curvedGreatswordCatalog},
        Profile{26, 400, 4010000, colossalSwordCatalog}, Profile{31, 2300, 23060000, colossalWeaponCatalog},
        Profile{38, 1800, 18010000, halberdCatalog}, Profile{50, 1900, 19010000, scytheCatalog},
        Profile{36, 1600, 16020000, spearCatalog}, Profile{37, 1700, 16550000, greatSpearCatalog},
        Profile{27, 500, 5010000, thrustingSwordCatalog}, Profile{28, 700, 7000000, curvedSwordCatalog},
        Profile{61, 6600, 66500000, greatKatanaCatalog}, Profile{60, 6700, 67500000, lightGreatswordCatalog},
        Profile{57, 6100, 62500000, thrustingShieldCatalog}, Profile{22, 2200, 22000000, clawCatalog},
        Profile{42, 2100, 21070000, fistCatalog}, Profile{42, 2121, 21520000, fistCatalog}, Profile{62, 6800, 68500000, beastClawCatalog}, Profile{58, 6400, 64500000, backhandBladeCatalog}, Profile{55, 6300, 60500000, martialArtsCatalog},
        Profile{55, 6301, 60510000, footworkCatalog}, Profile{23, 201, 2020000, straightSword110Catalog}};
    for (const auto& [category, variation, weaponId, catalog] : profiles) {
        std::vector<MoveDefinition> definitions, expectedDefinitions;
        for (const auto [id, row, query, count] : contexts) {
            CAPTURE(category, row);
            const auto reference = std::ranges::find(catalog, id, &MoveDefinition::id);
            REQUIRE(reference != catalog.end());
            const auto result = graph.resolvePlayerThrow({id, reference->name, -1}, throws, row, std::array{category});
            if (!result) FAIL(result.error());
            const auto candidates = playerThrowCandidates(throws, query);
            REQUIRE(candidates);
            CHECK(candidates->size() == count);
            const auto byTarget = graph.resolveWeaponThrow({id, reference->name, -1}, weapons,
                weaponId, throws, query);
            if (!byTarget) FAIL(byTarget.error());
            CHECK(erdo::json::write(*byTarget) == erdo::json::write(*result));
            definitions.push_back(*byTarget);
            expectedDefinitions.push_back(*reference);
        }
        const auto actual = importMoveset(cache, category, variation, definitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        const auto expected = importMoveset(cache, category, variation, expectedDefinitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(actual.report.failures.empty());
        REQUIRE(expected.report.failures.empty());
        CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*expected.moveset));
    }
    CHECK_FALSE(graph.inspect("ThrowAtk"));
    CHECK_FALSE(graph.inspect("ThrowAtk", {{"ThrowID", 31701}}));
    const auto ambiguous = graph.resolveWeaponThrow({MoveId::RIPOSTE, "Target", -1}, weapons,
        2000000, throws, {2010, 25});
    REQUIRE_FALSE(ambiguous);
    CHECK_THAT(ambiguous.error(), Catch::Matchers::ContainsSubstring("Ambiguous throw animations"));
}

TEST_CASE("behavior graph - complete GameData import and JSON", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    const auto reference = load_game_data(root);
    std::vector<MovesetImportReport> reports;
    const auto actual = load_game_data(root, graph, &reports);
    REQUIRE(reports.size() == 87);
    for (const auto& report : reports) {
        CHECK(report.importedMoves == (report.motionCategory == 20 ? 66
            : report.motionCategory == 27 ? 71
            : report.motionCategory == 28 ? 70
            : report.motionCategory == 61 ? 63
            : report.motionCategory == 60 ? 61
            : report.motionCategory == 57 ? 52
            : report.motionCategory == 21 ? 46
            : report.motionCategory == 62 ? 64
            : report.motionCategory == 22 || report.motionCategory == 42 || report.motionCategory == 55 || report.motionCategory == 58 ? 68
            : report.motionCategory == 43 ? 51
            : report.motionCategory == 26 || report.motionCategory == 31 ? 56
            : report.motionCategory == 50 || report.motionCategory == 37 ? 57 : report.motionCategory == 34 || report.motionCategory == 36 ? 60
            : report.motionCategory == 25 || report.motionCategory == 32 || report.motionCategory == 35
                || report.motionCategory == 40 || report.motionCategory == 38 ? 59 : 63));
        CHECK(report.failures.empty());
    }
    QTemporaryDir scratch;
    REQUIRE(scratch.isValid());
    const auto file = std::filesystem::path(scratch.path().toStdString()) / "graph-game-data.json";
    erdo::json::save(file, actual);
    const auto restored = erdo::json::load<GameData>(file);
    REQUIRE(actual.weapons.size() == reference.weapons.size());
    REQUIRE(restored.weapons.size() == actual.weapons.size());
    CHECK(actual.game_version == reference.game_version);
    CHECK(restored.game_version == reference.game_version);
    std::map<std::pair<int, int>, std::shared_ptr<Moveset>> shared;
    std::size_t supported = 0;
    for (std::size_t i = 0; i < reference.weapons.size(); ++i) {
        const auto& expected = reference.weapons[i];
        const auto& imported = actual.weapons[i];
        const auto& loaded = restored.weapons[i];
        CAPTURE(expected.full_name);
        CHECK(imported.full_name == expected.full_name);
        CHECK(loaded.full_name == expected.full_name);
        REQUIRE(bool(imported.moveset) == bool(expected.moveset));
        REQUIRE(bool(loaded.moveset) == bool(expected.moveset));
        if (!expected.moveset) continue;
        ++supported;
        CHECK(erdo::json::write(*imported.moveset) == erdo::json::write(*expected.moveset));
        CHECK(erdo::json::write(*loaded.moveset) == erdo::json::write(*expected.moveset));
        auto& identity = shared[{loaded.moveset->motionCategory, loaded.moveset->behaviorVariation}];
        if (identity) CHECK(identity == loaded.moveset);
        else identity = loaded.moveset;
    }
    CHECK(supported == 1260);
    REQUIRE(shared.size() == 87);
    CHECK(shared.at({20, 100}) != shared.at({23, 200}));
    BehaviorGraph missing;
    missing.loadXml("<hktagfile version='3'/>");
    reports.clear();
    REQUIRE_THROWS_WITH(load_game_data(root, missing, &reports),
        Catch::Matchers::ContainsSubstring("Definition resolution failed"));
    REQUIRE(reports.size() == 87);
    for (const auto& report : reports) {
        CHECK(report.importedMoves == 0);
        CHECK(report.failures.size() == report.requestedMoves);
    }
    // A graph registration and HKS upper limit do not establish that every
    // selected clip has hit data. Fresh a24 lacks the selected fourth entry;
    // keep the explicit diagnostic rather than inventing a three-stage family.
    ParamTable weapons, attacks, behaviors;
    weapons.load((root / EquipParamWeaponFile).string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    // The partial special category selects registered overrides, then base
    // clips. Exact variation rows still win over family rows even when their
    // reference type is unsupported: do not quietly borrow Dryleaf's counter.
    const auto footworkDefinitions = graph.resolveSupportedCatalog(weapons,
        std::array{60510000LL}, footworkCatalog);
    REQUIRE(footworkDefinitions.size() == 68);
    CHECK(std::ranges::count(footworkDefinitions, 246, &MoveDefinition::motionCategory) == 26);
    const auto footwork = importMoveset(cache, 55, 6301, footworkDefinitions, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    const auto footworkReference = importMoveset(cache, 55, 6301, footworkCatalog, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    CHECK(footwork.report.importedMoves == 67);
    REQUIRE(footwork.report.failures.size() == 1);
    const auto& counter = footwork.report.failures.front();
    CHECK(counter.moveId == MoveId::TWO_HAND_GUARD_COUNTER);
    CHECK(counter.animationId == 32700);
    REQUIRE(counter.unresolved.size() == 1);
    REQUIRE(counter.unresolved.front().candidates.size() == 1);
    const auto& bullet = counter.unresolved.front().candidates.front();
    CHECK(bullet.behaviorParamId == 106301311);
    CHECK(bullet.refType == 1);
    CHECK(bullet.refId == 200031311);
    CHECK(erdo::json::write(*footwork.moveset) == erdo::json::write(*footworkReference.moveset));
    CHECK(erdo::json::write(footwork.report) == erdo::json::write(footworkReference.report));
    if (std::filesystem::exists(root / BulletParamFile)) {
        ParamTable bullets;
        bullets.load((root / BulletParamFile).string().c_str());
        const auto linked = importMoveset(cache, 55, 6301, footworkDefinitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON, &bullets);
        const auto linkedReference = importMoveset(cache, 55, 6301, footworkCatalog, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON, &bullets);
        REQUIRE(linked.report.failures.empty());
        CHECK(linked.report.importedMoves == 68);
        CHECK(erdo::json::write(*linked.moveset) == erdo::json::write(*linkedReference.moveset));
    }
    // Partial diagnostic imports must not become application weapon data.
    for (const auto& weapon : actual.weapons)
        if (weapon.base_name == "Dane's Footwork") CHECK_FALSE(weapon.moveset);
    const auto twinChain = graph.resolveLightChain(weapons, 10000000, MoveId::POWERSTANCE_L1_1, std::array{24});
    REQUIRE(twinChain);
    REQUIRE(twinChain->size() == 4);
    CHECK(twinChain->back().animationId == 34030);
    const auto incompleteTwin = importMoveset(cache, 24, 1000, *twinChain, index, attacks,
        std::nullopt, BehaviorLookup::PLAYER_WEAPON);
    CHECK(incompleteTwin.report.requestedMoves == 4);
    CHECK(incompleteTwin.report.importedMoves == 3);
    REQUIRE(incompleteTwin.report.failures.size() == 1);
    CHECK(incompleteTwin.report.failures[0].moveId == MoveId::POWERSTANCE_L1_4);
    CHECK(incompleteTwin.report.failures[0].animationId == 34030);
}

TEST_CASE("behavior graph - game weapon parameters select ordinary categories", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable weapons, attacks, behaviors;
    weapons.load((root / "regulation-bin/EquipParamWeapon.param.xml").string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    int checked = 0;
    for (int weaponId : {1000000, 2000000}) {
        const auto row = weapons.find(weaponId);
        const int category = *weapons.integer(row, "wepmotionCategory");
        const int variation = *weapons.integer(row, "behaviorVariationId");
        const auto catalog = weaponId == 1000000 ? std::span<const MoveDefinition>(daggerCatalog)
                                                : std::span<const MoveDefinition>(straightSwordCatalog);
        std::vector<MoveDefinition> definitions, expectedDefinitions;
        for (auto first : {MoveId::ONE_HAND_R1_1, MoveId::TWO_HAND_R1_1, MoveId::OFFHAND_R1_1, MoveId::POWERSTANCE_L1_1}) {
            const auto chain = graph.resolveLightChain(weapons, weaponId, first);
            if (!chain) FAIL(chain.error());
            definitions.insert(definitions.end(), chain->begin(), chain->end());
        }
        for (auto id : {MoveId::ONE_HAND_R2_1, MoveId::ONE_HAND_R2_2, MoveId::ONE_HAND_CHARGED_R2_1,
                       MoveId::ONE_HAND_CHARGED_R2_2, MoveId::TWO_HAND_R2_1, MoveId::TWO_HAND_R2_2,
                       MoveId::TWO_HAND_CHARGED_R2_1, MoveId::TWO_HAND_CHARGED_R2_2}) {
            const auto reference = std::ranges::find(catalog, id, &MoveDefinition::id);
            REQUIRE(reference != catalog.end());
            // Only semantic ID/name comes from the reference, never its clip or constraints.
            const auto heavy = graph.resolveWeaponHeavyMove({id, reference->name, -1}, weapons, weaponId);
            if (!heavy) FAIL(heavy.error());
            definitions.push_back(*heavy);
        }
        for (const auto& reference : catalog) {
            const bool paired = powerstanceMovementState(reference.id).has_value();
            if (!movementAttackState(reference.id) && !paired) continue;
            const auto movement = paired
                ? graph.resolveMatchingPowerstanceMovementMove({reference.id, reference.name, -1}, weapons, weaponId)
                : graph.resolveWeaponMovementMove({reference.id, reference.name, -1}, weapons, weaponId);
            if (!movement) FAIL(movement.error());
            definitions.push_back(*movement);
        }
        for (const auto id : jumpingIds) {
            const auto reference = std::ranges::find(catalog, id, &MoveDefinition::id);
            REQUIRE(reference != catalog.end());
            const auto jumping = graph.resolveWeaponJumpingMove({id, reference->name, -1}, weapons, weaponId);
            if (!jumping) FAIL(jumping.error());
            definitions.push_back(*jumping);
            auto route = *jumpingAttackRoute(id);
            route.variables["JumpAttack_Land"] = 1;
            const auto landing = graph.inspect(route.stateName, route.variables);
            REQUIRE_FALSE(landing);
            CHECK_THAT(landing.error(), Catch::Matchers::ContainsSubstring("Dynamic graph variable bindings"));
        }
        for (const auto& reference : catalog) {
            const bool whiff = reference.id == MoveId::BACKSTAB_WHIFF;
            if (!mountedAttackState(reference.id) && !whiff) continue;
            // Keep the established phase filter, but neither animation/category
            // nor behavior-row constraints may seed the graph/player lookup.
            const MoveDefinition seed{reference.id, reference.name, -1, {}, {}, reference.eventJudgeId};
            const auto mounted = whiff ? graph.resolveWeaponBackstabWhiff(seed, weapons, weaponId)
                : graph.resolveWeaponMountedMove(seed, weapons, weaponId);
            if (!mounted) FAIL(mounted.error());
            definitions.push_back(*mounted);
        }
        for (const auto& definition : definitions)
            expectedDefinitions.push_back(*std::ranges::find(catalog, definition.id, &MoveDefinition::id));
        const auto actual = importMoveset(cache, category, variation, definitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        const auto expected = importMoveset(cache, category, variation, expectedDefinitions, index, attacks);
        REQUIRE(actual.report.failures.empty());
        REQUIRE(expected.report.failures.empty());
        CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*expected.moveset));
        checked += static_cast<int>(definitions.size());
    }
    CHECK(checked == 119);
    // Partial special sets: both use base lights but override the first heavy.
    for (const auto [weaponId, special, variation] : {
        std::array{2010000, 111, 209}, std::array{2020000, 110, 201}}) {
        CAPTURE(weaponId);
        const auto chain = graph.resolveLightChain(weapons, weaponId, MoveId::ONE_HAND_R1_1);
        REQUIRE(chain);
        REQUIRE(chain->size() == 5);
        CHECK(chain->front().motionCategory == 23);
        const auto heavy = graph.resolveWeaponHeavyMove({MoveId::ONE_HAND_R2_1, "Heavy", -1}, weapons, weaponId);
        if (!heavy) FAIL(heavy.error());
        CHECK(heavy->motionCategory == special);
        auto definitions = *chain;
        definitions.push_back(*heavy);
        const auto imported = importMoveset(cache, 23, variation, definitions, index, attacks,
            std::nullopt, BehaviorLookup::PLAYER_WEAPON);
        INFO(erdo::json::write(imported.report));
        REQUIRE(imported.report.failures.empty());
        REQUIRE(imported.moveset->moves.size() == 6);
        const auto& hit = imported.moveset->moves.back().hits.front();
        CHECK(hit.event.sourceMotionCategory == special);
        CHECK(hit.behaviorParamId == 100000000 + variation * 1000 + 100);
        const auto restored = erdo::json::read<Moveset>(erdo::json::write(*imported.moveset));
        CHECK(erdo::json::write(restored) == erdo::json::write(*imported.moveset));
    }
}

TEST_CASE("behavior graph - game heavy effect routing", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable attacks, behaviors;
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    int checked = 0;
    for (int category : {20, 23}) {
        const auto catalog = category == 20 ? std::span<const MoveDefinition>(daggerCatalog)
                                           : std::span<const MoveDefinition>(straightSwordCatalog);
        const int variation = category == 20 ? 100 : 200;
        for (const auto& original : catalog) {
            if (!heavyAttackRoute(original.id, {})) continue;
            auto seed = original;
            seed.animationId = -1;
            const auto derived = graph.resolveHeavyMove(seed, std::array{category}, {});
            REQUIRE(derived.has_value());
            const auto actual = importMoveset(cache, category, variation, std::array{*derived}, index, attacks);
            const auto expected = importMoveset(cache, category, variation, std::array{original}, index, attacks);
            REQUIRE(actual.report.failures.empty());
            REQUIRE(expected.report.failures.empty());
            CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*expected.moveset));
            ++checked;
        }
    }
    CHECK(checked == 16);
    for (const auto& [effect, animation] : std::array{std::pair{1681, 30600}, std::pair{1811, 30620}}) {
        const auto definition = graph.resolveHeavyMove(
            {MoveId::ONE_HAND_CHARGED_R2_1, "Buffed heavy", -1}, std::array{23}, std::array{effect});
        REQUIRE(definition.has_value());
        CHECK(definition->animationId == animation);
        const auto imported = importMoveset(cache, 23, 200, std::array{*definition}, index, attacks);
        REQUIRE(imported.report.failures.empty());
        REQUIRE(imported.moveset->moves.size() == 1);
        const auto& hits = imported.moveset->moves.front().hits;
        const std::vector<int> judges = effect == 1681 ? std::vector{3950, 3951, 3952} : std::vector{3982, 3982, 3983, 3980};
        const std::vector<float> physical = effect == 1681 ? std::vector{82.f, 82.f, 86.f} : std::vector{0.f, 0.f, 0.f, 175.f};
        REQUIRE(hits.size() == judges.size());
        for (std::size_t i = 0; i < hits.size(); ++i) {
            CHECK(hits[i].event.behaviorJudgeId == judges[i]); // Preserve encoded source value.
            CHECK(hits[i].behaviorParamId == 300200000 + judges[i] % 1000);
            CHECK(hits[i].atkParamId == hits[i].behaviorParamId);
            CHECK(hits[i].motionValues.values[0] == physical[i]);
        }
        CHECK(erdo::json::write(erdo::json::read<Moveset>(erdo::json::write(*imported.moveset)))
              == erdo::json::write(*imported.moveset));
    }
}

TEST_CASE("behavior graph - game selectors resolve from named context", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable weapons, attacks, behaviors;
    weapons.load((root / EquipParamWeaponFile).string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    for (const auto [weaponId, category, variation] : {std::tuple{16020000, 36, 1600}, {16010000, 36, 1615},
        {16550000, 37, 1700}, {17060000, 37, 1708}, {5010000, 27, 500}, {6000000, 39, 602}}) {
        const auto baseline = resolveAttackEvents(cache.get(category, 30900), variation, index, attacks,
            {}, {}, BehaviorLookup::PLAYER_WEAPON);
        REQUIRE(baseline.diagnostics.empty());
        REQUIRE(baseline.hits.size() == (category == 37 ? 2 : 1));
        for (const auto [style, torch, animation] : {std::tuple{0, false, 30900}, {1, false, 30901},
            {2, false, 30902}, {2, true, 30903}, {3, false, 30904}}) {
            CAPTURE(weaponId, style, torch);
            const auto definition = graph.resolveWeaponShieldpoke({MoveId::SHIELDPOKE, "Guard", -1},
                weapons, weaponId, style, torch);
            REQUIRE(definition);
            CHECK(definition->motionCategory == category);
            CHECK(definition->animationId == animation);
            auto hits = resolveAttackEvents(cache.get(category, animation), variation, index, attacks,
                {}, {}, BehaviorLookup::PLAYER_WEAPON);
            REQUIRE(hits.diagnostics.empty());
            REQUIRE(hits.hits.size() == baseline.hits.size());
            // These authored guard branches have identical attack events and
            // resolved powers, while retaining their distinct selected clips.
            for (auto& hit : hits.hits) {
                hit.event.animationId = 30900;
                hit.event.sourceAnimationId = 30900;
            }
            CHECK(erdo::json::write(hits.hits) == erdo::json::write(baseline.hits));
        }
    }
    // Actual HKS entry for an offhand attack, now through its nested selector.
    const auto offhand = graph.inspect("AttackLeftHeavy1");
    REQUIRE(offhand.has_value());
    CHECK(offhand->animationId == 35000);
    CHECK(offhand->offsetType == 14);
    auto missing = graph.inspect("AttackRightHeavySpecial1Start");
    REQUIRE_FALSE(missing);
    CHECK_THAT(missing.error(), Catch::Matchers::ContainsSubstring("AttackRightHeavySpecialType"));
    for (const auto& [index, start, end] : std::array{
        std::tuple{0, 30600, 30605}, std::tuple{1, 30620, 30625}}) {
        const BehaviorVariables context{{"AttackRightHeavySpecialType", index}};
        const auto begin = graph.inspect("AttackRightHeavySpecial1Start", context);
        const auto finish = graph.inspect("AttackRightHeavySpecial1End", context);
        REQUIRE(begin.has_value());
        REQUIRE(finish.has_value());
        CHECK(begin->animationId == start);
        CHECK(finish->animationId == end);
        CHECK(begin->offsetType == 13);
    }
}

TEST_CASE("behavior graph - game mappings match catalog hits", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable attacks, behaviors;
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    std::size_t checked = 0;
    for (int category : {20, 23}) {
        const int variation = category == 20 ? 100 : 200;
        const auto catalog = category == 20 ? std::span<const MoveDefinition>(daggerCatalog)
                                           : std::span<const MoveDefinition>(straightSwordCatalog);
        for (const auto& original : catalog) {
            if (!directAttackState(original.id))
                continue; // Availability still comes from the validated catalogs.
            CAPTURE(category, original.name);
            auto seed = original;
            seed.animationId = -1;
            const auto definition = graph.resolveMove(seed, std::array{category});
            if (!definition) FAIL(definition.error());
            CHECK(definition->animationId == original.animationId);
            const auto derived = importMoveset(cache, category, variation, std::array{*definition}, index, attacks);
            const auto expected = importMoveset(cache, category, variation, std::array{original}, index, attacks);
            REQUIRE(derived.report.failures.empty());
            REQUIRE(expected.report.failures.empty());
            CHECK(erdo::json::write(*derived.moveset) == erdo::json::write(*expected.moveset));
            ++checked;
        }
    }
    CHECK(checked == 83);
    // The registered override supplies the heavy clip; its absent light clip
    // falls back to the caller's base category, independent of file enumeration.
    const MoveDefinition seed{MoveId::ONE_HAND_R2_1, "Heavy", -1};
    auto heavy = graph.resolve(seed, "AttackRightHeavy1End", 13, std::array{110, 23});
    REQUIRE(heavy.has_value());
    CHECK(heavy->motionCategory == 110);
    CHECK(heavy->animationId == 30505);
    auto light = graph.resolve(seed, "AttackRightLight1", 13, std::array{110, 23});
    REQUIRE(light.has_value());
    CHECK(light->motionCategory == 23);
}

TEST_CASE("behavior graph - game light chains need no catalog seeds", "[moveset][game_xml][behavior_graph]")
{
    BehaviorGraph graph;
    graph.load(ERDO_MOVESET_BEHAVIOR_XML);
    const std::filesystem::path root = ERDO_MOVESET_XML_ROOT;
    ParamTable weapons, attacks, behaviors;
    weapons.load((root / "regulation-bin/EquipParamWeapon.param.xml").string().c_str());
    attacks.load((root / AtkParam_PcFile).string().c_str());
    behaviors.load((root / BehaviorParam_PcFile).string().c_str());
    BehaviorIndex index;
    index.build(behaviors);
    AnimationCache cache(root);
    int checked = 0;
    for (int weaponId : {1000000, 2000000}) {
        auto row = weapons.find(weaponId);
        const int category = *weapons.integer(row, "wepmotionCategory");
        const int variation = *weapons.integer(row, "behaviorVariationId");
        const auto catalog = weaponId == 1000000 ? std::span<const MoveDefinition>(daggerCatalog)
                                                : std::span<const MoveDefinition>(straightSwordCatalog);
        for (auto first : {MoveId::ONE_HAND_R1_1, MoveId::TWO_HAND_R1_1, MoveId::POWERSTANCE_L1_1}) {
            const auto definitions = graph.resolveLightChain(weapons, weaponId, first, std::array{category});
            if (!definitions) FAIL(definitions.error());
            const auto actual = importMoveset(cache, category, variation, *definitions, index, attacks);
            REQUIRE(actual.report.failures.empty());
            std::vector<MoveDefinition> expectedDefinitions;
            // Catalog is only the independent regression oracle; no constraints,
            // IDs, counts or animation numbers are fed to the new resolver.
            const auto prefix = first == MoveId::ONE_HAND_R1_1 ? "1H R1 #"
                              : first == MoveId::TWO_HAND_R1_1 ? "2H R1 #" : "Powerstance L1 #";
            for (const auto& definition : catalog)
                if (definition.name.starts_with(prefix)) expectedDefinitions.push_back(definition);
            const auto expected = importMoveset(cache, category, variation, expectedDefinitions, index, attacks);
            REQUIRE(expected.report.failures.empty());
            CHECK(erdo::json::write(*actual.moveset) == erdo::json::write(*expected.moveset));
            checked += static_cast<int>(definitions->size());
        }
    }
    CHECK(checked == 30);
    // Both extra powerstance states exist for the six-step fist chain.
    for (auto id : {MoveId::POWERSTANCE_L1_5, MoveId::POWERSTANCE_L1_6}) {
        auto definition = graph.resolveMove({id, "Fist", -1}, std::array{42});
        REQUIRE(definition.has_value());
    }
}
#endif
