# Automatic moveset mapping investigation

Investigated 2026-09-28 against the user's local game installation and retained
11711000 XML. The manual Dagger/Longsword catalogs are verified fixtures, not
evidence that per-weapon manual catalogs are fundamentally necessary.

## Confirmed local evidence

The retained XML has EquipParamWeapon, BehaviorParam_PC, AtkParam_Pc and player
TAEs, but did not contain the player behavior archive or action scripts.
These additional files already exist in the user's game installation:

- `chr/c0000.behbnd.dcx` (540,192 bytes).
- `action/script/c0000.hks` (1,274,079 bytes, compiled HavokScript).
- `action/script/common_define.hks` (175,611 bytes).

Copied all three into ignored `build/mapping-research/`, then unpacked only
that copy of the behavior archive using the existing WitchyBND 3.0.1.0. It contains
`Behaviors/c0000.hkx` (15,596,096 bytes), a small project HKX and a character HKX.
Downloaded the official HKLibCLI v0.1.2 release into the research directory and
converted `Behaviors/c0000.hkx` to `Behaviors/c0000.xml` (65,387,571 bytes).
No installed game files or retained XML were changed. No animation-motion
archives were unpacked.

The generated XML contains typed objects with IDs and references. In this export:

- `object1191` is an `hkbStateMachine::StateInfo`, named `AttackRightLight1`.
  Its generator points to `object2100`.
- `object2100` is a `CustomManualSelectorGenerator`, named
  `AttackRightLight1_CMSG`, with `animId=30000`, `offsetType=13`
  (`WeaponCategoryRight`), and 58 referenced clip generators.
- Those clips include `a023_030000` and `a020_030000`, matching the existing
  Longsword and Dagger first-light-attack mappings.
- `AttackRightHeavy1End_CMSG` has `animId=30505`, `offsetType=13`, and 117 clips,
  including `a023_030505`, `a020_030505`, `a110_030505`, and `a111_030505`.
- `AttackBothLight1_CMSG` has `animId=32000`, `offsetType=16`, and 68 clips,
  including `a023_032000` and `a020_032000`.

Object IDs above identify this particular export; future code must follow typed
references and properties rather than hardcoding those IDs.

Weapon XML examples (base rows, before affinity variants):

| Row | wepmotionCategory | spAtkcategory | behaviorVariationId |
| --- | --- | --- | --- |
| 1000000 | 20 | 0 | 100 |
| 1010000 | 20 | 103 | 101 |
| 2000000 | 23 | 0 | 200 |
| 2010000 | 23 | 111 | 209 |
| 2020000 | 23 | 110 | 201 |

`spAtkcategory` is now read by the experimental light-chain/category resolver; production
catalog assignment still does not use it. The retained TAE directories
already include categories 103, 110 and 111. For example, category 110 includes
030505 but not 030000. This is evidence for partial override sets; the exact
complete runtime priority rule remains unverified. The scoped ordinary policy
added below handles these partial overrides without claiming universal priority.

## Community sources

- [ERClipGeneratorTool documentation](https://github.com/The12thAvenger/ERClipGeneratorTool/blob/main/README.md)
  explains that clip generators represent TAE entries, CMSGs group candidates
  for an animation ID, active TAE IDs determine selection, and HKS controls combo
  length/composition. It directs full graph conversion to HKLib.CLI.
- [HKLib](https://github.com/The12thAvenger/HKLib) supplies the Havok binary/XML
  converter used here; release [v0.1.2](https://github.com/The12thAvenger/HKLib/releases/tag/v0.1.2).
- [EldenRingHKS](https://github.com/soulsmods/EldenRingHKS) provides readable,
  decompiled and modified player scripts. A reference copy is saved as
  `build/mapping-research/c0000.reference.hks`. It is research material, not an
  exact substitute for the user's compiled script and not a production dependency.
  `AttackRightLight1_onUpdate`, `GetAttackMaxNumber`, `GetDualAttackMaxNumber`,
  `IsEnableNextAttack`, and `IsEnableDualWielding` expose useful selection rules.
- [DSLuaDecompiler](https://github.com/katalash/DSLuaDecompiler) is the decompiler
  used by that project and now also by this local investigation (details below).
- [Smithbox ER weapon annotations](https://github.com/vawser/Smithbox/blob/main/src/Smithbox.Data/Assets/PARAM/ER/Param%20Annotations/English/EQUIP_PARAM_WEAPON_ST.json)
  identify `spAtkcategory` as the special motion category and describe the other
  weapon motion/behavior fields.

## First resolver implementation

`src/core/behavior_graph.ixx` now provides `witchy::BehaviorGraph`, exported through
the existing `erdo` module. It reads HKLib tagfile XML with pugixml, indexes types,
objects and state names, and follows typed StateInfo → CMSG → Clip references.
It does not rely on object numbers or selector naming conventions. Unsupported
generators/bindings, disabled states/TAE, broken references, duplicate
matching clips, malformed IDs and mismatched selector contexts fail explicitly.

`directAttackState(MoveId)` supplies a shared vocabulary for 45 ordinary attack
states. `BehaviorGraph::resolveMove(existingDefinition, categoryPriority)` derives
the animation ID and selected TAE category from the graph. This vocabulary has
no per-weapon IDs or animation numbers. The existing definition retains its
category/behavior constraints and phase filter. `MoveDefinition::motionCategory`
is an optional per-move TAE category; the existing importer/cache now honors it,
and existing `ResolvedHit` provenance/serialization records the selected source.

The resolver handles **clip selection for an explicitly supplied context**, not
complete input/state-machine execution. Category priority is a caller argument.
An absent category may fall through to the next registered candidate; once a
candidate is chosen, missing TAE or failed behavior resolution is an error rather
than a reason to use a different category. The real-data check confirms all
**83 applicable mappings** across the Dagger and Longsword catalogs, comparing
complete imported hit results with the existing catalog path. It also confirms
the graph has category 110 for heavy state 30505 but uses category 23 for light
state 30000 when explicitly requested in that order.

The production `load_game_data` assignment remains on its validated catalogs.
Do not interpret these tests as validation of every weapon's category ordering,
combo length, or conditional heavy-attack routing. No new weapon support is
published merely because a candidate clip exists.

Developer inspection command (offset 13 is WeaponCategoryRight in this export):

```powershell
$graph = 'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-state $graph AttackRightHeavy1End 13 110 23
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-state $graph AttackRightLight1 13 110 23
```

Expected clips are `a110_030505` and `a023_030000`. The command labels priority as
explicit and does not assert HKS reachability.

Nine portable Catch2 cases cover graph resolution/rejection, bounded chains and
the existing importer bridge, including missing selected TAE with a valid base
file present. Configure `ERDO_MOVESET_BEHAVIOR_XML` to enable four real-graph cases
when `ERDO_MOVESET_XML_ROOT` is also set. Local XML is not bundled test data.

## Local HKS and light-chain generation

Built upstream DSLuaDecompiler commit
[`c27340ab1b898a584748dc7e43a516cc1f1684f6`](https://github.com/katalash/DSLuaDecompiler/tree/c27340ab1b898a584748dc7e43a516cc1f1684f6)
using an isolated Microsoft .NET SDK 9.0.318 under `build/mapping-research/dotnet/`.
The SDK ZIP was checked against Microsoft's release-metadata SHA-512. No system
SDK, PATH or Witchy configuration was changed. Both copied game scripts decompile
successfully, with no decompiler errors, to `c0000.game.dec.hks` and
`common_define.game.dec.hks`. These game-derived files remain ignored local data.

Reproduce with the downloaded sources/tools:

```powershell
$research = 'build/mapping-research'
$decompiler = "$research/decompiler-source/DSLuaDecompiler-c27340ab1b898a584748dc7e43a516cc1f1684f6/DSLuaDecompiler/bin/Release/net9.0/DSLuaDecompiler.dll"
& "$research/dotnet/dotnet.exe" $decompiler -o "$research/c0000.game.dec.hks" "$research/c0000.game.hks"
& "$research/dotnet/dotnet.exe" $decompiler -o "$research/common_define.game.dec.hks" "$research/common_define.game.hks"
```

Verified in this exact decompilation:

- `common_define`: lines 292–295 define the four hand/style constants; lines
  357–397 define motion categories (20 = dagger, 23 = straight sword, etc.).
  These are not `wepType`/`Weapon::Type` values.
- `c0000`: `GetAttackMaxNumber` (2307), `GetDualAttackMaxNumber` (2366), and
  `IsEnableNextAttack` (2290) establish shared chain limits. Special categories
  249/258 use five ordinary attacks, or four two-handed; 255/257 use six ordinary,
  or four two-handed. The dual helper does not apply those special overrides.
- `AttackRightLight2..5_onUpdate` and `AttackBothLight2..5_onUpdate` gate the next
  numbered state using those limits. `AttackDualLight1..5_onUpdate` uses the dual
  limit; `ExecAttack` maps its left-light requests to `AttackDualLight1..6`.
- `IsEnableDualWielding` (2398) explicitly excludes dueling shields even though
  the dual count helper returns four for them. Torches also lack that route.
  The new resolver rejects these matching-pair powerstance requests.
- `ExecAttack` also redirects attacks under special effects (for example
  19903/19904/19915). Combo limits are therefore not a full reachability model.
  Offhand attacks enter `AttackLeftHeavy1`, whose generator is not a direct CMSG.
  The resolver now follows that nested selector instead of using the matching
  `AttackLeftLight1` clip as a shortcut (see below).

`lightAttackChain(wepmotionCategory, spAtkcategory, firstMoveId)` is a C++
translation of the verified ordinary melee limits and returns shared semantic
definitions with animation IDs unset. `BehaviorGraph::resolveLightChain` reads
the existing `ParamTable` row (including schema defaults), selects that chain,
and derives each animation from its graph state. No weapon-specific catalog,
numeric animation formula, or catalog constraints are supplied. Resolution is
atomic: a failed intermediate state returns an error, not a shortened chain.
Powerstance assumes a matching weapon pair. Ranged weapons, catalysts, shields,
unknown categories and offhand chains reject until their request paths are handled.

This is a source-verified translation of one local script, not a general HKS
interpreter or automatic version detector. Recheck it against the hashes below
when updating game inputs. Explicit category priority remains available;
this API does not enable arbitrary weapons in `load_game_data` yet.

```powershell
$graph = 'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
$params = 'C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data/regulation-bin/EquipParamWeapon.param.xml'
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-chain $graph $params 1000000 1h 20
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-chain $graph $params 2000000 2h 23
```

These produce six Dagger and five Longsword attacks. The real-data regression
independently compares all 30 ordinary 1H/2H/powerstance moves across those two
weapons, including complete imported hits and existing serialization. It also
checks the graph's fifth/sixth powerstance states for the fist motion category.
The suite passes **67/67** with both local inputs. No additional animation
archives were unpacked and no bundled GameData was regenerated.

## Generic manual selectors and maintenance

`BehaviorGraph` now traverses `hkbManualSelectorGenerator` nodes between a state
and its CMSG. For an unbound selector it reads `selectedGeneratorIndex`. For a
bound selector it follows `hkbVariableBindingSet`, requires a whole-variable
binding to that member, and resolves the variable index through the graph's
`data` → `stringData` → `variableNames` references. Neither variable numbers nor
selector object IDs/names are hardcoded. This agrees with the ER examples in
the [behavior editing documentation](https://soulsmodding.com/doku.php?id=topic%3Ahavok_behavior_editing).

Callers supply a `BehaviorVariables` map of known runtime values to `inspect`,
`resolve` or `resolveMove`. Missing values produce a diagnostic naming the
variable; serialized initial values are not silently treated as live state.
This evaluates a selection snapshot, not HKS execution or transitions over time.
Unaudited script generators, non-exact blends, index-selector objects, non-instantaneous transition effects, bit and
property bindings, and multiple graph scopes still reject. Cycles, invalid
indices/counts, missing references and ambiguous variable names also reject.

Verified against the local graph:

- `AttackLeftHeavy1` → unbound manual selector, index 0 → CMSG 35000, offset 14.
  `OFFHAND_R1_1` now uses this actual HKS entry. All 83 existing catalog/graph hit
  comparisons still pass with this correction.
- `AttackRightHeavySpecial1Start/End` → selector bound to
  `AttackRightHeavySpecialType`. Context 0 selects 30600/30605; context 1 selects
  30620/30625. The variable is index 354 in this export, but code discovers its
  name and index from XML. HKS `ExecAttack` sets this variable using active-effect
  conditions. The heavy-routing API now evaluates the relevant effect conditions
  from an explicit active-effect snapshot (see below).

Developer inspection examples:

```powershell
$graph = 'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-state $graph AttackLeftHeavy1 14 23
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-state $graph AttackRightHeavySpecial1Start 13 23 AttackRightHeavySpecialType=0
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-state $graph AttackRightHeavySpecial1Start 13 23 AttackRightHeavySpecialType=1
```

The maintenance target is shared game rules plus imported data. Weapon parameters
select categories; graph nodes describe branches; HKS supplies runtime conditions.
A unique weapon can therefore select unique behavior without requiring a custom
C++ weapon-ID branch. However, the game itself has some category-specific script
rules. The existing C++ combo-limit translation mirrors a few of those and must
be audited when HKS changes. It is not yet an automatic script evaluator.

The manual animation catalogs remain temporary validated coverage. Their old
behavior-ID disambiguations are retained as regression references; production
now uses the shared player-weapon row lookup described below. Do not grow them into a
comprehensive weapon database as a substitute for following game rules. When
new behavior cannot be interpreted, report the unsupported mechanism rather
than guessing or quietly choosing a weapon-specific animation. Complete removal
of exceptions cannot be promised until category/behavior routing is established.

## Active effects and encoded behavior judges (2026-09-29)

`heavyAttackRoute(MoveId, activeEffects, twoHandingLeft)` implements the ordinary
R2 effect branches from local HKS `ExecAttack` (lines 1524–1557 and 1638–1696).
`BehaviorGraph::resolveHeavyMove` passes that state and its derived selector
variables into the existing graph resolver. It supports both combo stages,
charged/released phases, and right/left two-handing. Its inputs contain no weapon
IDs or animation IDs. Independent effect tests retain HKS overwrite order;
effect 19912's final reset intentionally checks right-hand effect IDs even in
left-two-handed style, matching the local script.

This snapshot must list effects that are actually active. Weapon passive fields
and the equipped skill alone cannot establish that a temporary buff is active.
The community's [HKS API reference](https://github.com/vawser/ER-Documentation/blob/main/Info%20-%20HKS.txt)
identifies `env(1116, id)` as `GetSpEffectID`; the effect names for 1681 and 1811
are Barbaric/Milos Roar and War Cry in
[Paramdex](https://github.com/soulsmods/Paramdex/blob/master/ER/Names/SpEffectParam.txt).
Local SpEffect rows also contain finite durations. This code does not infer buff
activation/expiration, apply buff damage multipliers, or simulate charge timing.
Guard-counter effects reject because their entry needs additional input/node
context. Combo sub-starts and other attack-request branches remain outside this API.

The selected buff TAEs exposed judges such as 3950 and 3982, which are not literal
`behaviorJudgeId` values in BehaviorParam_PC. The ER implementation in
[DSAnimStudio, pinned commit f1bff06](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/TaeEditor/TaeActionSimulationEnvironment.cs#L1831)
provides the encoding and fallback rule for player weapon events:

- For judges 1000–9999, namespace = judge / 1000 and suffix = judge % 1000.
- Candidate row ID = namespace * 100000000 + variation * 1000 + suffix.
- Try the selected weapon's variation, then its hundred-group variation, then 0.
  Stop at the first existing row, before checking its reference/category.

`BehaviorIndex::findEncoded` implements that rule. The default diagnostic mode of
`resolveAttackEvents` uses it for the encoded range and retains strict field-based
matching for ordinary judges, including ambiguity diagnostics. The explicit
player-weapon mode described below also addresses ordinary judges by row ID.
Selected rows still pass all existing reference, attack-row and category checks;
an invalid selected row cannot fall through to another variation. Raw encoded
judges stay in `ResolvedHit::event`, with the actual behavior/attack IDs recorded
separately. Caller-selected weapon/source context is still required; this does
not establish mixed-weapon routing or common/base-model event handling.

The local regression now imports the Longsword charged roar paths end to end:
1681 selects a23/30600, whose three physical MVs are 82, 82, 86; 1811 selects
a23/30620, whose ordered physical MVs are 0, 0, 0, 175. Zero-MV contact events are
retained. These are raw XML values before buff multipliers and enemy defenses.
All 16 ordinary heavy moves across Dagger/Longsword still match full catalog hit
results with an empty effect context. Portable tests exercise precedence, hand
selection, namespace boundaries, family/common fallback, failed selected rows,
category/explicit-ID constraints, rebuilds and JSON provenance.

```powershell
$graph = 'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-heavy $graph 1h 1 charged 23 effect=1681
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-heavy $graph 1h 1 charged 23 effect=1811
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-heavy $graph 2h-left 1 released 23 effect=1813
```

The development command reports clip routing only. Buffed variants are not
attached to production GameData: the current UI/model does not select active
effects, and assigning them to the default MoveId would misrepresent ordinary
attacks. Existing classes/serialization are reused without a new game-data model.

## Ordinary player-weapon lookup (2026-09-29)

The same pinned DSAnimStudio implementation starts ordinary judges in namespace 1.
`BehaviorIndex::findPlayerWeapon` now handles both ordinary and encoded judges;
the explicit `BehaviorLookup::PLAYER_WEAPON` argument selects it in the existing
resolver and both `importMoveset` overloads. Its inputs represent the weapon
already selected by the caller. The source's event simulation chooses right/left
weapon for Source 1/2, retains the selected context for Source 0 and throw events,
and has a separate base-model/common path. This API does not infer that context
or implement those other paths.

Production `importWeaponMovesets` uses this mode and clears the legacy per-move
category/behavior-ID constraints before import. Animation and phase definitions
are unchanged. The strict diagnostic mode remains available to inspect collisions
and compare old constrained imports. No new weapon-ID exceptions were added.

A real-input regression compares all **129 moves / 149 ordered hits** from the
unconstrained player-weapon path against the old constrained Dagger/Longsword
imports, including complete serialized values, timing, channels and provenance.
It also samples variation 201: ordinary judge 0 inherits row 100200000, while
judge 100 uses row 100201100. This verifies Broadsword behavior-row fallback,
not its complete animation mapping or moveset. Portable tests cover exact,
family and common rows, same-category namespace collisions, constraint failures,
invalid selected references and index rebuilds.

Supported production weapons remain the same 26 Dagger/Longsword affinity variants.
This removes their manual behavior disambiguation; automatic animation-category
selection and broader weapon assignment are still separate work.

## Parameter-derived ordinary categories (2026-09-29)

`ordinaryWeaponCategories` reads `wepmotionCategory`, `spAtkcategory`, the requested
stance's motion position and `isWeaponCatalyst`, including schema defaults. It
orders a nonzero special category before the base category, without duplicates.
Missing/malformed fields, categories outside 0–999, unsupported melee routing,
nonstandard stance positions and weapon catalysts reject. There is no implicit
category-0 fallback. This policy concerns ordinary selected-weapon 1H/2H attacks;
it does not cover powerstance, offhand, skills, body events or temporary effects.

Evidence for this scoped policy combines the ER-specific
[mod author's account of partial special-motion overrides](https://www.nexusmods.com/eldenring/mods/7222),
the [clip tool author's description of registration and fallback](https://github.com/The12thAvenger/ERClipGeneratorTool/blob/main/README.md#troubleshooting),
and the local graph/TAE partial sets. **The policy is an inference from these
sources, not a recovered native selector implementation.** The tests validate
its implementation and sampled data consistency, not every in-game context.
Smithbox annotations were also checked at commit
[`3411b8b`](https://github.com/vawser/Smithbox/blob/3411b8bb03a8e7ca20e04a11939d1e577d392d4f/src/Smithbox.Data/Assets/PARAM/ER/Param%20Annotations/English/EQUIP_PARAM_WEAPON_ST.json);
they name the relevant fields but do not document complete priority semantics.
An ignored copy and commit record are in `build/mapping-research/`.

`BehaviorGraph::resolveLightChain(weapons, weaponId, firstMove)` now supplies that
policy to the existing graph/chain resolver. `resolveWeaponHeavyMove(definition,
weapons, weaponId)` does the same for a requested neutral heavy stage with no
temporary effects. Neither creates replacement game classes or serialization.
Missing special registrations may use base registrations; a malformed selected
clip or a missing selected TAE still fails instead of silently falling back.

Real tests now compare 119 Dagger/Longsword moves to complete catalog hit
values. They also resolve/import five light attacks plus the first heavy for
Shortsword and Broadsword: lights use a23, heavies use a111/a110, and behavior
lookup selects variation-209/201 rows. This is sample coverage, not complete
support for those weapons. Production full catalogs and support remain unchanged.

The developer chain command can omit category arguments for ordinary 1H/2H:

```powershell
$graph = 'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
$weapons = 'C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data/regulation-bin/EquipParamWeapon.param.xml'
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-chain $graph $weapons 2020000 1h
```

Offhand and matching powerstance can omit categories when the special category
is zero; nonzero special categories still require explicit arguments. The generic state/heavy
commands retain their explicit context rather than adopting this limited policy.

## Graph-backed development GameData import (2026-09-29)

`load_game_data(root, graph, reports)` now connects the graph to the existing
weapon importer through an optional `MovesetDefinitionResolver` callback. The
default overload is unchanged in behavior. The graph partition depends on the
importer, while the importer accepts definitions without depending on the graph;
no cyclic module dependency or replacement data/serialization layer is needed.

`BehaviorGraph::resolveSupportedCatalog` derives ordinary/offhand/matching-powerstance chains,
the requested neutral heavy stages, movement attacks, guard counters, airborne jumps, mounted attacks and backstab whiff. It retains names, order and phase filters;
uncovered move families retain their validated catalog definitions. There are
290 graph-derived mappings and 25 retained mappings across dagger, straight-sword, axe, hammer and flail families.
The importer attaches 246 variants whose complete routing signature matches
these families, with full 66/63/63/63/60-move coverage. Graph sample coverage alone does
not grant a weapon a partial production moveset.

Before sharing a resolved set, the bridge compares every affinity row's motion
category, special category, stance positions, catalyst flag and behavior
variation. Missing or differing context fails explicitly. Graph-generated light
membership must exactly match the supported catalog; missing states or failed
selected TAEs cannot fall back to a catalog animation. The importer also requires
the definition callback to preserve all IDs and their order. Failures for all
groups are collected into the existing reports before throwing.

`--import-graph <xml-root> <behavior.xml> <output.json>` exposes this opt-in path.
It uses the same save/load/semantic verification as `--import` and refuses to
overwrite an output file. The generated ignored artifact is
`build/development-game-data/11711000.graph-flails.json` (3,296 weapons,
246 with movesets in twenty-two exact behavior groups). It can be loaded through the normal UI workflow documented in
`tests/README.md`. Installed files, retained XML and bundled JSON are unchanged.

New Catch2 regressions verify that deliberately changed graph animations replace
catalog animations only in covered families; reject missing states, incomplete
chains and mismatched affinity contexts; and compare complete real GameData
movesets before/after JSON with the original import, including shared identity.
The existing default import remains the independent comparison path.

## Movement requests and guard counters (2026-09-29)

The shared `movementAttackState` vocabulary covers running R1/R2, rolling R1,
backstep R1 and guard counter for 1H/2H. `directAttackState` delegates to it and
`resolveWeaponMovementMove` uses the existing ordinary category policy. The
development catalog bridge derives all twenty mappings across the two supported
weapons, with no weapon-specific IDs or animation arithmetic.

Evidence is the exact local `build/mapping-research/c0000.game.dec.hks`:

- Locomotion selects the RightLightDash/RightHeavyDash and BothDash/BothHeavyDash
  events when speed is 2 (6525–6529).
- Rolling/backstep EvasionCommonFunction calls supply RightLightStep/BothLightStep
  and RightBackstep/BothBackstep (7328/7351/7375).
- ExecAttack's guard-counter branch (1439–1462) checks effects 173/174 or the
  active guard node with effect 102020, then R2 input and weapon category. Its
  successful dispatch uses **AttackRightHeavyCounter / AttackBothHeavyCounter**.

The prior LightCounter mapping happened to select identical clips: the inspected
a23 graph resolves both right-hand names to 30700 and both two-handed names to
32700. The state vocabulary is now corrected to the script's HeavyCounter route;
all validated hit values remain unchanged. Portable tests reject a graph that
only provides LightCounter, even if a valid catalog animation is supplied.

This resolves a requested situational attack, not real-time input eligibility.
Movement state, stamina, guard windows, stealth branches and temporary redirects
are not simulated. In particular, choosing the guard-counter MoveId assumes its
guard precondition has been met. The neutral-heavy API still rejects effects
that require that additional context. Offhand/powerstance/jumping/mounted/critical
families remain outside this movement helper.

The real parameter-derived comparison now covers all 119 graph-backed moves with
complete hit values, and the full GameData/JSON test still checks all 129 moves
across 26 affinities. The synthetic catalog test deliberately changes all 28
covered Longsword clips and verifies missing running/counter states fail instead
of reusing the catalog.

## Offhand and matching powerstance (2026-09-29)

The exact local HKS offhand callbacks (8628–8684) route through
`AttackLeftHeavy1…6`, despite the community-facing Offhand R1 names. After the
first stage, `IsEnableNextAttack(n, HAND_LEFT)` uses `GetAttackMaxNumber`'s ordinary
one-handed limit, including its shared special-category overrides. First-stage
continuation offers stage 2 without that check; the melee categories supported
here all have limits of at least three. The shared chain vocabulary now includes
the six Offhand MoveIds. It does not infer that every L1 input attacks: request
selection (1325–1337) checks powerstance and guarding first, while L2 can select
skills or left-heavy attacks (1351–1382).

Dual callbacks (8693–8753) use the independent `GetDualAttackMaxNumber`, and
`ExecAttack` (1705–1730) translates chain and movement requests into
`AttackDualLight*`, `AttackDualDash`, `AttackDualRolling` and `AttackDualBackStep`.
The matching-pair subset of `IsEnableDualWielding` is already reflected in the
chain checks: torches/dueling shields are excluded. This does not generalize to
mixed weapons or special dagger/katana/thrusting-sword combinations.

`offhandOrMatchingPairCategories` validates the selected weapon and allows
automatic left/dual category selection only when `spAtkcategory` is zero. With
one base category there is no unverified special/base ordering to infer.
Nonzero special categories fail with an explicit-context diagnostic; the
existing explicit-priority inspection overload remains available.
`resolveMatchingPowerstanceMovementMove` applies this rule to the three movement
IDs; `resolveLightChain` uses it for offhand and matching-powerstance chains.
No weapon IDs or animation IDs are added to these rules.

The complete catalog bridge now derives all 83 previously validated direct-state
mappings. All four chain memberships must match the complete catalog. Missing
offhand/dual states, mismatched lengths or failed selected TAEs reject the import.
Full imported hit values, including source hands, timing and MVs, match the old
catalog path across all supported affinities and survive existing JSON sharing.

```powershell
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-chain $graph $weapons 1000000 offhand
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-chain $graph $weapons 1000000 powerstance
```

These yield six offhand and four powerstance chain entries for base Dagger.
The command labels offhand as an already-selected attacking left-hand request
and powerstance as a matching weapon pair. It does not simulate runtime input.

## Airborne jump routing and landing boundary

The exact local HKS initializes `JumpAttack_HandCondition` to 0 for ordinary
right-hand attacks and 1 for right-two-handed attacks (3566–3570).
`JumpCommonFunction` (22834 onward) sets `JumpAttackFormRequest` to 0 for R1,
1 for R2, and 2 for eligible powerstance L1. Its ordinary falling entry is
`JumpAttack_Start_Falling`; `JumpAttack_Land == 0` selects the airborne branch.
`jumpingAttackRoute` supplies these named variables for five existing MoveIds.
`resolveWeaponJumpingMove` uses the existing category policies, restricts
powerstance to the supported matching-pair context, and rejects special-category
293's jump R2 magic redirect. It resolves an already-selected airborne attack,
not jump input eligibility, left-two-handed style or a complete flight timeline.

The nested landing-condition selectors reference an unbound
`CustomTransitionEffect` with zero duration/start fraction, default modes/flags
and no alignment bone. Traversal now accepts only that instantaneous case;
nonzero timing, bindings, other effect types or modes still fail. No object IDs,
clip IDs or weapon IDs from the local export are embedded in the resolver.

Landing branches select clips whose `startTime` is bound to `TimeActEditor_10`.
The [ERClipGeneratorTool documentation](https://github.com/The12thAvenger/ERClipGeneratorTool#jump-attacks-trigger-again-when-landing)
explains that the preceding TAE's BlendToIdleOrMovementAnim event supplies this
value to synchronize landing. They remain explicitly unsupported: serialized
zero start times are not a substitute for that runtime context. Airborne hit
times remain animation-relative, and landing events are not appended or counted
twice. This does not claim to predict which hits connect before landing.

The bridge now derives 93 of 129 definitions. All ten added Dagger/Longsword
jump definitions match the complete catalog hits, including source hands,
motion values, timing and alias provenance. Portable tests use different clip
IDs for the five branches and reject dynamic landing clips and timed transitions.
Existing GameData/JSON tests continue to verify all 26 affinities and shared sets.

## Mounted sides, chains and heavy phases (2026-09-30)

Local `ExecRideAttack` (18649–18733) selects the equipped attack weapon from
hand style, then uses R/L input to select the side of the horse. Both sides'
graph CMSGs use offset 16 (`WeaponCategoryHandStyle`); L attacks must not be
resolved as ordinary offhand offset 14. `RideAttack_R_Top*` callbacks
(20817–20902) and their L counterparts (21249–21339) cycle through three
stages. `mountedAttackState` maps the six existing light MoveIds to those
semantic states without animation arithmetic.

The heavy callbacks (20904 onward and 21079 onward) enter `Hard1_Start` and
switch to `Hard1_End` on release in the charging window. Existing charged and
charging MoveIds both use the Start clip with their established event-judge
filters; the ordinary R2/L2 uses End. Graph traversal preserves these filters
and refuses a phase MoveId without one. The filters still come from the
validated definitions, not an automatic interpretation of TAE charging windows.
Repeated events and animation-relative times remain unchanged.

`resolveWeaponMountedMove` scopes automatic resolution to the selected
right-hand melee weapon, ordinary hand-style position 10, no catalyst and
zero special category. This avoids guessing mounted special/base precedence.
Generic explicit-category inspection remains available. The helper does not
simulate charge duration, mounted jumps, left-equipped style or the additional
Hard2 continuation used by thrusting-sword categories (`IsEnableRideAttackHard2`,
19142 onward). A selected stage is not a claim that every input context enters it.

The complete import now derives 117 of 129 animation/category mappings, with
12 critical/backstab definitions still catalog-based. All 24 mounted additions
match complete old-path hit data. Synthetic bridge coverage is 57 Longsword
definitions, with distinct missing right-light/left-heavy state failures;
complete GameData and shared-pointer JSON comparisons cover all 26 affinities.
No additional archives or per-weapon animation tables were needed.

## Explicit throw context and backstab whiff (2026-09-30)

`Throw_Activate` in the exact local HKS (15490 onward) reads native env(273)
into `ThrowID`. The `ThrowAtk` graph state is a parametric blender bound to that
named variable. Its authored sample values match the attack animation IDs in
`ThrowParam`; the selected child then supplies the hand-style CMSG (offset 16).
The five successful critical catalog animations can each be reached through
explicit existing throw rows. Multiple throw types and targets share animations,
so the current small/large labels do not determine a unique throw interaction.

The resolver now accepts exact samples of `hkbBlenderGenerator` with only flag
16, an explicit whole-variable `blendParameter` binding, strictly increasing
finite sample weights, no bone mask, full motion weights and no synchronization
or reference-pose contribution. Flag 16 is `FLAG_PARAMETRIC_BLEND` in
[HKLib's pinned definition](https://github.com/The12thAvenger/HKLib/blob/ac40bc9915d8caff0df154c2b9a79656dfca1d77/HKLib/hk2018/Autogen/hkbBlenderGenerator.cs).
No interpolation, clamping, cyclic blend or event merging is implemented. Unknown
or duplicate samples fail, even when adjacent clips could produce a plausible hit.

`resolvePlayerThrow(definition, throws, rowId, categoryPriority)` requires an
explicit player-attacker row with ordinary weapon-category handling and type ID
zero. It reads `atkAnimId` including XML defaults, supplies `ThrowID`, and checks
that the graph returns that same animation; the graph's error-handling substitute
is not accepted. The caller must select the row using target/interaction context
and supply category priority. The passed MoveId/name is a caller label, not an
inferred classification. No enemy size heuristic or production target-ID table
was added. This API reuses MoveDefinition, ParamTable and the existing importer.

```powershell
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-throw `
    'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml' `
    'C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data/regulation-bin/ThrowParam.param.xml' `
    11317000 23
```

This explicitly selected interaction resolves a23/31720; it does not establish
target eligibility or a universal small-enemy backstab. Real tests use five
representative rows, compare all ten imported critical moves across Dagger and
Longsword to the old catalog, and preserve behavior lookup and alias provenance.

`ThrowBackStab` instead wraps a normal CMSG in `hkbScriptGenerator` with only
`ThrowBackStab_Activate()` enabled. The audited local callback (15462 onward)
establishes attack/guard hand without selecting a different clip. Traversal
allows exactly that activation callback and empty remaining callbacks; arbitrary
scripts still reject. `resolveWeaponBackstabWhiff` scopes automatic use to the
base-category selected right weapon and requires the existing strike phase
filter, excluding throw-acquisition events. The full import therefore gains two
whiff mappings: **119 derived / 10 catalog-based**. Successful throws remain
catalog-based there until target/throw selection is represented. Full flight,
critical bonus and enemy-defense calculations are still outside this importer.

## Target-query throw hit data (2026-09-30)

`PlayerThrowQuery` supplies defender character ID, throw type, attack-linked
throw type ID and attacker enable state. `playerThrowCandidates` matches these
authored fields with player attacker ID zero, reads schema defaults and returns
all matching row IDs in sorted order. It does not fall back to defender zero,
infer a character model from an NpcParam ID, or remove unsupported matches.
These fields' meanings are documented by the pinned
[Smithbox ThrowParam annotations](https://github.com/vawser/Smithbox/blob/3411b8bb03a8e7ca20e04a11939d1e577d392d4f/Smithbox.Release/Output/Assets/PARAM/ER/Param%20Annotations/English/THROW_PARAM_ST.json).
The enums still give generic type numbers; the resolver does not invent their
runtime state semantics or equate them with the existing small/large MoveIds.

`resolveWeaponThrow` additionally validates `enableThrow == 1` and the existing
base-category selected-right-weapon hand-style policy. It resolves every
candidate through `resolvePlayerThrow`, rejecting any unsupported row or any
different resulting animation/category. If all agree, their attack hit data is
the same for the supplied weapon and phase definition. This establishes a common
clip for calculation; it does not select a valid positioning row or promise
that an attack can currently start. Distance, facing, height, collision, target
state and the native throw request remain runtime concerns.

For example, c3950/type 22 has eight rows with different position/direction data
but the same attack animation (31730). c2010/type 25 has both 31700 and 31710;
that query fails as ambiguous rather than selecting the first row. The tests
verify those real cases and reproduce all ten critical catalog moves using
target queries and weapon parameters, without passing row IDs or animations to
the new resolver. No per-weapon or per-target production mapping table was added.

```powershell
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --resolve-target-throw `
    'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml' `
    'C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data/regulation-bin/EquipParamWeapon.param.xml' `
    'C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data/regulation-bin/ThrowParam.param.xml' `
    2000000 3950 22 0 0
```

Arguments after the three XML paths are weapon ID, defender character ID, throw
type, type ID and attacker enable state. The example resolves eight candidate
rows to a23/31730. This developer command does not expose technical parameters
in the application's user flows. GameData still has no selected-target context,
so the target-independent development import remains at 119 graph mappings and
ten representative catalog critical definitions. The latest development JSON
is unchanged; target-query support does not require a new serialization format.

## Complete shared families (2026-10-04; initial expansion)

The full importer no longer restricts complete movesets to the two original
weapon IDs. It requires an exact validated parameter signature: dagger weapon
type with motion category 20 / behavior variation 100, or straight-sword type
with 23 / 200; ordinary stance positions 0/10; zero special category; no catalyst
or built-in paired weapon; and critical capability (`enableThrow == 1`). All
fields must be present in the default-expanded parameter map. Matching only the
motion category is insufficient. The graph bridge checks these additional
sharing inputs across every member too.

This reuses shared HKS rules and parameter routing: ordinary/offhand combo
counts use category and special category, matching powerstance admits these
ordinary same-category pairs, graph clips use the same category/stance inputs,
and hit behavior lookup uses the same variation. Existing phase/critical
definitions remain family-specific regression expectations. Attack power,
scaling, elemental damage and passive status differ by weapon and continue
through the existing calculator, not copied from the family reference weapon.

The retained data yields five dagger weapons (41 variants) and eight straight
swords (56 variants), listed in `tests/README.md`. They share two complete
movesets, including across base weapon names. No per-weapon allowlist or new
animation table was introduced. Nonzero special/other behavior variations and
other weapon classes remain unsupported pending their own complete rules.

Portable tests prove membership is independent of ID and reject a change or
omission of each signature field. Full real-data tests independently name the
thirteen expected weapons, check every move through calculation, preserve
cross-weapon identity through JSON, compare graph/default imports, and exercise
UI loading and both optimizer algorithms for all 97 variants. The original
Dagger/Longsword hit snapshots remain unchanged. There are still 119 graph
mappings and ten representative critical definitions across the two unique
sets; the coverage increase is in weapons, not new move types.

## Behavior variations within complete families (2026-10-04)

The next expansion preserves those animation-routing guards while allowing
integer behavior variations 100–199 for daggers and 200–299 for straight swords.
The existing PLAYER_WEAPON resolver checks an exact variation before its
hundred-family and common rows. Animation selection still depends on the same
validated category, stance and special-category context. A differing behavior
variation creates a separate complete import; it never shares another variation's
resolved hit data. Invalid exact rows remain authoritative failures.

Seven more named weapons qualify in the retained XML: Reduvia (104), Coded Sword
(204), Sword of St. Trina (205), Rotten Crystal Sword (206), Golden Epitaph (207),
Sword of Night and Flame (214), and Regalia of Eochaid (215). None has ordinary
namespace-1 behavior overrides in this data; their ordinary hits inherit the
base rows. Their variation-specific namespace-3 skill rows remain outside the
supported move catalogs. There is no new weapon-ID or name allowlist.

`load_game_data` now supplies only exported weapon rows to the moveset importer.
Unnamed/debug rows, including unused dagger variation 116, cannot create extra
groups or invalidate an otherwise complete export. The existing weapon parsing,
calculator classes and reflection serializer remain in use.

Coverage is now **20 base weapons / 104 weapon-affinity variants / 9 shared
movesets**, using the same two complete animation templates (66 dagger moves,
63 straight-sword moves). Portable tests verify separate exact-variation contexts,
shared equivalent contexts and rejection of malformed/out-of-family values.
Real tests compare every new ordinary hit with the independently snapshotted
base catalog, verify JSON variation metadata and pointer identity, and exercise
all supported weapons through the UI and both optimizer algorithms.

## Ordinary axes and critical event context (2026-10-04)

Axes use motion category 30 (`WEAPON_CATEGORY_AX`), distinct from weapon type 17.
The exact local HKS gives five ordinary/offhand light stages, four matching-pair
stages, and permits matching axe pairs (GetAttackMaxNumber/GetDualAttackMaxNumber,
2307–2377; IsEnableDualWielding, 2454). They have no special-heavy feint eligibility
(IsEnableSpecialAttack, 2379), shieldpoke request (1207–1220), or mounted Hard2
continuation (19142). With special category zero and ordinary stance/catalyst/
paired/throw flags, they use the existing 63-move vocabulary and mounted phase
judges. All graph-covered routes are compared with that template in the full
GameData test; hit values are independently snapshotted from axe XML.

`axeCatalog` derives from the existing straight-sword template, clearing its
behavior constraints and selecting type-304 events for successful criticals.
It is a shared class template, not a list of weapon-ID exceptions. The importer
adds type17/motion30/variation1400–1499 to its existing signature rules. Six names
qualify: Battle Axe, Jawbone Axe, Highland Axe, Sacrificial Axe, Rosus' Axe and
Stormhawk Axe. Their 54 variants share three exact-variation sets (1400/1412/1414).
The latter two inherit ordinary behavior rows in the retained regulation data.

The [Witchy ER TAE template](https://github.com/ividyon/WitchyBND/blob/main/WitchyBND/Assets/Templates/TAE.Template.ER.xml)
distinguishes type1 `AttackBehavior` from type304 `ThrowAttackBehavior`, matching
the locally installed 3.0.1.0 template. Axe criticals contain two type304 hits
and separate type1 collision events; backstab's collision judge592 has no
matching ordinary behavior row. The critical calculation is explicitly scoped
to the throw victim, so it selects type304 rather than treating every collision
in the clip as a hit on that victim. This does not infer whether those collision
events execute or hit bystanders at runtime.

`MoveDefinition::eventType` is an optional filter combined conjunctively with
`eventJudgeId`. It accepts the two extracted attack-event types, preserves all
selected events and their order/timing, and rejects empty selections or any bad
selected hit. It never alters the raw cache. Unfiltered import still rejects
unresolved collisions. `resolvePlayerThrow` now supplies type304 explicitly and
rejects an incompatible supplied event type. Original dagger/straight-sword
hit snapshots are unchanged; all fifteen representative critical datasets are
also checked through explicit target queries.

The retained `a30-tae` directory was incomplete: the graph registers a30/31730,
but its XML was missing. Re-extracting a workspace copy of the existing 760 KB
`a30.tae` recovered the alias to a30/31720. All127 pre-existing clips have identical
attack signatures. No category fallback or invented alias was added. Fresh XML
is in `build/mapping-research/axe-tae-audit/a30-tae`; the ignored development input
view `build/development-xml/axes` uses directory junctions to it and the existing
retained parameter/message/TAE directories. The local test cache points there.
The existing c0000 archive already contains everything needed for this expansion;
`generate_game_data` needs no additional archive. The retained input is untouched.

The raw snapshot `tests/expected_axe_moves.inc` covers all63 moves/77 ordered hits,
all12 MV channels, source/alias provenance, behavior/attack IDs and timing. It was
generated directly from XML, independently of C++ importer output. The older local
community CSV agrees with the main hit sequences but lists440 for one small-PvE
backstab hit where the current representative target data gives418; retain the
current XML result rather than forcing that broad target label to match the sheet.

Coverage after the axe expansion: **26 base weapons /158 weapon-affinity variants /12 shared
movesets**, three class families,192 move definitions/226 hits across their base
sets. Critical bonuses, target eligibility, secondary collision targets and skills
remain outside the current move-power calculation.

## Ordinary hammers (2026-10-05)

The existing five-stage template also matches ordinary hammers: motion category33
(`WEAPON_CATEGORY_HAMMER`), weapon type21, behavior variations1100–1199. The exact
local HKS gives five ordinary/offhand stages and four matching-pair stages, permits
matching hammers, and excludes the same feint/shieldpoke/mounted Hard2 routes as
ordinary axes. Existing stance0/10, special0, catalyst0, built-in-pair0 and throw1
guards remain. `hammerCatalog` reuses the complete63-move vocabulary and phase
filters; all hits use hammer behavior rows and the authored TAE aliases.

Nine names qualify: Curved Club, Warpick, Varré's Bouquet, Hammer and Flowerstone
Gavel (1100), Envoy's Horn (1110), Nox Flowing Hammer (1112), Ringed Finger (1113),
and Marika's Hammer (1115). Their45 variants share five exact-variation sets.
The four non-base variations have no ordinary namespace1 override rows in this
regulation version, so their complete hit data inherits1100. Names are independent
test expectations; the production policy uses the shared parameter signature.

Fresh extraction of the retained726464-byte `a33.tae` recovered critical aliases
missing from the old XML. All132 existing clips' attack signatures match the fresh
extraction. Criticals/backstab whiff alias a30 clips but retain the hammer behavior
variation and attack IDs, exercising cross-class event provenance without copying
axe motion values. The raw snapshot `tests/expected_hammer_moves.inc` checks all63
moves/77 hits, all12 MV channels, event/behavior/attack IDs, indices, timing and alias
origins. Explicit target-query checks now cover20 representative critical datasets.

The current development input is `build/development-xml/hammers`: a read-only input
view using directory junctions to retained parameters/messages/other TAEs, fresh
a30 XML and fresh a33 XML. The local test cache points here. Original retained
data and prior development artifacts remain intact; the existing c0000 archive
already contains both TAEs.

Coverage after the hammer expansion: **35 base weapons /203 weapon-affinity variants /17 shared sets**.
Four base families contain255 definitions/303 hits:235 graph-derived mappings and
20 representative catalog criticals. Portable tests verify signature-based hammer
membership with arbitrary weapon IDs, sharing and rejection of changed/missing
fields. Real tests compare every new set with the raw hammer snapshot and preserve
sharing through JSON, UI display and both optimizer paths. Special categories,
skills, mixed pairs, target eligibility and critical bonuses retain their prior limits.

## Flails and shorter complete chains (2026-10-05)

Flails use motion category34 (`WEAPON_CATEGORY_FLAIL`), weapon type24 and ordinary
behavior variations1300–1399. The exact local HKS gives four ordinary/offhand light
stages and four powerstance stages (GetAttackMaxNumber/GetDualAttackMaxNumber),
permits matching flail pairs (IsEnableDualWielding,2470), and excludes the ordinary
feint/shieldpoke/mounted Hard2 routes already scoped for axes/hammers. All existing
ordinary stance, special-category, catalyst, built-in-pair and throw guards remain.

`flailCatalog` derives60 complete definitions from the shared63-entry template by
omitting the three fifth-stage ordinary/offhand MoveIds. The compile-time
`omitCatalogMoves` helper checks exact membership and expected size; missing or
duplicate omissions cannot create default entries or truncate another move.
Powerstance L1#4 remains present. The graph bridge independently generates all
four chains from HKS limits and rejects disagreement with this membership.

Seven base names qualify: Nightrider Flail, Flail, Chainlink Flail (1300), Family
Heads(1302), Bastard's Stars(1301), Serpent Flail(1320), Golden Order Flail(1330).
Their43 variants share five exact-variation sets. All non-base variations inherit
the ordinary1300 rows in this regulation version. These names are regression
expectations, not a production allowlist.

The independent raw XML snapshot covers60 moves/78 hits and all12 channels, timing,
sources, indices and behavior/attack/alias provenance. The three-hit powerstance
finisher keeps all its hits and50% status multipliers; repeated or multi-source
events are not reduced to a single representative hit. Successful criticals retain
their explicit victim-directed304 context, resolving flail rows even when their
event data aliases axe clips.

Re-extracting a workspace copy of the existing746752-byte `a34.tae` recovered aliases
missing from retained XML. All118 existing clips' attack signatures match. The
current input view/test cache is `build/development-xml/flails`, linking retained
parameters/messages/other TAEs and fresh a30/a33/a34 directories. Read as input
only; the original retained data and earlier views remain intact. No additional
game archive is needed.

Current coverage: **42 base weapons /246 weapon-affinity variants /22 shared sets**,
five families containing315 definitions/381 hits:290 graph-derived definitions and
25 representative catalog criticals. Raw-data, graph/default, JSON, UI and optimizer
regressions cover every supported variant. Native target eligibility, critical
bonuses, skills, special-category routing and mixed-pair calculations keep their
previous limits.

## Ordinary greatswords, greataxes and great hammers (2026-10-05)

These three families use an authored stance signature of 2/12 rather than the
previous 0/10. The current import gate adds these complete profiles:

| Family | Weapon type | Motion category | Behavior family | Stance positions |
| --- | --- | --- | --- | --- |
| Greatsword | 5 | 25 | 300–399 | 2/12 |
| Greataxe | 19 | 32 | 1500–1599 | 2/12 |
| Great hammer | 23 | 35 | 1200–1299 | 2/12 |

All still require special category zero, catalyst zero, built-in-pair zero and
throw enabled. `ordinaryWeaponMotionPosition` shares the expected positions
between import membership and category resolution. Heavy special categories
require explicit context; other poses are not accepted by relaxing the guard.
The existing generic resolver remains available with explicit categories.

The exact local HKS gives four ordinary/offhand stages (2355) and three matching
powerstance stages (2369). Matching pairs are admitted at2430/2462/2466;
IsEnableSpecialAttack, shieldpoke selection and IsEnableRideAttackHard2 exclude
additional ordinary feint/shieldpoke/Hard2 membership for these categories.
`greatswordCatalog` omits powerstance L1#4 from the60-entry flail membership;
greataxes and great hammers share that59-entry vocabulary and existing phase
filters. The graph bridge independently checks all four chain memberships and
derives the54 noncritical definitions for each class from named states.

The position policy is a scoped inference from the retained weapon signatures,
the exact HKS category dispatch and authored graph selectors using weapon
category offsets13/16. It is not a recovered native pose selector. The pinned
[Smithbox field annotations](https://github.com/vawser/Smithbox/blob/3411b8bb03a8e7ca20e04a11939d1e577d392d4f/src/Smithbox.Data/Assets/PARAM/ER/Param%20Annotations/English/EQUIP_PARAM_WEAPON_ST.json)
name motion position fields but do not establish complete native routing. The
real graph/default comparison now verifies every imported hit for the exact
ordinary2/12 profiles; this does not claim arbitrary poses, effects or specials.

Thirteen greatswords add73 variants in8 groups; eight greataxes add80 variants in2
groups; thirteen great hammers add121 variants in5 groups. Names are expectations
in tests, not a production allowlist. Alabaster Lord's Sword (303) and Beastclaw
Greathammer (1201) have eight ordinary heavy overrides each, which select their
exact rows; other hits inherit their hundred-family rows. The other new exact
variations have no ordinary overrides in this data. Weapon names, scaling, AR
and passive status do not control grouping.

Five independent raw XML snapshots verify the three bases plus both override
variations, including all12 channels, event timing, source/index, alias origin and
explicit behavior/attack IDs. The three base catalogs contain67/71/81 ordered
hits. New snapshot behavior IDs explicitly distinguish inherited and exact rows,
so an override with equal MV numbers cannot hide incorrect behavior selection.
JSON regressions preserve every exact variation and pointer sharing; the UI
evaluates every move/hit for all520 supported weapon-affinity variants.

Only workspace copies of existing a25/a32/a35 TAEs (806464/812896/753200 bytes)
were freshly unpacked. All140/130/118 previously extracted clips have unchanged
attack signatures; restored aliases use authored TAE references. No additional
game archive or original input mutation was needed. The current read-only
junction view/test cache is `build/development-xml/heavy`, with fresh a25/a30/a32/
a33/a34/a35 and retained parameters/messages/other TAEs. Never unpack or delete
recursively through this view.

Current coverage: **76 base weapons /520 weapon-affinity variants /37 shared sets**,
eight base families containing492 definitions/600 hits:452 graph-derived
definitions and40 representative catalog criticals. All67 Catch2/CTest cases
pass; app/core-dev/tests build. Fresh development output is
`build/development-game-data/11711000.graph-heavy.json`, verified using the
existing save/load path. Skills, native target eligibility, critical bonuses,
special-category priority and mixed-pair calculations retain their previous limits.

## Ordinary katanas and curved greatswords (2026-10-06)

Katanas (weapon type13/motion29/behavior900–999) use the existing ordinary0/10
stance signature. Curved greatswords (type11/motion40/behavior800–899) use the
audited2/12 heavy signature. Both production profiles retain special0, catalyst0,
built-in-pair0 and enableThrow1. `ordinaryWeaponMotionPosition` now includes40;
automatic heavy category policy rejects its special categories just as it does
for25/32/35. This extends the scoped ordinary policy, not native arbitrary pose
or special-category priority inference.

The exact local HKS gives katanas5 ordinary/offhand stages and4 powerstance stages,
and curved greatswords4/3 (GetAttackMaxNumber/GetDualAttackMaxNumber,2355–2374).
Matching pairs are allowed at2446/2490; the katana/Wakizashi mixed-pair exception
in the script remains outside the matching-pair calculator. Neither ordinary
category is in IsEnableSpecialAttack, the shieldpoke selection, or
IsEnableRideAttackHard2. Thus no ordinary feint/shieldpoke/Hard2 MoveId is added.
`katanaCatalog` shares the63-entry ordinary vocabulary/phase filters;
`curvedGreatswordCatalog` shares the59-entry heavy vocabulary. Graph import
independently derives/checks all four chain memberships, all movement/jump/mounted/
whiff states and58/54 noncritical definitions respectively.

Six katanas qualify: Star-Lined Sword and Uchigatana(900), Hand of Malenia(902),
Rivers of Blood(906), Moonveil(905), Dragonscale Blade(910). Their18 variants share5
exact-variation sets. Nine curved greatswords qualify: Dismounter, Monk's
Flameblade, Beastman's Cleaver, Freyja's Greatsword, Horned Warrior's Greatsword,
Hefty Scimitar(800), Bloodhound's Fang(804), Onyx Lord's Greatsword(805), Morgott's
Cursed Sword(810). Their69 variants share4 sets. These names are independent
regression expectations, not a production allowlist. Special-category versions
remain unsupported by complete import.

Bloodhound's Fang has four ordinary namespace1 jumping overrides (judges150/160/
350/360); other ordinary hits inherit800 rows. Its1H jumping R2 uses142 physical,
magic, fire and lightning MV, but145 holy MV in this retained regulation. These
channels are preserved exactly; no weapon exception or correction is inserted.
The other new non-base variations have no ordinary overrides in this data.

Three independent raw XML snapshots cover the katana base(63moves/72hits), curved
greatsword base(59/73) and Bloodhound variation(59/73). The ExpectedHit test fixture
now accepts explicit values for the four nonphysical damage channels, in addition
to physical/status MV and behavior/attack IDs. Each new snapshot specifies all
five damage channels independently; old uniform snapshots keep their existing
expectations. Thus a channel difference cannot disappear behind a physical-only
comparison or normalization. Every new exact variation is compared with its
correct raw snapshot reference through GameData and JSON sharing.

Workspace copies of704160-byte a29 and635216-byte a40 were freshly unpacked;
all122/107 previously extracted clips' attack signatures match. Fresh aliases
resolve through their authored targets, including existing a25/a30 data. No
additional game archive or original input mutation was needed. The current
read-only junction view/test cache is `build/development-xml/blades`, adding fresh
a29/a40 to the previous heavy view. Do not unpack or recursively delete through
these junctions.

Current coverage: **91 base weapons /607 weapon-affinity variants /46 shared sets**,
ten base families containing614 definitions/745 hits:564 graph-derived
definitions and50 representative catalog criticals. Development output is
`build/development-game-data/11711000.graph-blades.json`; native eligibility,
critical bonuses, skills, special-category priority and arbitrary mixed-pair
calculations retain their previous limits.

## Ordinary colossal swords and colossal weapons (2026-10-06)

Colossal swords use weapon type 7, motion category 26 and behavior variations
400–499. Colossal weapons use type 41, category 31 and variations 2300–2399.
Both require the audited ordinary stance positions 2/12, special category 0,
catalyst 0, built-in-pair 0 and enableThrow 1. The shared stance helper now
includes both categories; automatic heavy-special rejection uses that same
helper. This preserves the scoped signature policy without a second category
list or weapon-specific routing exceptions.

The exact local HKS gives three ordinary/offhand stages and three matching
powerstance stages (GetAttackMaxNumber/GetDualAttackMaxNumber, 2353–2374).
Matching pairs are allowed at 2434/2458; ordinary special category 0 excludes
additional feint, shieldpoke and mounted Hard2 membership. Both 56-entry catalogs
derive from the existing heavy vocabulary by omitting ordinary/offhand stage 4.
Graph import independently derives and checks all four chains, yielding 51
noncritical definitions per class. The five representative successful-critical
definitions retain their existing target-context limitation.

Five swords qualify: Watchdog's Greatsword and Troll's Golden Sword (400), Royal
Greatsword (406), Grafted Blade Greatsword (407), Maliketh's Black Blade (408).
Their 29 affinity variants share four exact-variation sets. Eleven colossal
weapons qualify: Anvil Hammer, Dragon Greatclaw, Troll's Hammer and Devonia's
Hammer (2300), Watchdog's Staff (2301), Envoy's Greathorn (2306), Staff of the
Avatar and Rotten Staff (2307), Fallingstar Beast Jaw (2308), Duelist Greataxe
and Rotten Greataxe (2312). Their 35 variants share six sets. Names remain
independent test expectations; production selects complete parameter signatures.

Colossal weapon variations 2301, 2306, 2307, 2308 and 2312 contain respectively
14, 12, 12, 12 and 6 exact ordinary namespace-1 behavior rows. Several reference
attack data outside their own variation: behavior 102301100 selects attack
2300100, while 102301105 selects 2311105. Existing exact/family/common lookup
already handles these authored references. Seven independent raw XML snapshots
cover the sword base (56 moves/97 ordered hits) and all six weapon contexts
(56/112 each), explicitly checking behavior and attack IDs, every MV channel,
event timing, source/index and alias origins. The other sword variations have no
ordinary overrides and compare with the independently snapshotted base.

Workspace copies of a26/a31 (814640/821488 bytes) were freshly extracted; all
135/115 previously extracted clips have unchanged attack signatures. The current
read-only junction view/test cache is `build/development-xml/colossals`, retaining
the previous fresh family inputs and adding a24/a26/a31. Never unpack or delete
recursively through this view. No additional game archive was needed.

Coverage is now **107 base weapons /671 weapon-affinity variants /56 shared
movesets**. Twelve base families contain 726 definitions/954 ordered hits:
666 graph-derived definitions and 60 representative critical mappings. All 67
Catch2/CTest cases pass, including every supported move/hit through the UI and
optimizer. The existing import/save/load path verified
`build/development-game-data/11711000.graph-colossals.json`.

## Twinblade availability gap (2026-10-06)

Ordinary twinblades have type 14, motion category 24, behavior family 1000 and
stance positions 3/13. Their explicit matching-powerstance graph traversal
selects a24/34000, 34010, 34020 and 34030 according to the script's four-stage
limit. The last TAE entry is absent from both retained XML and a fresh extraction
of the 736960-byte a24 binary. A real regression now checks that importing this
explicit chain reports the missing fourth stage, preserving the first three
without treating the partial result as a complete supported weapon moveset.

The graph and script limit alone do not establish runtime chain reachability.
The exact HKS includes transition/chain-reset conditions, but their native
meaning has not established why this final entry is absent. TAE effect 45 is
counter frames and event type 224 controls turn speed; neither proves a shorter
combo. Automatic twinblade import remains disabled until that routing is
explained. Do not arbitrarily trim to three stages or substitute another clip.
Euporia's exact behavior variation 1010 also contains its own ordinary rows and
needs an independent snapshot once membership is established.

## Ordinary halberds and scythes (2026-10-09)

Halberds use weapon type 29, motion category 38 and behavior family 1800;
scythes use type 31, category 50 and family 1900. Both require their authored
stance positions 3/13, special category 0, catalyst 0, built-in-pair 0 and
enableThrow 1. The shared stance helper now includes only these audited pole
profiles; twinblade 3/13 routing remains excluded. Special categories with these
poses require explicit context rather than acquiring ordinary priority rules.
This is a scoped policy inferred from retained signatures, exact HKS category
dispatch and graph selectors, not recovered arbitrary native pose execution.

The exact local HKS gives four ordinary/offhand and three matching-powerstance
light stages (2355/2369), with matching pairs admitted at 2486/2502. Neither
ordinary category adds feints, shieldpoke or mounted Hard2. Halberds share the
59-entry heavy vocabulary. Scythes have 57 damaging moves: their a50/38100 and
39100 clips exist and contain only release judges 635/735, with no charging
attack judges 630/730. Their catalog therefore omits the two non-damaging mounted
charging entries while retaining both charged release moves. The importer still
rejects a requested absent phase. Tests explicitly verify the raw event counts,
release judges and missing-phase failures; absence does not become a fabricated
zero-MV hit or relaxed parser behavior. Charging time and repeating-hit execution
remain outside calculation.

Eight halberds qualify, all variation 1800: Pest's Glaive, Ripple Crescent
Halberd, Vulgar Militia Saw, Glaive, Vulgar Militia Shotel, Gargoyle's Halberd,
Gargoyle's Black Halberd and Spirit Glaive. Their 68 affinity variants share one
set. Three scythes qualify: Grave Scythe (1900), Halo Scythe (1902) and Winged
Scythe (1906), with 15 variants sharing three sets. The latter two variations
have no ordinary namespace-1 overrides and use family 1900 hit data. Names are
independent test expectations only. Special-category weapons, including the
weapons named Halberd and Scythe, remain outside automatic complete import.

Independent raw snapshots cover every channel, timing, source/index, behavior
and attack ID for halberds (59 moves/69 hits) and scythes (57/67). Halberd whiff
and critical clips alias a36; their behavior variation remains 1800. Scythe
successful criticals retain their own a50 source. Graph/default comparison,
serialization sharing, move evaluation, UI and optimizer checks include all new
variants. Graph independently derives 54/52 noncritical definitions; critical
target-query regressions now verify 70 representative datasets across 14 classes.

The retained a38/a50 directories were empty and their original binaries absent.
A workspace copy of the installed `c0000.anibnd.dcx` (6899216 bytes) was extracted
under `build/mapping-research/pole-tae-audit/`, without touching retained data or
installed game files. Its 506358951-byte extraction can serve future audits.
All 1721 previously staged clips across twelve supported families plus a24 have
identical attack signatures in the new archive, and a24/a26/a31 raw binary hashes
match previous audit copies. The new a38/a50 binaries are 732352/715920 bytes.
The survey's zero old a38/a50 clips is missing comparison input, not evidence of
comparison against a prior complete extraction. Audit scripts and logs remain
ignored development aids, independent of C++ importer output.

The current read-only input view/test cache is `build/development-xml/poles`;
it preserves earlier targets and adds fresh a36/a38/a50 junctions. Never unpack
or delete recursively through it. No new production archive dependency was
introduced: `c0000.anibnd.dcx` was already in `needed_packed_files`.

Coverage is now **118 base weapons /754 weapon-affinity variants /60 shared
movesets**. Fourteen base families contain 842 definitions/1090 ordered hits:
772 graph-derived definitions and 70 representative successful-critical mappings.
The existing import/save/load path verified
`build/development-game-data/11711000.graph-poles.json`. Existing target-context,
critical-bonus, skill, mixed-pair and effect limitations still apply.

## Ordinary spears and shieldpoke context (2026-10-09)

Ordinary spears require type 25, motion category 36, behavior 1600–1699,
stance positions 3/13, special category 0, catalyst 0, built-in-pair 0 and
enableThrow 1. Exact HKS gives four ordinary/offhand and three matching-pair
lights (2355/2369), admitting matching spears at 2474. They add shieldpoke
membership, with no feint or mounted Hard2 extension. The 60-entry catalog
extends the existing 59-entry vocabulary by one explicitly named
`Shieldpoke (normal guard)` sample. No new MoveId, model or serializer is needed.

The HKS `GetAttackRequest` guard branch (1205–1213) selects a right-hand guard
attack for spear, great spear, rapier and heavy rapier categories when the
one-handed guard request is active. `ExecAttack` (2254–2267) obtains the left
guard style from env(278), overrides it for torch/dueling shields, supplies
`IndexAttackWhileGuard`/`IsAttackWhileTorchGuard`, and enters
`W_AttackRightWhileGuard`. Its activation callback (8288–8290) only sets attack
hand right and guard hand left. Generic graph traversal now accepts this exact
callback, an unbound wrapper and empty other callbacks, alongside the previously
audited whiff wrapper. Arbitrary script execution remains unsupported.

`BehaviorGraph::resolveWeaponShieldpoke(definition, weapons, weaponId,
guardStyleIndex, torchGuard)` requires explicit caller context, a shieldpoke ID,
ordinary one-hand stance, an eligible category and zero special category. Valid
guard indices are 0–3; torch requires index 2. It follows the actual named state
through the two selectors with the corresponding named variables. Missing or
malformed states, wrong offsets, bad context, unsupported scripts and cycles
fail. Complete catalog import explicitly selects index 0/non-torch and retains
the normal-guard label. GameData does not infer the left guarding weapon or
native eligibility from that entry.

Real regressions resolve all five branches: normal (30900), heavy (30901), small
(30902), torch (30903), dueling (30904). Both base 1600 and exact variation 1615
have identical resolved attack data across those branches in this archive;
selected clip provenance remains distinct. Comparison normalizes only those
clip IDs, then checks full serialized hit equality. Portable tests use different
branch animations and verify that supplied context actually changes selection,
while rejecting missing context, bad categories/poses/catalysts/specials and
unsafe script paths. This proves scoped routing and hit-data equivalence, not
universal guard-weapon or runtime request eligibility.

Ten bases qualify: Crystal Spear, Clayman's Harpoon, Celebrant's Rib-Rake,
Torchpole, Inquisitor's Girandole, Iron Spear, Rotten Crystal Spear and
Bloodfiend's Fork (1600), Bolt of Gransax (1602), Spear (1615). Their 70 variants
share three sets. Variation 1615 has 66 ordinary exact rows, while 1602 has none.
Independent full raw snapshots for 1600 and 1615 cover all 60 moves/69 ordered
hits, every MV channel, timing, source/index, aliases and explicit behavior/attack
IDs. Production grouping still uses complete routing signatures; names are
only independent expectations. The raw survey verifies StateInfo 0 for every
selected spear attack event. The recovered archive supplies a36 through the
existing input-view target, so no additional copying or unpacking was needed.

Current coverage: **128 base weapons /824 weapon-affinity variants /63 shared
movesets**. Fifteen base families contain 902 definitions/1159 ordered hits:
827 graph-derived definitions and 75 representative successful-critical mappings.
All 68 Catch2/CTest cases pass (61 with game XML only, 58 portable); application,
core development executable and tests build. The existing serializer verified
`build/development-game-data/11711000.graph-spears.json`. Current read-only
input view/test cache: `build/development-xml/spears`.

## Great-spear conditional events (2026-10-09; previous milestone)

Great spears were audited but remain outside automatic complete import. Their
ordinary type 28/category 37/poses 3/13 signatures include Bloodfiend's Sacred
Spear (1700), Lance (1708) and Mohgwyn's Sacred Spear (1711). However, a37 attack
clips contain namespace-5 judges 5000/5001/5002 alongside ordinary strikes,
with AttackIndex 2 and StateInfo 187. For example, 5000 resolves common behavior
500000000→attack1703001 (200 physical MV), alongside ordinary judge 0 with
100 physical MV. Lance also has 73 exact ordinary override rows.

Before the conditional-event milestone, `AttackEvent` did not retain StateInfo,
so summing these raw events would have treated gated attacks as always active. The pinned
[DSAnimStudio simulation](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/TaeEditor/TaeActionSimulationEnvironment.cs#L431)
checks positive StateInfo values against an activation map before spawning hits.
This supports preserving and evaluating conditional activation; it does not
establish the game's native meaning of StateInfo 187. The connection to
Serpent-Hunter-specific attack data is an inference from the referenced rows,
not an implemented runtime condition. Do not infer an activation rule from a
weapon ID or discard namespace 5 globally.

Ignored raw great-spear survey snapshots contain all events (57 moves/115 hits)
and are research records, not valid unconditional calculation expectations.
The independent StateInfo audit finds 42 events gated by 187, 67 with zero and
six without the field. Do not silently include, skip or flatten unknown strikes.

### Implemented conditional evaluation (2026-10-09)

The existing `AttackEvent` now contains optional raw `stateInfo`, retained through
alias resolution, behavior resolution and the existing reflection serializer.
Missing legacy fields remain readable. Malformed/overflowed or duplicate XML
StateInfo parameters reject the animation. Missing/zero/negative values are
ungated; each positive ID requires a caller-supplied `MoveContext::state_info`
boolean. Unknown IDs return an error identifying the condition, animation and
judge. No weapon or behavior-namespace exceptions supply flags implicitly.

`calculate_move` and `MoveEvaluator` apply this context before source/MV checks.
Inactive events retain their ordered hit identity and original `ResolvedHit`,
with `HitAttack::active == false` and zero channel/total powers. This avoids
shifting selected-hit indices. The evaluator owns its flags for the duration
of evaluation; changing flags during an animation is outside this model.
Both optimizer paths use `MoveObjective::context` and the same condition lookup,
before accumulating their prepared MV multipliers. Selected-hit objectives
require only that selected hit's flags. The current UI has no implicit flags:
whole conditional moves display the unknown-condition diagnostic and are
excluded from optimization; selected ungated optimization hits remain valid.

Portable regressions cover active/inactive/unknown and multiple IDs, legacy
JSON, aliases, strict parsing, all channels, stable hit indices, second-weapon
requirements, evaluator context ownership, both optimizers and UI diagnostics.
Real a37 regressions explicitly import 30000/30010/30900 for variations
1700/1708/1711, verifying all 18 ordered events, raw conditions, exact/family/
common behavior resolution, timing/provenance and every MV channel. These tests
do not add an automatic great-spear profile or claim native eligibility.

The audit uses the prior graph-selected development JSON only for event
selection/provenance, then reads conditions independently from raw XML. All
4,936 events in the existing 63 shared movesets (809 source clips) have absent
or zero StateInfo, across all fifteen supported families. Existing coverage
remains 128 bases /824 affinities /63 groups. Research script/log:
`build/mapping-research/audit_state_info.py`, `build/state-info-audit.log`.
Current read-only view/cache: `build/development-xml/conditions`, preserving every
spears-view target and linking a37 to the already recovered full archive. No
archive was unpacked or copied; never unpack/delete through this junction view.
The development JSON is `build/development-game-data/11711000.graph-conditions.json`.
All **74** Catch2/CTest cases pass (67 with game XML only, 63 portable). The
application, core development executable and tests build. Existing CLI semantic
save/load verification passes for 3296 weapons /824 with movesets; a separate
complete-data comparison, normalizing pointer IDs and stripping only new StateInfo
fields, confirms no other serialized data changed from the spear milestone.
Automatic great-spear support still needs verified native activation semantics
and a usable gameplay context; the importer/calculator can now evaluate explicit
research scenarios safely.

## Ordinary great spears and native arena effect (2026-10-10)

StateInfo 187 now has native activation evidence. In the retained regulation,
SpEffectParam **1908 is the sole row with stateInfo 187**. A read-only copy of
the installed `event/m16_00_00_00.emevd.dcx` was decompressed under ignored
`build/mapping-research/greatspear-event-audit/`. The native Rykard map contains:

- Event **16002810**, instruction 27: grants SpEffect 1908 to player entity
  10000 during boss-fight activation, before enabling AI/health display.
- Event **16002840**, instruction 6: grants the same effect to player 10000
  inside region 16002164 while the boss-defeated flag 16000800 is unset.
  This is a local, repeating region check.
- Event **16002800**, instruction 12: removes effect 1908 from player 10000
  after boss defeat.

These events contain no equipped-weapon predicate and no event parameter
substitutions. The audit reader uses the primary
[SoulsFormats EMEVD layout](https://github.com/soulsmods/SoulsFormatsNEXT/blob/master/SoulsFormats/Formats/EMEVD/EMEVD.cs)
and pinned
[DarkScript3 ER instruction definitions](https://github.com/AinTunez/DarkScript3/blob/733f024253930b70db8e3d0039c4b53b6ee0a2d4/DarkScript3/Resources/er-common.emedf.json).
The [mod author's Serpent-Hunter instructions](https://www.nexusmods.com/eldenring/mods/642?tab=description)
independently identify passive effect 1908 as enabling its boss moveset outside
the arena. That supports the effect's gameplay meaning, not a weapon-ID rule.
The native evidence plus SpEffectParam supplies the explicit activation scenario;
it does not prove every gated hit contacts every weapon/target combination.
Native collision geometry, equipment eligibility and target filters remain
outside the calculated raw MV-adjusted powers.

The research-only reader/log are `build/mapping-research/audit_greatspear_event.py`
and `build/greatspear-event-audit.log` (78 events/1770 instructions). Packed input
is 9344 bytes, SHA256
`69c78bd1f62bb7659cba17768b949f7a112928ff7ed472079b1ce2ad2685ae07`;
decompressed EMEVD is 90976 bytes, SHA256
`202aa6ad15a94a1b612780a4551613cd06a0e1c22705ce04d320e71711635f68`.
Witchy handled KRAK/Oodle on the workspace copy. This adds no production event
archive dependency or EMEVD parser, and no installed file or retained XML changed.
The existing 506 MB full TAE extraction already supplies a37.

`calculator::MoveEnvironment` now provides `ORDINARY_COMBAT` and
`RYKARD_ARENA_EFFECT`. `move_context(environment)` decides only verified flag
187 (false/true respectively); other positive IDs still require explicit
caller decisions. Both UI tabs select these environments, defaulting to ordinary
combat, and propagate the same context into the existing evaluator and optimizer
objective. Inactive hits display their original number and zero powers. The
low-level evaluator keeps its default empty context for strict conditional-data
checking. No weapon-ID/name or namespace/AttackIndex exception was added.

The complete ordinary profile now admits **type 28/motion 37/behavior 1700–1799**
with authored **3/13** stances, special 0/catalyst 0/built-in-pair 0/enableThrow 1.
Bloodfiend's Sacred Spear (1700), Lance (1708), and Mohgwyn's Sacred Spear (1711)
add **15 affinity variants/three shared groups**. HKS defines three-stage ordinary,
offhand and matching-powerstance light chains. `greatSpearCatalog` omits the
three fourth ordinary/offhand lights from the existing spear vocabulary, retaining
shieldpoke and phase filters: **57 moves/115 raw events**. Ordinary combat has
73 active events and 42 gated/inactive events; the arena-effect scenario enables
all 115. Those counts concern ordered imported events, not confirmed target contacts.
Lance has 73 exact ordinary override rows; 1711 inherits base behavior rows.
Special-category great spears, including Serpent-Hunter, remain unsupported by
this complete ordinary routing policy.

Independent full raw snapshots `tests/expected_greatspear_1700_moves.inc` and
`expected_greatspear_1708_moves.inc` verify every move/channel/timing/source/index,
StateInfo, aliases and behavior/attack ID. All three exact groups compare with
their corresponding snapshot through JSON. Every great-spear variant evaluates
all moves in both environments while retaining hit indices. Graph tests check
all three complete light chains, 52 noncritical definitions, five representative
critical target queries and all five shieldpoke branches for base 1700/Lance.
Portable tests check the environment helper, unknown-ID rejection, Stats hit
display and both optimizer buttons' scores through the existing GameData serializer.

Coverage is **131 bases/839 variants/66 shared sets**, across sixteen families.
Their base vocabularies contain **959 definitions/1274 ordered hits**:
879 graph-derived definitions plus 80 representative critical mappings.
All **76** Catch2/CTest cases passed in **76.70s** (69 with game XML only,
65 portable), after a 74-case baseline; app/core development executable/tests
build successfully. The existing CLI verified save/load of
`build/development-game-data/11711000.graph-greatspears.json`
(13908093 bytes/3296 weapons/839 with movesets). A complete-data comparison
normalizing shared-pointer IDs and removing only the three newly assigned sets
confirms all previous serialized values and shared identities are unchanged
(`build/mapping-research/compare_greatspear_data.py`,
`build/greatspear-data-comparison.log`). Bundled JSON/user edits were preserved.
The current read-only input view/cache remains `build/development-xml/conditions`.

## Ordinary whips and non-throw membership (2026-10-10)

The complete ordinary whip profile requires **wepType39/motion43/behavior2000–2099**,
stances **0/10**, special0/catalyst0/built-in-pair0 and **enableThrow0**.
`SupportedCatalog` now records its required `enableThrow` value, defaulting to1
for every previous profile. This preserves the full admission guards while
supporting a non-throw class; it does not relax the flag for other weapons.
No weapon-ID/name exception, replacement model or serializer was added.

Local HKS `GetAttackMaxNumber`/`GetDualAttackMaxNumber` at2355/2369 specifies
four ordinary/offhand and three matching-powerstance light stages for whips.
`IsEnableDualWielding` admits matching whips at2498. Whips are absent from
the script's feint admission (`IsEnableSpecialAttack` at2390) and shieldpoke
category list at1205–1213. Their disabled throw parameter removes the entire
critical/whiff family from complete automatic membership. `whipCatalog` derives
from the existing four-light/three-pair scythe vocabulary, omitting all six
throw/whiff IDs: **51 damaging moves/58 ordered events**. As with scythes,
mounted38100/39100 contain only release judges635/735; no charging630/730 hit
exists. Both charged releases remain supported, and explicit requests for the
absent phases still fail instead of producing zero or shortening an imported move.

Six bases qualify: **Whip, Thorned Whip, Hoslow's Petal Whip, Tooth Whip (2000),
Magma Whip Candlestick (2003), Giant's Red Braid (2006)**. Their **42 affinity
variants** share **three exact behavior groups**. Base2000 has79 namespace1
rows;2003/2006 have none and inherit ordinary base rows. The independent raw
audit verifies identical complete resolved ordinary hit data across all three
groups and StateInfo0 on every selected event. They retain distinct variation
metadata/pointers. Urumi uses special category226 and remains outside this
complete ordinary profile.

Independent snapshot **`tests/expected_whip_2000_moves.inc`** covers all51moves/
58hits, all12MV channels, timings, source/index, StateInfo and behavior/attack IDs.
Research script/log: `build/mapping-research/audit_whips.py`, `build/whip-audit.log`.
Existing portable cases now check whip membership, retained releases and every
admission guard using an arbitrary synthetic weapon ID. Real-data tests check
absent-phase failures, complete values/sharing through existing JSON and evaluation
for all42variants. The existing graph independently derives all51definitions,
with complete light-chain membership checks and strict graph-failure rejection.
The full UI workflow and both optimizer paths cover the expanded881variant set.

The retained a43 XML directory has **zero clips**. The new read-only642-TAE view
**`build/development-xml/whips`** preserves every previous conditions-view target
and links a43 to the already recovered506MB full archive. No archive was copied
or unpacked, and no installed/retained file changed. Zero old clips is missing
comparison input, not validation against a previous complete whip extraction.
The staging helper is `build/mapping-research/stage_whip_view.ps1`; an initial
PowerShell Target-property indexing mistake created only empty workspace
directories, then was corrected before the junctions/configuration were created.
Never unpack or recursively delete through the input view; never call
`generate_game_data` during development.

Coverage is **137 bases/881 variants/69 shared sets**, seventeen families with
**1010 definitions/1332 ordered hits**:930graph-derived/80representative critical
mappings. Baseline76/76 passed in77.17s; final76/76 passed in81.81s, with the
same Catch2 executable and optional-input counts (69game XML only/65portable).
App/core development executable/tests build successfully. The existing CLI
verified semantic save/load of
`build/development-game-data/11711000.graph-whips.json`
(13998012bytes/3296weapons/881with movesets). A full-GameData comparison normalizes
shared-pointer IDs and removes only the three newly assigned whip sets,
confirming all previous serialized values and sharing are unchanged
(`build/mapping-research/compare_whip_data.py`, `build/whip-data-comparison.log`).
Bundled JSON and prior development outputs were preserved.

## Ordinary thrusting swords and neutral feints (2026-10-10)

The complete ordinary thrusting-sword profile requires **type15/motion27/
behavior500–599**, stances **0/10**, special0/catalyst0/built-in-pair0/enableThrow1.
**Cleanrot Knight's Sword, Rapier and Antspur Rapier** add **39 affinity variants**
sharing one base500moveset. Base500 has109namespace1behavior rows. Names/IDs remain
independent test expectations, not production mapping rules. Specials (including
Estoc, Rogier's Rapier and Frozen Needle) and Carian Sorcery Sword's catalyst
routing remain outside complete automatic import.

HKS `GetAttackMaxNumber` at2359 gives six ordinary/offhand lights;
`GetDualAttackMaxNumber` at2371 gives four matching-pair lights, admitted at2438.
`thrustingSwordCatalog` extends the existing neutral vocabulary with the three
sixth light stages, shieldpoke and four feints: **71moves/83ordered events**,
all ungated. Native feint evidence comes from the four heavy Start callbacks:

| Callback | Selected state | Graph offset | Registered clip |
| --- | --- | --- | --- |
| AttackRightHeavy1Start (8456–8457) | AttackRightSpecial1 | 13 | 30950 |
| AttackRightHeavy2Start (8486–8490) | AttackRightSpecial2 | 13 | 30955 |
| AttackBothHeavy1Start (8996–8997) | AttackBothSpecial1 | 16 | 32950 |
| AttackBothHeavy2Start (9026–9034) | AttackBothSpecial2 | 16 | 32955 |

Each transition requires `IsEnableSpecialAttack`, held R2/evasion inputs and
effect100290's window; the second-stage callbacks have a special953redirect.
Zero-special thrusting weapons use the states above. `feintAttackState` names
the requested phase. **`resolveWeaponFeintMove(definition,weapons,weaponId)`**
uses the existing ordinary category/stance policy, admitting only the audited
27/39 categories with special0, and resolves through the actual graph.
`resolveSupportedCatalog` now invokes it for all four IDs, rejecting missing
states/offsets instead of keeping a catalog seed. No hardcoded clip arithmetic,
per-weapon rule or HKS interpreter was added. This maps a selected feint;
input eligibility, charge timing, left-two-handing and temporary effects are
not simulated. Effect100290 is a native script query, not a new TAE StateInfo
flag supplied to `MoveContext`.

The graph directly registers the four CMSG states for both27and39. Independent
raw snapshot `tests/expected_thrusting_500_moves.inc` checks all71moves/83events,
all12channels/timing/source/index/StateInfo/behavior+attack IDs. A separate
`tests/expected_heavy_thrusting_feints_600_moves.inc` checks the four a39feints:
125physicalMV one-handed,130two-handed,100status. Variations600/601/602 all
inherit these base hits; they were surveyed separately. Research scripts/logs:
`build/mapping-research/audit_thrusting.py`, `build/thrust-audit.log`,
`audit_thrust_graph.py`, `build/thrust-graph-audit.log`. Initial full heavy audit
stopped on absenta39/34030 (`build/thrust-audit-initial.log`); subsequent heavy
snapshots deliberately cover only four independently supported feints.

**Heavy thrusting swords are not enabled for complete automatic import.** Their
ordinary signature has asymmetric positions **3/10**, now correctly represented
by `ordinaryWeaponMotionPosition`. Five-stage ordinary/offhand chains and four
feints resolve for Bloody Helice602, Dragon King's Cragblade601 and Queelign's
Greatsword600. But HKS permits four powerstance stages while `AttackDualLight4`
has **no category39registration**, and the recovered full TAE archive has
**noa39/34030**. Real regressions require the fourth-stage/no-category diagnostic,
reject a seed34020substitute and refuse complete catalog resolution. This differs
from twinblades, whose fourth-stage graph registration exists but its TAE is absent.
Neither chain is silently shortened; transition/reachability evidence is still needed.

Two new portable cases test non-arithmetic graph clips, preserved filters,
complete-bridge failure, scope/stance/catalyst/special rejection and feint stance/
scores in both optimizers (whole move and selected hit). One new real case checks
all24feints across the six ordinary thrusting/heavy-thrusting base weapons and
the heavy-chain diagnostic. Existing real selector tests now cover30guard samples,
including all five branches for27and39; independent raw XML confirms identical
events after normalizing only clip provenance. Critical target queries add the
five representative category27contexts. Full import/serialization/calculator/UI
tests cover every move/channel for all920supported variants.

The current read-only642-TAE view/cache is **`build/development-xml/thrusting`**.
It preserves every whips-view target and links a27/a39 to the already recovered
full506MBarchive (`build/mapping-research/stage_thrust_view.ps1`). Retained a27/a39
have zero XML clips, so no prior complete comparison input exists. No archives
copied/unpacked, installed/retained files modified or production dependencies added.
Never unpack/delete recursively through views or call `generate_game_data` during
development.

Coverage: **140bases/920variants/70shared sets**, eighteen base vocabularies with
**1081definitions/1415ordered hits**, **996graph-derived/85representative critical
mappings**. Baseline76/76passed in81.62s; final**79/79passed in84.53s**
(71gameXML-only/67portable), same existing Catch2 executable. App/core development
executable/tests build successfully. Existing CLI semantic save/load verified
`build/development-game-data/11711000.graph-thrusting.json`
(14041510bytes/3296weapons/920with movesets). Complete comparison normalizing
shared-pointer IDs/removing only the new category27set confirms all previous
serialized values and shared identities unchanged (`compare_thrust_data.py`,
`build/thrust-data-comparison.log`). Bundled JSON/user edits and previous outputs
were preserved.

## Ordinary curved swords (2026-10-10)

The complete ordinary parameter signature **type 9/motion 28/behavior 700–799,
stances 0/10, special 0, catalyst 0, built-in-pair 0, enableThrow 1** adds seven
bases and 67 affinity variants. Falchion, Bandit's Curved Sword, Scavenger's
Curved Sword, Serpent-God's Curved Sword, Grossmesser and Spirit Sword share 700;
Magma Blade retains a separate variation-703 set. Production selects the full
signature rather than names or IDs. Nonzero specials and built-in pairs remain
outside this complete profile.

Exact local HKS establishes six ordinary/offhand light stages, four powerstance
stages, matching-category pair eligibility and curved-sword feint eligibility.
The graph registers category 28 for all four requested feints (offsets
13/13/16/16; clips 30950/30955/32950/32955) and the fourth paired light 34030.
`resolveWeaponFeintMove` extends its existing neutral scope to 28 alongside
27/39, preserving stance/context checks, filters and graph failure behavior.
Native input/charge/effect-window simulation is outside this phase-selection API.
`curvedSwordCatalog` reuses the thrusting vocabulary and omits shieldpoke because
native guard-attack eligibility excludes category 28. The result has **70 moves/
87 ordered hits**, including four feints, with all StateInfo values zero.

Independent raw XML audit resolves aliases and parameter defaults without reading
C++ importer output. Base 700 has 115 namespace-1 behavior rows; 703 has no exact
rows and inherits the family. Their complete resolved hit data matches, while
shared identity remains separate. The tracked raw base-700 snapshot verifies
both variations' full move list, twelve MV channels, timing/order/provenance and
behavior/attack references. Portable graph tests use non-arithmetic category-28
clips and special rejection; real tests cover all seven bases' feints, five
representative throw contexts and every move/channel through calculator,
optimizers, JSON and UI.

The current read-only view/cache **`build/development-xml/curved`** preserves all
thrusting-view targets and replaces empty retained a28 with the already recovered
full archive. No archive was copied/unpacked again. Never unpack or recursively
delete through these directory junctions. Retained a28 has no previous complete
XML comparison input.

Coverage is **147 bases/987 variants/72 shared sets**, nineteen base vocabularies
with **1151 definitions/1502 ordered hits**, **1061 graph-derived/90 representative
critical mappings**. Baseline **79/79 passed in 78.49s**; final **79/79 passed in
81.99s** (`build/curved-tests.log`), existing Catch2 executable and full app/core
build. Fresh existing CLI save/load verifies
**`build/development-game-data/11711000.graph-curved.json`**
(14131480 bytes/3296 weapons/987 with movesets). Full comparison removing only
the new category-28 assignments and normalizing pointer IDs preserves all prior
serialized values and sharing (`build/curved-data-comparison.log`). User edits,
bundled JSON, installed files and retained input were preserved.

## Ordinary great katanas and alias event sources (2026-10-10)

The complete ordinary signature **type94/motion61/behavior6600-6699/stances2,12,
special0/catalyst0/built-in-pair0/enableThrow1** adds Great Katana,
Dragon-Hunter's Great Katana and Reed Great Katana: **27 affinity variants/one
shared set**. Production selects the complete parameter signature rather than
names or IDs. Special953 (Rakshasa's Great Katana) remains outside the profile.

Exact local HKS establishes four ordinary/offhand light stages, three matching
powerstance stages and great-katana feint eligibility. The graph registers four
neutral feints at offsets13/13/16/16 with clips30950/30955/32950/32955, and paired
stage3 at34020. The existing stance helper adds category61's authored2/12 poses;
neutral `resolveWeaponFeintMove` admits27/28/39/61 without relaxing special,
context, offset, state or filter validation. `greatKatanaCatalog` reuses the
curved-sword vocabulary minus fifth/sixth ordinary/offhand stages and paired
stage4: **63moves/75ordered events**, **58graph-derived/five representative
critical mappings**. Requested-phase selection does not simulate native
R2/evasion/effect windows or left-two-handing.

The audit exposed an overly conservative cache rejection: a61/31719 is an
`ImportOtherAnim` alias to **a32/31719**, despite local type1 judges590/509.
Pinned [DSAnimStudio reference-chain resolution](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/DSAProj/DSAProj.cs#L1513-L1544)
returns the referenced animation recursively, and
[playback assigns that animation as its TAE event source](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/NewAnimationContainer.cs#L132-L146).
The existing cache now follows this verified rule: local rows are inactive,
not merged or substituted. Requested IDs and target category/animation/timing
remain distinct. Invalid/missing targets, cycles and failed-load retries retain
their previous checks. Portable regressions cover local type1/type304 rows,
including malformed local events in a nested alias and an empty imported source.

Independent raw audit (`audit_great_katana.py`, `build/great-katana-audit.log`)
uses default-merged parameter rows, recursively resolved TAE sources and all
96 namespace-1 rows in base6600. All imported StateInfo values are zero. The
tracked raw63-move/75-hit snapshot checks every channel, timing/order/provenance,
behavior and attack ID. Portable graph fixtures add non-arithmetic category61
clips, stance/special rejection and a complete light+feint bridge; real tests
cover all three weapons' feints, five representative critical contexts and
all1014variants through JSON, calculator, optimizers and UI.

Current read-only view/cache **`build/development-xml/great-katana`** preserves
all curved-view targets and links empty retained a61 to the already recovered
full archive. No archive was copied/unpacked again. Never unpack or recursively
delete through these views. Baseline **79/79passed in96.96s**; final
**79/79passed in99.72s** (`build/great-katana-tests.log`), same Catch2 executable
and complete app/core build. Coverage **150bases/1014variants/73shared sets**;
twenty base vocabularies total **1214definitions/1577ordered hits**, with
**1119graph-derived/95representative critical mappings**. Fresh existing CLI
semantic save/load verifies
**`build/development-game-data/11711000.graph-great-katana.json`**
(14170473bytes/3296weapons/1014with movesets). Full prior-value/shared-identity
comparison removing only category61 assignments passes
(`build/great-katana-data-comparison.log`); user edits and existing outputs remain.

Light greatswords were surveyed but remain disabled. Ordinary motion60 has
poses3/10 and base6700; mounted Start uses judges638/738, while End uses633/733,
all with simultaneous judge999/AttackIndex1. Judge999 resolves to attack6700999
with nonzero100 MVs and different target/hitbox flags (opposeTarget0, hit1 only).
Old635/735filters missing these events does not mean charged releases have no
hits. Eligibility/target and phase semantics require verification before this
family can produce reliable calculator totals. Do not omit999 by a weapon name
or guess that it contributes unconditional extra AR. Initial partial raw audit
is exploratory; the corrected great-katana audit is independent and complete.

## Ordinary light greatswords and opposing-target eligibility (2026-10-10)

The complete ordinary profile admits **type93/motion60/behavior6700–6799,
stances3/10**, special0/catalyst0/built-in-pair0/enableThrow1. Milady,
Leda’s Sword and Idus Sword add three bases/27 affinity variants sharing6700.
Names and weapon IDs are regression expectations only; production uses the
complete parameter signature. Rellana’s Twin Blades remain excluded by their
special/built-in-pair signature.

Exact HKS and graph audit (`audit_light_greatsword_graph.py`,
`build/light-greatsword-graph-audit.log`) verify five ordinary/offhand lights,
four matching-powerstance lights and the category60 stage registrations.
The shared stance helper includes asymmetric3/10. Base6700 has155 namespace1
behavior rows. `lightGreatswordCatalog` reuses the axe vocabulary, omits two
non-damaging mounted charging entries and selects release judges638/738.
Raw Start38100/39100 places these after its charge window, alongside judge999;
End38110/39110 contains633/733+999. The requested-phase API does not simulate
input/charge duration. No feints or shieldpoke are enabled. **61 moves/153 raw
ordered events** resolve; **56 graph-derived/five representative critical
mappings**, all StateInfo0. Criticals retain existing representative target limits.

Pinned [Paramdex AtkParam fields](https://github.com/soulsmods/Paramdex/blob/ff7245e524329bc3eab00036723d2bd53384cedf/ER/Defs/AtkParam.xml#L480-L500)
identify `opposeTarget` as opposing-target eligibility (default1), separately
from friendly/self targets. The calculator computes opposing-target powers,
so an explicitly false flag must not add AR even when its MVs are nonzero.
`ResolvedHit::opposeTarget` is optional for legacy JSON; default-merged XML
requires exact0/1 values. Existing reflection serialization handles it without
custom code. The shared `attack_hit_active` applies eligibility before StateInfo,
source and MV requirements in evaluation and both optimizer preflights. Inactive
events retain hit indices and contribute zero across all12channels; the existing
UI displays them as inactive. Missing flags preserve previous JSON behavior.
This decides eligibility, not contact, geometry, defense or critical bonuses.

Independent raw audit (`audit_light_greatsword.py`,
`build/light-greatsword-audit.log`) and tracked
`tests/expected_light_greatsword_6700_moves.inc` verify all12MVs, raw provenance,
order/timing, behavior/attack IDs and flags. **127 events can affect opposing
targets; 26 cannot**. Ineligible attack counts6700992:5,997:1,998:3,999:17 all
have raw100 corrections and all three target flags0. Production filters by the
flag, not attack ID/index. Fifty prior great-hammer helper events across five
sets are also ineligible, but already had all-zero MVs; their numeric totals
stay unchanged. Existing great-hammer/Beastclaw expectations now retain false flags.

Two new portable Catch2 cases cover strict field parsing/defaults/overrides,
legacy JSON, ordered inactive hits, source/MV/StateInfo bypass and both optimizer
whole-move/selected-hit scores. Complete graph/JSON/UI checks cover1041variants.
Baseline **79/79passed in96.14s**; final **81/81passed in102.93s**
(`build/light-greatsword-final-tests.log`); existing app/core/tests build passes.
Coverage/benchmarks remain OFF. The642-TAE junction view/cache
**`build/development-xml/light-greatsword`** preserves earlier targets and links
empty retaineda60 to the existing full archive. No new archive was unpacked;
never unpack into or recursively delete through these views.

Coverage now **153bases/1041variants/74shared sets**, twenty-one base
vocabularies with **1275definitions/1730raw ordered hits**,
**1175graph-derived/100representative critical mappings**. Existing CLI semantic
JSON round-trip generated
**`build/development-game-data/11711000.graph-light-greatsword-checked.json`**
(14361941bytes/3296weapons/1041with movesets). Full prior-value/shared-identity
comparison removes only new assignments/target metadata and matches every flag
against raw AtkParam (`build/light-greatsword-final-data-comparison.log`).
Previous outputs, bundled data, retained inputs and user changes are preserved.

## Ordinary thrusting shields and native pairing exclusion (2026-10-10)

The full ordinary signature **type90/motion57/behavior6100–6199/stances3,13**,
special0/catalyst0/built-in-pair0/enableThrow1 admits Dueling Shield, Carian
Thrusting Shield and Ritual Thrusting Shield: three bases/39affinities sharing6100.
Production selects parameter signatures; weapon names/IDs remain independent
regression expectations. Existing classes, serializer and calculation/UI paths
are reused.

Exact HKS gives four ordinary/offhand stages. Its paired combo-limit function
also returnsfour, but `IsEnableDualWielding` explicitly returns-1 for this class
(2518). Combo limits alone do not establish native attack availability.
`excludesMatchingPowerstance` centralizes this audited21/57exclusion;
`resolveSupportedCatalog` skips only the forbidden paired chain, preserving
strict ordinary-chain completeness and rejecting any paired definition.
The shared stance helper includes57’s3/13. `thrustingShieldCatalog` derives from
flailCatalog and removes eight paired entries: **52moves/55raw ordered events**,
**47graph-derived/five representative critical mappings**. Mounted charging
and release remain valid, using630/635 and730/735. Feints/shieldpoke are excluded.

Independent raw audit (`audit_thrusting_shields.py`,
`build/thrusting-shield-audit.log`) resolves all66namespace1behavior rows’
selected events using aliases/default-merged params. Tracked
`tests/expected_thrusting_shield_6100_moves.inc` verifies all12MVs, timing/order,
source and alias/behavior/attack IDs. All selected events have StateInfo0 and
opposing-target eligibility. Backstab-whiff imports a36/31719; small riposte
imports a57/31720. Graph audit (`audit_thrusting_shield_graph.py`,
`build/thrusting-shield-graph-audit.log`) independently confirms fourth-stage
registrations at13/16/14 and mounted Start/End request handling.

These are selected normal attack states. Common HKS guard table enables57in
both styles, so native L1 can guard; the selected offhand attack can use the
skill-free L2 fallback. Guarded two-handed R1 switches to `AttackBothLightGuard`
(1615–1620), a separate state outside the current normal MoveId vocabulary.
Future support needs explicit stance/context semantics; do not substitute
normal R1 or one-handed SHIELDPOKE merely because the class is a shield.

The existing portable complete-bridge case now verifies a synthetic57profile
with non-arithmetic clips and no paired registrations. Forbidden paired
chain/movement/jump definitions reject; missing ordinary chain/movement/mounted
states and missing catalog stages still reject. Full signature mutations and
five representative critical queries are covered. Complete JSON/UI regression
checks **1080ordinary/1041powerstance** optimization eligibility, both optimizer
scores/buttons and all imported move/hit powers. Test counts stay81/73/69for
both inputs/XML-only/portable. Baseline **81/81passed in103.84s**; final
**81/81passed in102.20s** (`build/thrusting-shield-final-tests.log`); existing
app/core/test build passes, coverage/benchmarks OFF.

Current **642-TAE input view/cache `build/development-xml/thrusting-shields`**
links empty retaineda57 to the existing full extraction’s125clips and preserves
all prior targets. No new archive unpacked; never unpack into or recursively
delete through junction views. Coverage is **156bases/1080variants/75shared sets**,
twenty-two vocabularies with **1327definitions/1785raw ordered hits**,
**1222graph-derived/105representative critical mappings**. Existing CLI semantic
JSON round-trip generated
**`build/development-game-data/11711000.graph-thrusting-shields.json`**
(14393014bytes/3296weapons/1080with movesets). Full prior-value/shared-identity
comparison removes only new57assignments and preserves all previous data,
including target metadata (`build/thrusting-shield-data-comparison.log`).
Previous outputs, user edits, installed files and retained inputs are preserved.

## Remaining work

Trace `MoveId` semantics to named attack states, then follow the game's behavior
graph to candidate clips and resolve them using weapon motion parameters and
verified selector rules. Feed the resulting TAE events into the existing
behavior/attack resolver and existing `Moveset`/serialization model.

Next define how target/interaction context should be represented for calculation,
and investigate native request/state and geometry filters for ambiguous queries.
Only then replace representative critical labels with target-aware data; do not
choose hardcoded target rows merely to reach 129 automatic catalog mappings.
Landing timeline evaluation and special-category left/dual/mounted priority
still need additional runtime evidence.
Complete
native priority beyond the scoped ordinary policy remains unverified. Animation presence alone does not
prove an attack is available to a weapon. Conditional heavy/charged
states, mixed powerstance, additional mounted contexts and criticals require their own
verified traversal rules. Some selection behavior is native engine logic; XML
contains its parameters, not necessarily an executable definition of the rule.

Keep the existing Dagger/Longsword catalogs as regression expectations while
extending this resolver. Do not add weapon-specific catalogs merely because the
initial importer lacked behavior data. Graph support covers direct CMSG paths
and manual selectors with explicit context, exact parametric samples and one
audited script wrapper. Other script-generated, landing, additional mounted and
critical target-selection paths still need their own supported rules.

The commonly linked [Adding a Moveset tutorial](https://www.soulsmodding.com/doku.php?id=tutorial:adding-a-moveset)
describes special-category overrides but explicitly targets Dark Souls III.
It is a lead, not sufficient evidence for Elden Ring's complete priority rules.
DSAnimStudio sources checked during this pass did not establish those rules.
The [DSLuaDecompiler README](https://github.com/katalash/DSLuaDecompiler) says no
official release is available; building its pinned source with an isolated SDK
resolved that tooling gap. The community's edited script remains reference only.

Input SHA-256 hashes for reproducing the local investigation:

```text
7999C2A205790BD902224856694B008A6B882D64193D4FECD882192766F7B3BB  c0000.behbnd.dcx
BB8E5E9DF8E73182C029C507FB0A01C2F9AC8F31AB9E5C26675AB5084927CDCA  c0000.game.hks
A9B8A86B760DC4D1B33001DA161AC34302707A7B612A2FA68707FEB0FA7A3737  common_define.game.hks
```
