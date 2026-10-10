module;
#include <pugixml.hpp>
export module erdo:behavior_graph;
import std;
import :witchy;
import :calculator;

export namespace erdo::witchy
{
    // Ordinary movement request states from local HKS locomotion/EvasionCommonFunction
    // and ExecAttack's successful R2 guard-counter branch. This identifies the
    // selected attack; it does not simulate stamina, movement, guard windows or
    // temporary effect redirects. Rolling is distinct from the stealth branch.
    std::optional<std::pair<std::string_view, int>> movementAttackState(calculator::MoveId id)
    {
        using enum calculator::MoveId;
        switch (id) {
        case ONE_HAND_RUNNING_R1: return {{"AttackRightLightDash", 13}};
        case ONE_HAND_RUNNING_R2: return {{"AttackRightHeavyDash", 13}};
        case ONE_HAND_ROLLING_R1: return {{"AttackRightLightStep", 13}};
        case ONE_HAND_BACKSTEP_R1: return {{"AttackRightBackstep", 13}};
        case ONE_HAND_GUARD_COUNTER: return {{"AttackRightHeavyCounter", 13}};
        case TWO_HAND_RUNNING_R1: return {{"AttackBothDash", 16}};
        case TWO_HAND_RUNNING_R2: return {{"AttackBothHeavyDash", 16}};
        case TWO_HAND_ROLLING_R1: return {{"AttackBothLightStep", 16}};
        case TWO_HAND_BACKSTEP_R1: return {{"AttackBothBackstep", 16}};
        case TWO_HAND_GUARD_COUNTER: return {{"AttackBothHeavyCounter", 16}};
        default: return std::nullopt;
        }
    }

    // ExecAttack converts these movement requests to dual states after the
    // matching-pair request has been selected by GetAttackRequest.
    std::optional<std::pair<std::string_view, int>> powerstanceMovementState(calculator::MoveId id)
    {
        using enum calculator::MoveId;
        switch (id) {
        case POWERSTANCE_RUNNING_L1: return {{"AttackDualDash", 13}};
        case POWERSTANCE_ROLLING_L1: return {{"AttackDualRolling", 13}};
        case POWERSTANCE_BACKSTEP_L1: return {{"AttackDualBackStep", 13}};
        default: return std::nullopt;
        }
    }

    // Heavy Start callbacks enter these states when R2/evasion and the feint
    // window are active. This maps the requested phase, not input eligibility.
    std::optional<std::pair<std::string_view, int>> feintAttackState(calculator::MoveId id)
    {
        using enum calculator::MoveId;
        switch (id) {
        case ONE_HAND_CHARGED_R2_1_FEINT: return {{"AttackRightSpecial1", 13}};
        case ONE_HAND_CHARGED_R2_2_FEINT: return {{"AttackRightSpecial2", 13}};
        case TWO_HAND_CHARGED_R2_1_FEINT: return {{"AttackBothSpecial1", 16}};
        case TWO_HAND_CHARGED_R2_2_FEINT: return {{"AttackBothSpecial2", 16}};
        default: return std::nullopt;
        }
    }

    // ExecRideAttack and RideAttack_* callbacks: R/L is the side of the horse,
    // not a change of equipped weapon. Both use WeaponCategoryHandStyle (16).
    // Charging and charged release share the Start clip; their existing event
    // filters must survive graph resolution. Hard2 combo variants are separate.
    std::optional<std::pair<std::string_view, int>> mountedAttackState(calculator::MoveId id)
    {
        using enum calculator::MoveId;
        switch (id) {
        case MOUNTED_R1_1: return {{"RideAttack_R_Top", 16}};
        case MOUNTED_R1_2: return {{"RideAttack_R_Top02", 16}};
        case MOUNTED_R1_3: return {{"RideAttack_R_Top03", 16}};
        case MOUNTED_L1_1: return {{"RideAttack_L_Top", 16}};
        case MOUNTED_L1_2: return {{"RideAttack_L_Top02", 16}};
        case MOUNTED_L1_3: return {{"RideAttack_L_Top03", 16}};
        case MOUNTED_R2: return {{"RideAttack_R_Hard1_End", 16}};
        case MOUNTED_L2: return {{"RideAttack_L_Hard1_End", 16}};
        case MOUNTED_R2_CHARGING: case MOUNTED_CHARGED_R2: return {{"RideAttack_R_Hard1_Start", 16}};
        case MOUNTED_L2_CHARGING: case MOUNTED_CHARGED_L2: return {{"RideAttack_L_Hard1_Start", 16}};
        default: return std::nullopt;
        }
    }

    // Shared action vocabulary, not per-weapon animation IDs. These entry
    // states have been checked against the existing Dagger/Longsword catalogs.
    // Presence here does not assert combo length or HKS reachability.
    std::optional<std::pair<std::string_view, int>> directAttackState(calculator::MoveId id)
    {
        if (const auto movement = movementAttackState(id)) return movement;
        if (const auto movement = powerstanceMovementState(id)) return movement;
        using enum calculator::MoveId;
        switch (id) {
        case ONE_HAND_R1_1: return {{"AttackRightLight1", 13}};
        case ONE_HAND_R1_2: return {{"AttackRightLight2", 13}};
        case ONE_HAND_R1_3: return {{"AttackRightLight3", 13}};
        case ONE_HAND_R1_4: return {{"AttackRightLight4", 13}};
        case ONE_HAND_R1_5: return {{"AttackRightLight5", 13}};
        case ONE_HAND_R1_6: return {{"AttackRightLight6", 13}};
        case ONE_HAND_R2_1: return {{"AttackRightHeavy1End", 13}};
        case ONE_HAND_R2_2: return {{"AttackRightHeavy2End", 13}};
        case ONE_HAND_CHARGED_R2_1: return {{"AttackRightHeavy1Start", 13}};
        case ONE_HAND_CHARGED_R2_2: return {{"AttackRightHeavy2Start", 13}};
        case TWO_HAND_R1_1: return {{"AttackBothLight1", 16}};
        case TWO_HAND_R1_2: return {{"AttackBothLight2", 16}};
        case TWO_HAND_R1_3: return {{"AttackBothLight3", 16}};
        case TWO_HAND_R1_4: return {{"AttackBothLight4", 16}};
        case TWO_HAND_R1_5: return {{"AttackBothLight5", 16}};
        case TWO_HAND_R1_6: return {{"AttackBothLight6", 16}};
        case TWO_HAND_R2_1: return {{"AttackBothHeavy1End", 16}};
        case TWO_HAND_R2_2: return {{"AttackBothHeavy2End", 16}};
        case TWO_HAND_CHARGED_R2_1: return {{"AttackBothHeavy1Start", 16}};
        case TWO_HAND_CHARGED_R2_2: return {{"AttackBothHeavy2Start", 16}};
        case OFFHAND_R1_1: return {{"AttackLeftHeavy1", 14}};
        case OFFHAND_R1_2: return {{"AttackLeftHeavy2", 14}};
        case OFFHAND_R1_3: return {{"AttackLeftHeavy3", 14}};
        case OFFHAND_R1_4: return {{"AttackLeftHeavy4", 14}};
        case OFFHAND_R1_5: return {{"AttackLeftHeavy5", 14}};
        case OFFHAND_R1_6: return {{"AttackLeftHeavy6", 14}};
        case POWERSTANCE_L1_1: return {{"AttackDualLight1", 13}};
        case POWERSTANCE_L1_2: return {{"AttackDualLight2", 13}};
        case POWERSTANCE_L1_3: return {{"AttackDualLight3", 13}};
        case POWERSTANCE_L1_4: return {{"AttackDualLight4", 13}};
        case POWERSTANCE_L1_5: return {{"AttackDualLight5", 13}};
        case POWERSTANCE_L1_6: return {{"AttackDualLight6", 13}};
        default: return std::nullopt;
        }
    }

    // IsEnableDualWielding explicitly excludes these classes, even when the
    // separate combo-limit function describes a paired chain.
    bool excludesMatchingPowerstance(int motionCategory)
    {
        return motionCategory == 21 || motionCategory == 57;
    }

    // Translation of GetAttackMaxNumber/GetDualAttackMaxNumber from the local
    // c0000.hks; provenance and limits are recorded in docs/moveset-mapping.md.
    // This selects the ordinary uninterrupted melee chain, not every state that
    // can be reached with special effects, stance changes or another weapon.
    // motionCategory is wepmotionCategory, NOT calculator::Weapon::Type/wepType.
    std::expected<std::span<const MoveDefinition>, std::string> lightAttackChain(
        int motionCategory, int specialCategory, calculator::MoveId first)
    {
        using enum calculator::MoveId;
        static constexpr std::array oneHand{
            MoveDefinition{ONE_HAND_R1_1, "1H R1 #1", -1},
            MoveDefinition{ONE_HAND_R1_2, "1H R1 #2", -1},
            MoveDefinition{ONE_HAND_R1_3, "1H R1 #3", -1},
            MoveDefinition{ONE_HAND_R1_4, "1H R1 #4", -1},
            MoveDefinition{ONE_HAND_R1_5, "1H R1 #5", -1},
            MoveDefinition{ONE_HAND_R1_6, "1H R1 #6", -1}
        };
        static constexpr std::array twoHand{
            MoveDefinition{TWO_HAND_R1_1, "2H R1 #1", -1},
            MoveDefinition{TWO_HAND_R1_2, "2H R1 #2", -1},
            MoveDefinition{TWO_HAND_R1_3, "2H R1 #3", -1},
            MoveDefinition{TWO_HAND_R1_4, "2H R1 #4", -1},
            MoveDefinition{TWO_HAND_R1_5, "2H R1 #5", -1},
            MoveDefinition{TWO_HAND_R1_6, "2H R1 #6", -1}
        };
        static constexpr std::array offhand{
            MoveDefinition{OFFHAND_R1_1, "Offhand R1 #1", -1},
            MoveDefinition{OFFHAND_R1_2, "Offhand R1 #2", -1},
            MoveDefinition{OFFHAND_R1_3, "Offhand R1 #3", -1},
            MoveDefinition{OFFHAND_R1_4, "Offhand R1 #4", -1},
            MoveDefinition{OFFHAND_R1_5, "Offhand R1 #5", -1},
            MoveDefinition{OFFHAND_R1_6, "Offhand R1 #6", -1}
        };
        static constexpr std::array powerstance{
            MoveDefinition{POWERSTANCE_L1_1, "Powerstance L1 #1", -1},
            MoveDefinition{POWERSTANCE_L1_2, "Powerstance L1 #2", -1},
            MoveDefinition{POWERSTANCE_L1_3, "Powerstance L1 #3", -1},
            MoveDefinition{POWERSTANCE_L1_4, "Powerstance L1 #4", -1},
            MoveDefinition{POWERSTANCE_L1_5, "Powerstance L1 #5", -1},
            MoveDefinition{POWERSTANCE_L1_6, "Powerstance L1 #6", -1}
        };
        std::span<const MoveDefinition> chain;
        if (first == ONE_HAND_R1_1) chain = oneHand;
        else if (first == TWO_HAND_R1_1) chain = twoHand;
        else if (first == OFFHAND_R1_1) chain = offhand;
        else if (first == POWERSTANCE_L1_1) chain = powerstance;
        else return std::unexpected("Expected the first move of a 1H, 2H, offhand or powerstance chain");
        if (specialCategory < 0)
            return std::unexpected("Invalid special attack category");

        int count;
        switch (motionCategory) {
        case 26: case 31: case 37: // Colossal sword/weapon, great spear
            count = 3; break;
        case 21: case 25: case 32: case 34: case 35: case 36: case 38:
        case 40: case 43: case 50: case 56: case 57: case 61: case 62:
            count = 4; break;
        case 23: case 24: case 29: case 30: case 33: case 39: case 60:
            count = 5; break;
        case 20: case 22: case 27: case 28: case 42: case 53: case 55: case 58:
            count = 6; break;
        default:
            // HKS also gives limits for shields, catalysts and ranged weapons,
            // but their attack requests need different routing. Unknown future
            // categories must not inherit the script's default of one attack.
            return std::unexpected("Motion category requires unsupported attack routing");
        }
        if (first == POWERSTANCE_L1_1) {
            // Same-category pair only. IsEnableDualWielding explicitly excludes
            // dueling shields, despite GetDualAttackMaxNumber returning four.
            if (excludesMatchingPowerstance(motionCategory))
                return std::unexpected("Motion category does not support a matching powerstance pair");
            switch (motionCategory) {
            case 26: case 31: case 37: case 25: case 32: case 35: case 36:
            case 38: case 40: case 50: case 43: case 61:
                count = 3; break;
            case 22: case 42: case 53: case 55: case 58:
                count = 6; break;
            default: count = 4; break;
            }
        } else {
            // These overrides apply to ordinary chains only, before category
            // defaults. Both two-handed styles use the same limit in HKS.
            switch (specialCategory) {
            case 249: case 258: count = first == TWO_HAND_R1_1 ? 4 : 5; break;
            case 255: case 257: count = first == TWO_HAND_R1_1 ? 4 : 6; break;
            default: break;
            }
        }
        return chain.first(count);
    }

    // Ordinary selected-weapon policy: special registrations override base
    // registrations. This is deliberately separate from generic graph traversal;
    // it does not model body/common, powerstance, skill or effect-driven priority.
    // Evidence and the remaining native-engine limits are in moveset-mapping.md.
    std::expected<std::vector<int>, std::string> ordinaryWeaponCategories(
        const ParamTable& weapons, int weaponId, bool twoHanded)
    try {
        const auto row = weapons.find(weaponId);
        const auto category = weapons.integer(row, "wepmotionCategory");
        const auto special = weapons.integer(row, "spAtkcategory");
        const auto position = weapons.integer(row, twoHanded ? "wepmotionBothHandId" : "wepmotionOneHandId");
        const auto catalyst = weapons.integer(row, "isWeaponCatalyst");
        if (!category || !special || !position || !catalyst)
            return std::unexpected("Weapon row requires motion categories, stance position and catalyst flag");
        if (*category <= 0 || *category > 999 || *special < 0 || *special > 999)
            return std::unexpected("Weapon motion category is outside the supported TAE range");
        if (*position != ordinaryWeaponMotionPosition(*category, twoHanded) || *catalyst != 0)
            return std::unexpected("Weapon requires unsupported stance or catalyst routing");
        if (ordinaryWeaponMotionPosition(*category, false) != 0 && *special != 0)
            return std::unexpected("Special-category routing with this stance requires explicit context");
        const auto chain = lightAttackChain(*category, *special, twoHanded
            ? calculator::MoveId::TWO_HAND_R1_1 : calculator::MoveId::ONE_HAND_R1_1);
        if (!chain) return std::unexpected(chain.error());
        std::vector<int> categories;
        if (*special != 0 && *special != *category) categories.push_back(*special);
        categories.push_back(*category);
        return categories;
    }
    catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

    // No inferred special/base ordering for left/dual contexts yet. With a
    // zero special category there is only the selected weapon's base category.
    // Offhand assumes the request already selected an attacking left weapon;
    // powerstance assumes two copies, not arbitrary mixed-weapon compatibility.
    std::expected<std::vector<int>, std::string> offhandOrMatchingPairCategories(
        const ParamTable& weapons, int weaponId, bool powerstance)
    try {
        const auto categories = ordinaryWeaponCategories(weapons, weaponId, false);
        if (!categories) return std::unexpected(categories.error());
        const auto row = weapons.find(weaponId);
        const int special = *weapons.integer(row, "spAtkcategory");
        if (special != 0)
            return std::unexpected("Special-category offhand/powerstance priority requires explicit context");
        if (powerstance) {
            const auto chain = lightAttackChain(*weapons.integer(row, "wepmotionCategory"), special,
                calculator::MoveId::POWERSTANCE_L1_1);
            if (!chain) return std::unexpected(chain.error());
        }
        return categories;
    }
    catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

    // Authored ThrowParam selector fields, not an enemy-size classification.
    // Character ID is the model ID (e.g. c2010), not NpcParam's row ID.
    // Position, angle, collision and runtime eligibility are intentionally absent.
    struct PlayerThrowQuery {
        int defenderCharacterId;
        int throwType;
        int throwTypeId = 0;
        int attackerEnableState = 0;
    };

    std::expected<std::vector<int>, std::string> playerThrowCandidates(
        const ParamTable& throws, PlayerThrowQuery query) try
    {
        if (query.defenderCharacterId < 0 || query.throwType < 0 || query.throwType > 255
            || query.throwTypeId < 0 || query.attackerEnableState < 0 || query.attackerEnableState > 255)
            return std::unexpected("Invalid player throw query");
        std::vector<int> result;
        for (const auto& [id, row] : throws.rows) {
            auto value = [&](const char* field) {
                const auto found = throws.integer(row, field);
                if (!found) throw std::runtime_error(std::format("Throw row {} requires {}", id, field));
                return *found;
            };
            if (value("AtkChrId") != 0 || value("DefChrId") != query.defenderCharacterId
                || value("throwType") != query.throwType || value("throwTypeId") != query.throwTypeId
                || value("AtkEnableState") != query.attackerEnableState) continue;
            if (id < 0) return std::unexpected("Invalid matching throw row ID");
            // Keep every match, including unsupported rows. Silently removing
            // one could turn ambiguous/unsupported routing into false certainty.
            result.push_back(id);
        }
        std::ranges::sort(result);
        if (result.empty()) return std::unexpected("No throw rows match the target and interaction context");
        return result;
    }
    catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

    // Graph metadata, not a replacement for calculator::Move/ResolvedHit.
    struct BehaviorClip {
        int motionCategory;
        int animationId;
        std::string animationName;
    };

    struct BehaviorSelector {
        std::string stateName;
        int offsetType;
        int animationId;
        std::vector<BehaviorClip> clips;
    };

    // A snapshot of explicitly known HKS variable values. Bound selectors never
    // use serialized initial values as a substitute for runtime state.
    using BehaviorVariables = std::map<std::string, int, std::less<>>;

    struct JumpAttackRoute {
        std::string_view stateName;
        int offsetType;
        BehaviorVariables variables;
    };

    // Local HKS JumpCommonFunction: ordinary right-hand / right-two-handed
    // melee or matching powerstance, already airborne. Landing is a separate
    // runtime transition whose clip startTime binding must not be flattened.
    std::optional<JumpAttackRoute> jumpingAttackRoute(calculator::MoveId id)
    {
        using enum calculator::MoveId;
        int hand, request;
        switch (id) {
        case ONE_HAND_JUMPING_R1: hand = 0; request = 0; break;
        case ONE_HAND_JUMPING_R2: hand = 0; request = 1; break;
        case TWO_HAND_JUMPING_R1: hand = 1; request = 0; break;
        case TWO_HAND_JUMPING_R2: hand = 1; request = 1; break;
        case POWERSTANCE_JUMPING_L1: hand = 0; request = 2; break;
        default: return std::nullopt;
        }
        return JumpAttackRoute{"JumpAttack_Start_Falling", hand == 1 ? 16 : 13,
            {{"JumpAttack_HandCondition", hand}, {"JumpAttackFormRequest", request}, {"JumpAttack_Land", 0}}};
    }

    struct HeavyAttackRoute {
        std::string stateName;
        int offsetType;
        BehaviorVariables variables;
    };

    // The ordinary R2 branch of local HKS ExecAttack, followed to its charged
    // Start or released End state. Effects are an explicit active snapshot,
    // not inferred from equipped skills or weapon passive-effect parameters.
    // This does not evaluate input eligibility, charge timing, combo sub-starts,
    // guard counters, or buffs' damage modifiers.
    std::expected<HeavyAttackRoute, std::string> heavyAttackRoute(
        calculator::MoveId id, std::span<const int> activeEffects, bool twoHandingLeft = false)
    {
        using enum calculator::MoveId;
        bool twoHanded, charged;
        int stage;
        switch (id) {
        case ONE_HAND_R2_1: twoHanded = false; charged = false; stage = 1; break;
        case ONE_HAND_R2_2: twoHanded = false; charged = false; stage = 2; break;
        case ONE_HAND_CHARGED_R2_1: twoHanded = false; charged = true; stage = 1; break;
        case ONE_HAND_CHARGED_R2_2: twoHanded = false; charged = true; stage = 2; break;
        case TWO_HAND_R2_1: twoHanded = true; charged = false; stage = 1; break;
        case TWO_HAND_R2_2: twoHanded = true; charged = false; stage = 2; break;
        case TWO_HAND_CHARGED_R2_1: twoHanded = true; charged = true; stage = 1; break;
        case TWO_HAND_CHARGED_R2_2: twoHanded = true; charged = true; stage = 2; break;
        default: return std::unexpected("Expected an ordinary 1H/2H heavy MoveId");
        }
        if (twoHandingLeft && !twoHanded)
            return std::unexpected("Left-hand two-handing requires a 2H heavy move");
        if (std::ranges::any_of(activeEffects, [](int effect) { return effect < 0; }))
            return std::unexpected("Active effect IDs must be nonnegative");
        auto has = [&](int effect) { return std::ranges::contains(activeEffects, effect); };
        // Guard/counter entry depends on further input/node context, so this
        // ordinary-heavy API cannot safely choose it from effects alone.
        if (has(173) || has(174) || has(102020))
            return std::unexpected("Guard-counter effects require additional attack context");
        std::optional<int> variant;
        // HKS uses independent ifs: later matches overwrite earlier values.
        // Right-hand IDs in its final 19912 test remain right-hand IDs even
        // when two-handing the left weapon.
        if (has(twoHandingLeft ? 1683 : 1681) || has(twoHandingLeft ? 1688 : 1686)) variant = 0;
        if (has(twoHandingLeft ? 1813 : 1811) || has(twoHandingLeft ? 1818 : 1816)
            || (twoHanded && has(twoHandingLeft ? 1863 : 1861))) variant = 1;
        if (has(twoHandingLeft ? 1718 : 1716)) variant = 0;
        if (has(twoHandingLeft ? 1723 : 1721)) variant = 0;
        if (has(twoHandingLeft ? 102106 : 102101)) variant = 0;
        bool special = variant.has_value();
        if (!has(1681) && !has(1686) && !has(1811) && !has(1816) && has(19912)) {
            special = false;
            stage = 2;
        }
        HeavyAttackRoute route{
            std::format("Attack{}Heavy{}{}{}", twoHanded ? "Both" : "Right",
                special ? "Special" : "", stage, charged ? "Start" : "End"),
            twoHanded ? 16 : 13, {}};
        if (special) route.variables.emplace("AttackRightHeavySpecialType", *variant);
        return route;
    }

    // Reads HKLib's Havok 2018 XML. Traverses manual selectors to a CMSG and its
    // clips. Script generators/blenders and other executable paths still reject.
    class BehaviorGraph {
        pugi::xml_document document_;
        std::map<std::string, std::string, std::less<>> types_;
        std::map<std::string, pugi::xml_node, std::less<>> objects_;
        std::map<std::string, std::vector<pugi::xml_node>, std::less<>> states_;

        static pugi::xml_node field(pugi::xml_node object, const char* name)
        {
            const auto record = std::string_view(object.name()) == "record" ? object : object.child("record");
            const auto value = record.find_child_by_attribute("field", "name", name);
            if (!value)
                throw std::runtime_error(std::format("Missing graph field {} in {}", name, object.attribute("id").value()));
            return value;
        }

        static std::string string(pugi::xml_node object, const char* name)
        {
            const auto value = field(object, name).child("string").attribute("value");
            if (!value)
                throw std::runtime_error(std::format("Invalid graph string {}", name));
            return value.value();
        }

        static int number(std::string_view value)
        {
            int result;
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
            if (value.empty() || error != std::errc{} || end != value.data() + value.size())
                throw std::runtime_error("Invalid graph integer: " + std::string(value));
            return result;
        }

        static int integer(pugi::xml_node object, const char* name)
        {
            return number(field(object, name).child("integer").attribute("value").value());
        }

        static double real(pugi::xml_node object, const char* name)
        {
            const std::string_view value = field(object, name).child("real").attribute("dec").value();
            double result;
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
            if (error != std::errc{} || end != value.data() + value.size() || !std::isfinite(result))
                throw std::runtime_error("Invalid graph real " + std::string(name));
            return result;
        }

        void require_type(pugi::xml_node object, std::string_view expected) const
        {
            const auto type = types_.find(object.attribute("typeid").value());
            if (type == types_.end() || type->second != expected)
                throw std::runtime_error("Unsupported graph object type; expected " + std::string(expected));
        }

        pugi::xml_node object(std::string_view id) const
        {
            const auto found = objects_.find(id);
            if (found == objects_.end())
                throw std::runtime_error("Missing graph object " + std::string(id));
            return found->second;
        }

        static void require_unbound(pugi::xml_node node)
        {
            const auto pointer = field(node, "variableBindingSet").child("pointer").attribute("id");
            if (!pointer || std::string_view(pointer.value()) != "object0")
                throw std::runtime_error("Dynamic graph variable bindings are not supported on this path");
        }

        std::string variable_name(int index) const
        {
            pugi::xml_node graph;
            for (const auto& [id, node] : objects_) {
                if (types_.at(node.attribute("typeid").value()) != "hkbBehaviorGraph") continue;
                if (graph) throw std::runtime_error("Multiple behavior graphs require scoped variable lookup");
                graph = node;
            }
            if (!graph) throw std::runtime_error("Missing behavior graph for variable lookup");
            const auto data = object(field(graph, "data").child("pointer").attribute("id").value());
            require_type(data, "hkbBehaviorGraphData");
            const auto strings = object(field(data, "stringData").child("pointer").attribute("id").value());
            require_type(strings, "hkbBehaviorGraphStringData");
            const auto names = field(strings, "variableNames").child("array");
            const auto count = std::ranges::distance(names.children("string"));
            if (count != number(names.attribute("count").value()) || index < 0 || index >= count)
                throw std::runtime_error("Invalid graph variable index or name array");
            auto name = names.children("string").begin();
            std::advance(name, index);
            const std::string result = name->attribute("value").value();
            if (result.empty()) throw std::runtime_error("Empty graph variable name");
            int matches = 0;
            for (const auto value : names.children("string"))
                if (std::string_view(value.attribute("value").value()) == result) ++matches;
            if (matches != 1) throw std::runtime_error("Ambiguous graph variable name " + result);
            return result;
        }

        int bound_integer(pugi::xml_node selector, const char* member, const BehaviorVariables& variables) const
        {
            const auto binding = field(selector, "variableBindingSet").child("pointer").attribute("id");
            if (!binding) throw std::runtime_error("Missing selector binding reference");
            const auto set = object(binding.value());
            require_type(set, "hkbVariableBindingSet");
            if (integer(set, "indexOfBindingToEnable") != -1)
                throw std::runtime_error("Selector enable binding is unsupported");
            const auto bindings = field(set, "bindings").child("array");
            if (number(bindings.attribute("count").value()) != 1
                || std::ranges::distance(bindings.children("record")) != 1)
                throw std::runtime_error("Expected one selector index binding");
            const auto value = bindings.child("record");
            if (string(value, "memberPath") != member
                || integer(value, "bindingType") != 0 || integer(value, "bitIndex") != -1)
                throw std::runtime_error("Unsupported selector variable binding");
            const auto name = variable_name(integer(value, "variableIndex"));
            const auto found = variables.find(name);
            if (found == variables.end())
                throw std::runtime_error("Missing behavior variable " + name);
            return found->second;
        }

        int selected_index(pugi::xml_node selector, const BehaviorVariables& variables) const
        {
            const auto binding = field(selector, "variableBindingSet").child("pointer").attribute("id");
            if (!binding) throw std::runtime_error("Missing selector binding reference");
            if (std::string_view(binding.value()) == "object0") return integer(selector, "selectedGeneratorIndex");
            return bound_integer(selector, "selectedGeneratorIndex", variables);
        }

        pugi::xml_node exact_blend_child(pugi::xml_node generator, const BehaviorVariables& variables) const
        {
            // FLAG_PARAMETRIC_BLEND only: select an exact authored sample, never
            // interpolate, clamp, synchronize, or merge events from two clips.
            if (integer(generator, "flags") != 16 || integer(generator, "indexOfSyncMasterChild") != -1
                || std::string_view(field(generator, "subtractLastChild").child("bool").attribute("value").value()) != "false"
                || real(generator, "referencePoseWeightThreshold") != 0)
                throw std::runtime_error("Unsupported parametric blender configuration");
            const int selected = bound_integer(generator, "blendParameter", variables);
            const auto children = field(generator, "children").child("array");
            if (std::ranges::distance(children.children("pointer")) != number(children.attribute("count").value()))
                throw std::runtime_error("Blender child array count mismatch");
            pugi::xml_node match;
            double previous = -std::numeric_limits<double>::infinity();
            for (const auto ref : children.children("pointer")) {
                const auto child = object(ref.attribute("id").value());
                require_type(child, "hkbBlenderGeneratorChild");
                require_unbound(child);
                if (std::string_view(field(child, "boneWeights").child("pointer").attribute("id").value()) != "object0"
                    || real(child, "worldFromModelWeight") != 1)
                    throw std::runtime_error("Unsupported blender child weights");
                const double weight = real(child, "weight");
                if (weight <= previous) throw std::runtime_error("Blender samples must be strictly increasing");
                previous = weight;
                if (weight == selected) match = object(field(child, "generator").child("pointer").attribute("id").value());
            }
            if (!match) throw std::runtime_error("No exact parametric blender sample for the supplied context");
            return match;
        }

        pugi::xml_node select_generator(pugi::xml_node generator, const BehaviorVariables& variables) const
        {
            std::set<std::string> visited;
            for (;;) {
                if (!visited.emplace(generator.attribute("id").value()).second || visited.size() > 128)
                    throw std::runtime_error("Cyclic or excessively deep behavior selector path");
                const auto& type = types_.at(generator.attribute("typeid").value());
                if (type == "CustomManualSelectorGenerator") return generator;
                if (type == "hkbBlenderGenerator") {
                    generator = exact_blend_child(generator, variables);
                    continue;
                }
                if (type == "hkbScriptGenerator") {
                    require_unbound(generator);
                    // Audited local callback establishes the attack/guard hand,
                    // but does not select another animation. No generic HKS execution.
                    const auto activation = string(generator, "onActivateScript");
                    if (activation != "ThrowBackStab_Activate()" && activation != "AttackRightWhileGuard_Activate()")
                        throw std::runtime_error("Unsupported graph script callback");
                    for (const auto* member : {"onPreUpdateScript", "onGenerateScript", "onHandleEventScript", "onDeactivateScript"})
                        if (!string(generator, member).empty()) throw std::runtime_error("Unsupported graph script callback");
                    generator = object(field(generator, "child").child("pointer").attribute("id").value());
                    continue;
                }
                require_type(generator, "hkbManualSelectorGenerator");
                const auto indexSelector = field(generator, "indexSelector").child("pointer").attribute("id");
                if (!indexSelector || std::string_view(indexSelector.value()) != "object0")
                    throw std::runtime_error("Unsupported manual selector indexSelector");
                const auto transition = field(generator, "generatorChangedTransitionEffect").child("pointer").attribute("id");
                if (!transition) throw std::runtime_error("Missing selector transition reference");
                if (std::string_view(transition.value()) != "object0") {
                    const auto effect = object(transition.value());
                    require_type(effect, "CustomTransitionEffect");
                    require_unbound(effect);
                    // Only an instantaneous, unshifted selection is supported.
                    // This is not an evaluator for blending or transition timing.
                    for (const auto* name : {"duration", "toGeneratorStartTimeFraction"}) {
                        const std::string_view value = field(effect, name).child("real").attribute("dec").value();
                        double parsed;
                        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
                        if (error != std::errc{} || end != value.data() + value.size() || parsed != 0)
                            throw std::runtime_error("Unsupported selector transition timing");
                    }
                    for (const auto* name : {"selfTransitionMode", "eventMode", "flags", "endMode", "blendCurve"})
                        if (integer(effect, name) != 0)
                            throw std::runtime_error("Unsupported selector transition mode");
                    if (integer(effect, "alignmentBone") != -1)
                        throw std::runtime_error("Unsupported selector transition alignment");
                }
                const auto children = field(generator, "generators").child("array");
                const auto count = std::ranges::distance(children.children("pointer"));
                const int selected = selected_index(generator, variables);
                if (count != number(children.attribute("count").value()) || selected < 0 || selected >= count)
                    throw std::runtime_error("Manual selector index out of range or array count mismatch");
                auto child = children.children("pointer").begin();
                std::advance(child, selected);
                generator = object(child->attribute("id").value());
            }
        }

        void index()
        {
            types_.clear();
            objects_.clear();
            states_.clear();
            const auto root = document_.child("hktagfile");
            if (!root || std::string_view(root.attribute("version").value()) != "3")
                throw std::runtime_error("Expected HKLib hktagfile version 3 XML");
            for (auto type : root.children("type")) {
                const std::string id = type.attribute("id").value();
                if (id.empty() || !types_.emplace(id, type.child("name").attribute("value").value()).second)
                    throw std::runtime_error("Missing or duplicate graph type ID");
            }
            for (auto node : root.children("object")) {
                const std::string id = node.attribute("id").value();
                if (id.empty() || !objects_.emplace(id, node).second)
                    throw std::runtime_error("Missing or duplicate graph object ID");
                const auto type = types_.find(node.attribute("typeid").value());
                if (type == types_.end())
                    throw std::runtime_error("Missing graph type definition");
                if (type->second == "hkbStateMachine::StateInfo")
                    states_[string(node, "name")].push_back(node);
            }
        }

    public:
        void load(const std::filesystem::path& path)
        {
            types_.clear(); objects_.clear(); states_.clear();
            if (const auto result = document_.load_file(path.string().c_str()); !result)
                throw std::runtime_error(std::format("Cannot load behavior graph {}: {}", path, result.description()));
            index();
        }

        void loadXml(std::string_view xml)
        {
            types_.clear(); objects_.clear(); states_.clear();
            if (const auto result = document_.load_buffer(xml.data(), xml.size()); !result)
                throw std::runtime_error(std::format("Cannot parse behavior graph: {}", result.description()));
            index();
        }

        std::expected<BehaviorSelector, std::string> inspect(
            std::string_view stateName, const BehaviorVariables& variables = {}) const try
        {
            const auto found = states_.find(stateName);
            if (found == states_.end())
                throw std::runtime_error("Missing behavior state " + std::string(stateName));
            if (found->second.size() != 1)
                throw std::runtime_error("Ambiguous behavior state " + std::string(stateName));
            const auto state = found->second.front();
            require_unbound(state);
            if (std::string_view(field(state, "enable").child("bool").attribute("value").value()) != "true")
                throw std::runtime_error("Behavior state is disabled");
            const auto selector = select_generator(
                object(field(state, "generator").child("pointer").attribute("id").value()), variables);
            require_type(selector, "CustomManualSelectorGenerator");
            require_unbound(selector);
            if (std::string_view(field(selector, "enableTae").child("bool").attribute("value").value()) != "true")
                throw std::runtime_error("Behavior selector disables TAE events");
            BehaviorSelector result{std::string(stateName), integer(selector, "offsetType"), integer(selector, "animId"), {}};
            const auto generators = field(selector, "generators").child("array");
            if (!generators)
                throw std::runtime_error("Missing clip generator array");
            for (auto ref : generators.children("pointer")) {
                const auto clip = object(ref.attribute("id").value());
                require_type(clip, "hkbClipGenerator");
                require_unbound(clip);
                const auto name = string(clip, "animationName");
                if (name.size() != 11 || name[0] != 'a' || name[4] != '_')
                    throw std::runtime_error("Unsupported animation name " + name);
                const int category = number(std::string_view(name).substr(1, 3));
                const int animation = number(std::string_view(name).substr(5, 6));
                if (category < 0 || animation != result.animationId)
                    throw std::runtime_error("Clip animation does not match selector animation");
                result.clips.push_back({category, animation, name});
            }
            if (std::cmp_not_equal(result.clips.size(), number(generators.attribute("count").value())))
                throw std::runtime_error("Clip generator array count mismatch");
            if (result.clips.empty())
                throw std::runtime_error("Behavior selector has no clips");
            return result;
        }
        catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

        // Priority is explicit: this layer does not pretend to evaluate HKS or
        // native engine rules. The caller supplies a verified context and offset.
        // Fall back only when a category is absent from the selector, never when
        // a selected clip's TAE is missing or has unresolvable attack events.
        std::expected<MoveDefinition, std::string> resolve(
            MoveDefinition definition, std::string_view stateName, int expectedOffsetType,
            std::span<const int> categoryPriority, const BehaviorVariables& variables = {}) const
        {
            const auto selector = inspect(stateName, variables);
            if (!selector)
                return std::unexpected(selector.error());
            if (selector->offsetType != expectedOffsetType)
                return std::unexpected("Behavior selector offset does not match the requested context");
            for (const int category : categoryPriority) {
                const BehaviorClip* match = nullptr;
                for (const auto& clip : selector->clips) {
                    if (clip.motionCategory != category)
                        continue;
                    if (match)
                        return std::unexpected("Multiple clip generators match the requested category");
                    match = &clip;
                }
                if (match) {
                    definition.animationId = match->animationId;
                    definition.motionCategory = match->motionCategory;
                    return definition;
                }
            }
            return std::unexpected("No clip matches the requested motion categories");
        }

        std::expected<MoveDefinition, std::string> resolveMove(
            MoveDefinition definition, std::span<const int> categoryPriority,
            const BehaviorVariables& variables = {}) const
        {
            const auto state = directAttackState(definition.id);
            if (!state)
                return std::unexpected("Move requires a behavior path not yet supported");
            return resolve(definition, state->first, state->second, categoryPriority, variables);
        }

        std::expected<MoveDefinition, std::string> resolveHeavyMove(
            MoveDefinition definition, std::span<const int> categoryPriority,
            std::span<const int> activeEffects, bool twoHandingLeft = false) const
        {
            const auto route = heavyAttackRoute(definition.id, activeEffects, twoHandingLeft);
            if (!route) return std::unexpected(route.error());
            return resolve(definition, route->stateName, route->offsetType, categoryPriority, route->variables);
        }

        // Neutral heavy snapshot; no temporary effects, offhand or paired style.
        // This resolves a requested stage, not its input/transition eligibility.
        std::expected<MoveDefinition, std::string> resolveWeaponHeavyMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto route = heavyAttackRoute(definition.id, {});
            if (!route) return std::unexpected(route.error());
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, route->offsetType == 16);
            if (!categories) return std::unexpected(categories.error());
            return resolve(definition, route->stateName, route->offsetType, *categories, route->variables);
        }

        // Neutral selected-weapon feint for audited thrusting, curved and great katanas.
        // Effects, specials and left-two-handing need context.
        std::expected<MoveDefinition, std::string> resolveWeaponFeintMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto state = feintAttackState(definition.id);
            if (!state) return std::unexpected("Expected a charged-heavy feint MoveId");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, state->second == 16);
            if (!categories) return std::unexpected(categories.error());
            const auto row = weapons.find(weaponId);
            if (weapons.integer(row, "spAtkcategory") != 0)
                return std::unexpected("Special-category feint priority requires explicit context");
            const int category = categories->front();
            if (category != 27 && category != 28 && category != 39 && category != 61)
                return std::unexpected("Motion category requires unsupported feint routing");
            return resolve(definition, state->first, state->second, *categories);
        }

        std::expected<std::vector<MoveDefinition>, std::string> resolveLightChain(
            const ParamTable& weapons, int weaponId, calculator::MoveId first) const
        {
            using enum calculator::MoveId;
            if (first != ONE_HAND_R1_1 && first != TWO_HAND_R1_1 && first != OFFHAND_R1_1 && first != POWERSTANCE_L1_1)
                return std::unexpected("Expected the first move of a supported chain");
            const auto categories = first == OFFHAND_R1_1 || first == POWERSTANCE_L1_1
                ? offhandOrMatchingPairCategories(weapons, weaponId, first == POWERSTANCE_L1_1)
                : ordinaryWeaponCategories(weapons, weaponId, first == TWO_HAND_R1_1);
            if (!categories) return std::unexpected(categories.error());
            return resolveLightChain(weapons, weaponId, first, *categories);
        }

        std::expected<MoveDefinition, std::string> resolveWeaponMovementMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto state = movementAttackState(definition.id);
            if (!state) return std::unexpected("Expected an ordinary 1H/2H movement or guard-counter MoveId");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, state->second == 16);
            if (!categories) return std::unexpected(categories.error());
            return resolve(definition, state->first, state->second, *categories);
        }

        std::expected<MoveDefinition, std::string> resolveMatchingPowerstanceMovementMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto state = powerstanceMovementState(definition.id);
            if (!state) return std::unexpected("Expected a powerstance running/rolling/backstep MoveId");
            const auto categories = offhandOrMatchingPairCategories(weapons, weaponId, true);
            if (!categories) return std::unexpected(categories.error());
            return resolve(definition, state->first, state->second, *categories);
        }

        std::expected<MoveDefinition, std::string> resolveWeaponJumpingMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto route = jumpingAttackRoute(definition.id);
            if (!route) return std::unexpected("Expected an airborne jumping MoveId");
            const auto categories = definition.id == calculator::MoveId::POWERSTANCE_JUMPING_L1
                ? offhandOrMatchingPairCategories(weapons, weaponId, true)
                : ordinaryWeaponCategories(weapons, weaponId, route->offsetType == 16);
            if (!categories) return std::unexpected(categories.error());
            // HKS redirects this special category's R2 to jump magic.
            if (weapons.integer(weapons.find(weaponId), "spAtkcategory") == 293
                && route->variables.at("JumpAttackFormRequest") == 1)
                return std::unexpected("Jumping R2 requires magic context for this special category");
            return resolve(definition, route->stateName, route->offsetType, *categories, route->variables);
        }

        // An explicitly selected player ThrowParam row. Selection by target,
        // position, throw type and weapon remains the caller's responsibility;
        // the supplied MoveId/name is a label, not inferred from target size.
        std::expected<MoveDefinition, std::string> resolvePlayerThrow(
            MoveDefinition definition, const ParamTable& throws, int rowId,
            std::span<const int> categoryPriority) const try
        {
            if (rowId < 0) return std::unexpected("Invalid throw row ID");
            if (definition.eventType && *definition.eventType != 304)
                return std::unexpected("Player throw hits require TAE event type 304");
            definition.eventType = 304;
            const auto row = throws.find(rowId);
            const auto attacker = throws.integer(row, "AtkChrId");
            const auto defender = throws.integer(row, "DefChrId");
            const auto skipCategory = throws.integer(row, "isSkipWepCate");
            const auto typeId = throws.integer(row, "throwTypeId");
            const auto animation = throws.integer(row, "atkAnimId");
            if (!attacker || !defender || !skipCategory || !typeId || !animation)
                return std::unexpected("Throw row requires character IDs, weapon-category policy, type ID and attack animation");
            if (*attacker != 0 || *defender < 0 || *skipCategory != 0 || *typeId != 0 || *animation < 0)
                return std::unexpected("Unsupported player weapon throw context");
            const auto result = resolve(definition, "ThrowAtk", 16, categoryPriority, {{"ThrowID", *animation}});
            if (!result) return result;
            if (result->animationId != *animation)
                return std::unexpected("Throw graph selected a substitute animation");
            return result;
        }
        catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

        // Resolve hit data only when every candidate interaction selects the
        // same supported clip. This does not pick a geometrically valid row or
        // establish that a throw can currently start. MoveId remains a label.
        std::expected<MoveDefinition, std::string> resolveWeaponThrow(
            MoveDefinition definition, const ParamTable& weapons, int weaponId,
            const ParamTable& throws, PlayerThrowQuery query) const try
        {
            const auto enabled = weapons.integer(weapons.find(weaponId), "enableThrow");
            if (!enabled || *enabled != 1)
                return std::unexpected("Weapon requires enableThrow=1");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, true);
            if (!categories) return std::unexpected(categories.error());
            if (weapons.integer(weapons.find(weaponId), "spAtkcategory") != 0)
                return std::unexpected("Special-category throw priority requires explicit context");
            const auto candidates = playerThrowCandidates(throws, query);
            if (!candidates) return std::unexpected(candidates.error());
            std::optional<MoveDefinition> result;
            for (const int row : *candidates) {
                const auto resolved = resolvePlayerThrow(definition, throws, row, *categories);
                if (!resolved) return std::unexpected(std::format("Throw row {}: {}", row, resolved.error()));
                if (result && (result->animationId != resolved->animationId || result->motionCategory != resolved->motionCategory))
                    return std::unexpected(std::format("Ambiguous throw animations for target {}: rows {} and {}",
                        query.defenderCharacterId, candidates->front(), row));
                result = *resolved;
            }
            return *result;
        }
        catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

        std::expected<MoveDefinition, std::string> resolveWeaponBackstabWhiff(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            if (definition.id != calculator::MoveId::BACKSTAB_WHIFF || !definition.eventJudgeId)
                return std::unexpected("Backstab whiff requires its MoveId and explicit strike phase filter");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, true);
            if (!categories) return std::unexpected(categories.error());
            if (weapons.integer(weapons.find(weaponId), "spAtkcategory") != 0)
                return std::unexpected("Special-category backstab priority requires explicit context");
            return resolve(definition, "ThrowBackStab", 16, *categories);
        }

        // GetAttackRequest selects the right-hand guard attack; ExecAttack sets
        // IndexAttackWhileGuard from left guard equipment (torch/dueling overrides).
        // Caller supplies that context. This does not prove guard eligibility.
        std::expected<MoveDefinition, std::string> resolveWeaponShieldpoke(
            MoveDefinition definition, const ParamTable& weapons, int weaponId,
            int guardStyleIndex, bool torchGuard) const
        {
            if (definition.id != calculator::MoveId::SHIELDPOKE)
                return std::unexpected("Expected shieldpoke MoveId");
            if (guardStyleIndex < 0 || guardStyleIndex > 3 || (torchGuard && guardStyleIndex != 2))
                return std::unexpected("Invalid shieldpoke guard context");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, false);
            if (!categories) return std::unexpected(categories.error());
            const auto row = weapons.find(weaponId);
            if (weapons.integer(row, "spAtkcategory") != 0)
                return std::unexpected("Special-category shieldpoke priority requires explicit context");
            const int category = categories->front();
            if (category != 27 && category != 36 && category != 37 && category != 39)
                return std::unexpected("Weapon category does not select shieldpoke requests");
            return resolve(definition, "AttackRightWhileGuard", 13, *categories,
                {{"IndexAttackWhileGuard", guardStyleIndex}, {"IsAttackWhileTorchGuard", torchGuard ? 1 : 0}});
        }

        // Selected right-hand melee weapon, on the ground while mounted.
        // Special mounted category priority and left-equipped style need their
        // own context. No full input/charge/jump execution is inferred here.
        std::expected<MoveDefinition, std::string> resolveWeaponMountedMove(
            MoveDefinition definition, const ParamTable& weapons, int weaponId) const
        {
            const auto state = mountedAttackState(definition.id);
            if (!state) return std::unexpected("Expected a mounted MoveId");
            using enum calculator::MoveId;
            if ((definition.id == MOUNTED_R2_CHARGING || definition.id == MOUNTED_CHARGED_R2
                || definition.id == MOUNTED_L2_CHARGING || definition.id == MOUNTED_CHARGED_L2)
                && !definition.eventJudgeId)
                return std::unexpected("Mounted charging/release requires an explicit event phase filter");
            const auto categories = ordinaryWeaponCategories(weapons, weaponId, true);
            if (!categories) return std::unexpected(categories.error());
            if (weapons.integer(weapons.find(weaponId), "spAtkcategory") != 0)
                return std::unexpected("Special-category mounted priority requires explicit context");
            return resolve(definition, state->first, state->second, *categories);
        }

        // No catalog seeds or animation arithmetic: the parameter row supplies
        // chain limits, and each named graph state supplies its clip. Category
        // priority is still explicit until native precedence is established.
        // Powerstance assumes a matching pair; conditional routing is excluded.
        std::expected<std::vector<MoveDefinition>, std::string> resolveLightChain(
            const ParamTable& weapons, int weaponId, calculator::MoveId first,
            std::span<const int> categoryPriority) const try
        {
            const auto row = weapons.find(weaponId);
            const auto category = weapons.integer(row, "wepmotionCategory");
            const auto special = weapons.integer(row, "spAtkcategory");
            if (!category || !special)
                return std::unexpected("Weapon row requires wepmotionCategory and spAtkcategory");
            const auto chain = lightAttackChain(*category, *special, first);
            if (!chain)
                return std::unexpected(chain.error());
            std::vector<MoveDefinition> result;
            for (const auto& seed : *chain) {
                const auto definition = resolveMove(seed, categoryPriority);
                if (!definition)
                    return std::unexpected(std::format("{}: {}", seed.name, definition.error()));
                result.push_back(*definition);
            }
            return result;
        }
        catch (const std::runtime_error& error) { return std::unexpected(error.what()); }

        // Development bridge for an already-supported complete catalog. Only
        // ordinary, offhand and matching-powerstance attacks come from the graph;
        // uncovered families retain
        // their validated definitions. Failure in a covered family never falls
        // back to its catalog animation. Every affinity must share graph inputs.
        std::vector<MoveDefinition> resolveSupportedCatalog(
            const ParamTable& weapons, std::span<const long long> weaponIds,
            std::span<const MoveDefinition> catalog) const
        {
            if (weaponIds.empty()) throw std::runtime_error("No weapons supplied for graph resolution");
            constexpr std::array fields{"wepmotionCategory", "spAtkcategory", "wepmotionOneHandId",
                "wepmotionBothHandId", "isWeaponCatalyst", "behaviorVariationId", "wepType", "isDualBlade", "enableThrow"};
            std::optional<std::array<int, fields.size()>> sharedContext;
            for (const auto id : weaponIds) {
                if (id < 0 || id > std::numeric_limits<int>::max())
                    throw std::runtime_error("Weapon ID is outside the parameter row range");
                std::array<int, fields.size()> context;
                for (std::size_t i = 0; i < fields.size(); ++i) {
                    const auto value = weapons.integer(weapons.find(static_cast<int>(id)), fields[i]);
                    if (!value) throw std::runtime_error(std::format("Weapon {} requires {}", id, fields[i]));
                    context[i] = *value;
                }
                if (sharedContext && *sharedContext != context)
                    throw std::runtime_error(std::format("Weapon {} has different graph inputs within a shared moveset", id));
                sharedContext = context;
            }
            const int weaponId = static_cast<int>(weaponIds.front());
            std::map<calculator::MoveId, MoveDefinition> chainMoves;
            using enum calculator::MoveId;
            for (auto first : {ONE_HAND_R1_1, TWO_HAND_R1_1, OFFHAND_R1_1, POWERSTANCE_L1_1}) {
                if (first == POWERSTANCE_L1_1 && excludesMatchingPowerstance((*sharedContext)[0]))
                    continue;
                const auto chain = resolveLightChain(weapons, weaponId, first);
                if (!chain) throw std::runtime_error(chain.error());
                for (const auto& definition : *chain) chainMoves.emplace(definition.id, definition);
            }
            std::vector<MoveDefinition> result(catalog.begin(), catalog.end());
            std::set<calculator::MoveId> ids;
            for (auto& definition : result) {
                if (!ids.insert(definition.id).second)
                    throw std::runtime_error("Duplicate MoveId in supported catalog");
                const auto light = chainMoves.find(definition.id);
                if (light != chainMoves.end()) {
                    definition.animationId = light->second.animationId;
                    definition.motionCategory = light->second.motionCategory;
                } else {
                    switch (definition.id) {
                    case ONE_HAND_R1_1: case ONE_HAND_R1_2: case ONE_HAND_R1_3:
                    case ONE_HAND_R1_4: case ONE_HAND_R1_5: case ONE_HAND_R1_6:
                    case TWO_HAND_R1_1: case TWO_HAND_R1_2: case TWO_HAND_R1_3:
                    case TWO_HAND_R1_4: case TWO_HAND_R1_5: case TWO_HAND_R1_6:
                    case OFFHAND_R1_1: case OFFHAND_R1_2: case OFFHAND_R1_3:
                    case OFFHAND_R1_4: case OFFHAND_R1_5: case OFFHAND_R1_6:
                    case POWERSTANCE_L1_1: case POWERSTANCE_L1_2: case POWERSTANCE_L1_3:
                    case POWERSTANCE_L1_4: case POWERSTANCE_L1_5: case POWERSTANCE_L1_6:
                        throw std::runtime_error("Catalog attack chain disagrees with weapon/script limits");
                    default: break;
                    }
                    const bool movement = movementAttackState(definition.id).has_value();
                    const bool pairedMovement = powerstanceMovementState(definition.id).has_value();
                    const bool jumping = jumpingAttackRoute(definition.id).has_value();
                    const bool mounted = mountedAttackState(definition.id).has_value();
                    const bool whiff = definition.id == BACKSTAB_WHIFF;
                    const bool shieldpoke = definition.id == SHIELDPOKE;
                    const bool feint = feintAttackState(definition.id).has_value();
                    if (movement || pairedMovement || jumping || mounted || whiff || shieldpoke || feint || heavyAttackRoute(definition.id, {})) {
                        // Discard the seed clip so no catalog animation can hide
                        // an unsupported/missing graph state or selected TAE.
                        definition.animationId = -1;
                        definition.motionCategory.reset();
                        const auto resolved = movement ? resolveWeaponMovementMove(definition, weapons, weaponId)
                            : pairedMovement ? resolveMatchingPowerstanceMovementMove(definition, weapons, weaponId)
                            : jumping ? resolveWeaponJumpingMove(definition, weapons, weaponId)
                            : mounted ? resolveWeaponMountedMove(definition, weapons, weaponId)
                            : whiff ? resolveWeaponBackstabWhiff(definition, weapons, weaponId)
                            // Representative normal guard, explicitly labeled in the catalog.
                            : shieldpoke ? resolveWeaponShieldpoke(definition, weapons, weaponId, 0, false)
                            : feint ? resolveWeaponFeintMove(definition, weapons, weaponId)
                            : resolveWeaponHeavyMove(definition, weapons, weaponId);
                        if (!resolved) throw std::runtime_error(std::format("{}: {}", definition.name, resolved.error()));
                        definition = *resolved;
                    }
                }
            }
            for (const auto& [id, definition] : chainMoves)
                if (!ids.contains(id))
                    throw std::runtime_error("Generated attack chain has a move absent from the supported catalog");
            return result;
        }
    };

    // Opt-in development path; load_game_data still owns weapon parsing,
    // moveset attachment, reports and completeness checks. No module cycle or
    // alternate GameData/serialization implementation is needed.
    calculator::GameData load_game_data(const std::filesystem::path& root,
        const BehaviorGraph& graph, std::vector<MovesetImportReport>* reports = nullptr)
    {
        ParamTable weapons;
        weapons.load((root / EquipParamWeaponFile).string().c_str());
        return load_game_data(root, reports, [&](auto weaponIds, auto catalog) {
            return graph.resolveSupportedCatalog(weapons, weaponIds, catalog);
        });
    }
}
