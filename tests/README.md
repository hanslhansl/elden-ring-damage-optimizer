# Running regressions

Use the repository's configured compiler preset. For example, in PowerShell:

```powershell
cmake --preset 'clang-24 Debug' -DBUILD_TESTING=ON
cmake --build --preset 'clang-24 Debug' --target tests -j 4
ctest --test-dir 'build/clang-24 Debug' -R '^normal\.' --output-on-failure
```

There is one Catch2 executable, `tests`. Its build options are both off by default:

| CMake option | Effect |
| --- | --- |
| `ERDO_TEST_COVERAGE` | Enable Clang/LLVM coverage for the shared core and its consumers, including tests. |
| `ERDO_TEST_BENCHMARKS` | Enable the existing Catch2 benchmark sections in `tests.cpp`. |

For example, enable coverage with:

```powershell
cmake --preset 'clang-24 Debug' -DERDO_TEST_COVERAGE=ON -DERDO_TEST_BENCHMARKS=OFF
cmake --build --preset 'clang-24 Debug' --target tests -j 4
ctest --test-dir 'build/clang-24 Debug' --output-on-failure
```

CTest writes per-process raw profiles under `build/clang-24 Debug/tests/profiles/`
so parallel tests do not overwrite each other's counters. Merge those profiles
with the matching `llvm-profdata` before producing a report with `llvm-cov`.
When running the executable directly, set `LLVM_PROFILE_FILE` yourself if you
want profiles in that directory. Coverage requires Clang/LLVM; unsupported
compilers produce a configuration error instead of silently omitting coverage.

For benchmarks, configure with `-DERDO_TEST_BENCHMARKS=ON` and
`-DERDO_TEST_COVERAGE=OFF`; use a Release preset for meaningful timings. Both
options can be enabled together, but instrumented timings include coverage
overhead. Set both options to `OFF` and rebuild to return to ordinary tests.
These are cached options: switching modes recompiles affected files. Separate
build directories/presets can retain each mode if you switch frequently.

The core is built once per build directory. Coverage also affects applications
linked to that core. The previous `test_with_coverage`, `test_with_benchmarks`,
and `erdo_core_with_coverage` targets are removed. CTest's `normal.` prefix stays
the same in every mode, so existing test filters continue to work.

Moveset regressions use the same Catch2 executable and CTest discovery;
there is no separate runner. The portable `[moveset][unit]` importer cases cover
IDs/catalogs, behavior resolution, encoded player behavior namespaces, ordinary
player-weapon lookup/fallback, attack events, StateInfo/legacy JSON, opposing-target flags, and import/serialization.
They use synthetic Witchy XML and run without game files. Their checks include
category/default/explicit-behavior handling, all 73 move IDs through JSON, unique
catalog IDs, phase selection and missing-phase failures, provenance, MV channels,
repeated/overlapping events, alias chains/cycles/missing targets, invalid inputs,
missing TAEs, rejection of partial moves, throwing checks for supported weapons,
and diagnostic report round-tripping through the existing reflection serializer.

To also run the four original animation regressions, the 63-entry Longsword
catalog (including a snapshot of its 29 added moves), and the 66-entry/77-hit
Dagger snapshot, configure the directory containing
`regulation-bin` and `chr`. The tests also check automatic assignment and shared
identity for 1260 variants of 180 weapons using thirty-three animation families and
eighty-seven exact behavior variations. The axe and hammer snapshots each cover all
63 moves / 77 hits; the flail snapshot covers 60 moves / 78 hits. Greatswords,
greataxes and great hammers have 59 moves and 67/71/81 hits respectively. Separate
Alabaster Lord's Sword and Beastclaw Greathammer snapshots verify their ordinary
heavy behavior overrides, including exact/family row selection. Katanas cover63
moves/72hits; curved greatswords cover59/73. Bloodhound's Fang has its own snapshot
for all four jumping overrides, including a nonuniform holy MV channel.
Colossal swords and colossal weapons each cover 56 moves, with 97 and 112 ordered
hits respectively. Seven independent snapshots check the sword base and all six
colossal-weapon behavior contexts, including cross-variation attack references.
Halberds cover 59 moves/69 hits; scythes cover 57/67. Scythe mounted charge clips
contain release hits only, so their two non-damaging charging entries are omitted.
Separate raw snapshots verify both families and the authored alias origins.
Spears cover 60 moves/69 hits; separate base and variation-1615 snapshots verify
the weapon named Spear's 66 ordinary override rows and family fallback.
Great spears cover 57 moves/115 raw hits; independent base-1700 and exact-1708
snapshots include all conditional hits and Lance's 73 ordinary override rows.
Ordinary combat disables 42 events gated by StateInfo 187, leaving 73 active
events. The explicit Rykard arena effect scenario activates all 115.
Whips cover 51 moves/58 events; the raw base-2000 snapshot also verifies inherited
data for 2003/2006. They have no critical throws or backstab whiff because their
parameters disable throws. Mounted charging has no damage event; charged releases
remain available, with explicit missing-phase requests still rejected.
Ordinary thrusting swords cover 71 moves/83 events, including six ordinary/
offhand light stages, four powerstance stages, shieldpoke and all four feints.
An independent four-hit snapshot also checks heavy-thrusting feints in 600/601/602.
Ordinary curved swords cover 70 moves/87 events, with six ordinary/offhand light
stages, four powerstance stages and four feints. The raw base-700 snapshot also
checks variation 703's family fallback; neither variation enables shieldpoke.
Great katanas cover 63 moves/75 events, four ordinary/offhand lights, three
powerstance lights and four feints. The raw base-6600 snapshot verifies the
inherited a32 backstab-whiff event source despite local rows on the a61 alias.
Light greatswords cover 61 moves/153 ordered events, with five ordinary/offhand
lights and four powerstance lights. The independent base-6700 snapshot checks
all twelve MV channels and target eligibility: 127 events can affect opposing
targets, while 26 remain ordered but inactive for calculation.
Thrusting shields cover 52 moves/55 ordered events, four ordinary/offhand lights
and both mounted charging/release phases; their native request rules exclude
powerstance despite a paired combo limit in the script. Ordinary torches cover
46 moves/46 events; their parameters disable throws and native rules forbid
powerstance:

```powershell
cmake --preset 'clang-24 Debug' '-DERDO_MOVESET_XML_ROOT=C:/path/to/unpacked_data'
cmake --build --preset 'clang-24 Debug' --target tests -j 4
ctest --test-dir 'build/clang-24 Debug' -R '^normal\.moveset' --output-on-failure
```

The case `moveset - game XML` (`[moveset][game_xml]`) is compiled and
discovered only when `ERDO_MOVESET_XML_ROOT` is set. Clear the cache variable
with `-DERDO_MOVESET_XML_ROOT=` and rebuild to disable it. No game XML is
downloaded or bundled by these tests. Expected game values reflect the samples
in `CODEX_HANDOFF.md`, `expected_dagger_moves.inc`, and
`expected_longsword_extended_moves.inc`; game updates or mods can legitimately
change them. The snapshots cross-check retained XML, not independent in-game
validation of every move.

To run the importer and move calculation tests directly with Catch2, use
`& './build/clang-24 Debug/tests/tests.exe' '[moveset]'`; append `[unit]` to the
filter (`'[moveset][unit]'`) to select only the portable checks.

The user migrated `dlc` from false/true to 0/1 in all three bundled JSON files,
resolving the previous six fixture-loading failures. Legacy DLC weapons are
intentionally attributed to DLC 1 because their original DLC provenance is
unavailable. `11711000.json` can be regenerated with accurate IDs later.

# Qt move controls

The Qt application is always configured. `erdo` runs the application;
`erdo_core_exe` uses `src/core/main.cpp` as a separate development entry point.
The former `ERDO_BUILD_UI` option has been removed, including its test guard:

```powershell
cmake --preset 'clang-24 Debug'
cmake --build --preset 'clang-24 Debug' --target erdo erdo_core_exe tests -j 4
ctest --test-dir 'build/clang-24 Debug' -R '^normal\.' --output-on-failure
```

This requires the preset's Qt installation. CTest supplies the Qt and KDChart DLL
directories automatically. Running the application or test executable directly
on Windows requires those directories on `PATH` (or deployment beside the
executable). Building only the `erdo_core_exe` target is useful during importer
development; it does not depend on the UI library.

Four portable UI cases always join the same `tests` executable. They run Qt
offscreen with temporary settings, covering Stats
hit totals, missing secondary weapons, integer DLC filters, replacement game
data, optimizer eligibility, unknown activation conditions, combat environment
selection, and both optimizer buttons' move scores. Select
them with CTest's `-R '^normal.UI moves'` or Catch2's `'[ui]'` filter.

With `ERDO_MOVESET_XML_ROOT` set, a fifth UI case imports the complete game data,
saves it with the existing serializer, and activates the JSON through the real
MainWindow game-data menu action. It checks every move/hit and all 12 power
channels for all 1204 supported variants against the original XML-derived
calculator values. It also checks shared moveset identity, DLC IDs, and both
optimizer buttons against direct optimization for whole moves/first hits,
one-handed/two-handed/powerstance attacks, and total-power/bleed targets.
Ordinary objectives include all 1204 variants; powerstance excludes 39 thrusting-shield variants and four torches (1161
eligible), according to native class rules.
Including the graph tests described below, there are **85 tests with both local
inputs**, **77 with only game XML**, or **73 without local inputs**. Tests create only
temporary JSON/settings and do not change the bundled game data or user settings.

In Stats, selecting a weapon shows its imported moves, per-hit powers and status
buildup. Choose a move or hit to narrow the breakdown. In Optimize, leave
`Weapon attack power` selected for the original behavior, or choose a move and
optionally one hit. The existing target selects total damage power or a specific
damage/status channel. Unsupported weapons are excluded and the result's
`Move Score` is separate from ordinary attack power. Stance follows the move;
spell scaling is unavailable as a move objective. The matching-weapon checkbox
explicitly enables two copies with the same affinity and upgrade level for
powerstance. Both tabs offer `Ordinary combat` (default) and `Rykard arena effect`;
inactive hits retain their numbers and display zero powers. The environment
decides only the verified StateInfo 187 flag; other unknown conditions still
produce diagnostics. Totals assume all active hits connect and are before critical bonuses and
enemy defenses.

The bundled JSON files currently have no imported movesets. Load newly generated
game data through the existing UI workflow to use the move controls; 172
weapons matching the twenty-four validated ordinary families are supported. Existing JSON without
movesets still supports ordinary attack power and spell scaling.

# Development JSON from retained XML

The development executable now accepts an import command. Running it without
arguments still runs the existing exploration code in `src/core/main.cpp`.

```powershell
cmake --build --preset 'clang-24 Debug' --target erdo_core_exe -j 4
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --import `
    'build/development-xml/straight-specials' `
    'build/development-game-data/11711000.json'
```

The command calls `witchy::load_game_data`, saves through `json::save`, reloads
through `json::load<GameData>`, and verifies weapon names and moveset contents.
It refuses to overwrite an existing output; choose another path for a later run.
It does not unpack archives or modify the XML. The current retained data produces
version 11711000 with 3,296 weapons and 1204 weapons sharing eighty-two imported movesets.
The development output is ignored under `build/`.

To use graph-derived ordinary, offhand and matching-powerstance mappings while retaining the complete
validated catalog for other moves, use the opt-in development import:

```powershell
& './build/clang-24 Debug/src/core/erdo_core_exe.exe' --import-graph `
    'build/development-xml/straight-specials' `
    'build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml' `
    'build/development-game-data/11711000.graph-straight-specials.json'
```

This file has been generated locally and verified through the existing JSON
round-trip. Choose a new output filename to rerun. It contains 1204
supported weapons and eighty-two complete movesets. Across the twenty-eight animation families,
1579 definitions come from the graph and 130 use representative critical catalog mappings;
each exact behavior variation resolves its own hit data. Graph failures or mismatched affinity
contexts reject the import; they do not fall back to a catalog animation.

The retained XML directory was missing some axe clips, including `a30/031730`,
although its binary `a30.tae` contains them. Only that 760 KB TAE was copied and
re-extracted into `build/mapping-research/axe-tae-audit/a30-tae`. Its attack data
matches all 127 previously extracted clips. The hammer TAE was also freshly
extracted into `build/mapping-research/hammer-tae-audit/a33-tae`: its 726 KB binary
supplies critical aliases missing from the retained XML, and all 132 existing
clips' attack signatures match. Hammer criticals explicitly alias axe clips,
while their attack values use hammer behavior variation 1100. Flail aliases were
likewise recovered from a workspace copy of `a34.tae` (747 KB), matching all 118
existing clips' attack signatures. Workspace copies of a25/a32/a35 (806/813/753 KB)
were also freshly extracted; all 140/130/118 existing clips' attack signatures match.
The a29/a40 copies (704/635KB) match all122/107 existing clips; their fresh
extractions restore missing aliases. The latest a26/a31 copies (814640/821488 bytes)
match all135/115 existing clips. A fresh 736960-byte a24 copy confirms the missing
twinblade powerstance clip described below. Original retained files stay unchanged.
The six heavy families use their audited stance positions 2/12, spears/great spears/halberds/scythes
use 3/13, and the other six use 0/10. Colossal swords require type 7/motion 26/behavior 400–499, and colossal
weapons require type 41/motion 31/behavior 2300–2399. Their ordinary, offhand and
matching-powerstance chains each contain three light stages. Altered positions,
special categories, catalysts and built-in paired weapons remain unsupported.
Halberds require type 29/motion 38/behavior 1800–1899, scythes type 31/motion
50/behavior 1900–1999. Both have four ordinary/offhand and three matching-pair
stages. Only zero special category is supported; weapons such as Halberd and
Scythe themselves have special routing and remain excluded.
Spears require type 25/motion 36/behavior 1600–1699 with 3/13 stances. Their
60-entry catalog adds `Shieldpoke (normal guard)` with explicit guard-style
index 0. The graph resolver accepts caller-supplied guard style/torch context;
real regressions check all five branches (normal, heavy, small, torch, dueling)
for both base and variation 1615. Their attack data agrees while clip IDs remain
distinct. The default entry is a labeled representative; it does not infer the
left guarding weapon or prove that the player can perform the attack.
Great spears require type 28/motion 37/behavior 1700–1799 with 3/13 stances and
the same complete ordinary signature guards. Bloodfiend's Sacred Spear (1700),
Lance (1708), and Mohgwyn's Sacred Spear (1711) add 15 variants/three shared sets.
Their ordinary/offhand and matching-powerstance lights each have three stages.
Full raw snapshots check all 57 moves/115 hits, and all five guard-style branches
are checked for base 1700 and Lance. SpEffectParam 1908 supplies StateInfo 187;
native Rykard map events grant it to the player in the arena and remove it after
boss defeat. This enables a meaningful explicit environment selection without
a weapon-ID exception. It does not establish hitbox contact or native weapon
eligibility for every conditional hit. Special-category great spears, including
Serpent-Hunter, remain outside this complete ordinary profile.

Whips require type 39/motion 43/behavior 2000–2099 and 0/10 stances, special 0,
catalyst 0, built-in-pair 0, and `enableThrow=0`. Whip, Thorned Whip, Hoslow's
Petal Whip and Tooth Whip use 2000; Magma Whip Candlestick uses 2003 and Giant's
Red Braid uses 2006. These six weapons add 42 variants/three shared sets. Their
ordinary/offhand chains have four stages and matching-powerstance has three.
The existing graph derives all 51 definitions, including movement, heavy,
jump and mounted releases. Every raw event has StateInfo 0. Urumi's nonzero
special category remains excluded; there is no weapon-name/ID exception.

Ordinary thrusting swords require type 15/motion 27/behavior 500–599, 0/10
stances and the complete ordinary guards with `enableThrow=1`. Cleanrot Knight's
Sword, Rapier and Antspur Rapier add 39 variants sharing one set. The graph
derives all 66 noncritical definitions, including the four feints. The existing
five representative critical mappings retain their target-context limits.
Neutral `resolveWeaponFeintMove` supports audited thrusting swords, curved swords and great katanas
(categories 27/28/39/61) with zero special category; it preserves all definition filters and rejects
missing/wrong graph states or offsets. It maps a requested feint, without
simulating the R2/evasion input, charge window or temporary effects.

Ordinary curved swords require type 9/motion 28/behavior 700–799, 0/10
stances, special 0/catalyst 0/built-in-pair 0/enableThrow 1. Falchion, Bandit's
Curved Sword, Scavenger's Curved Sword, Serpent-God's Curved Sword, Grossmesser
and Spirit Sword share variation 700; Magma Blade has a separate variation-703
set. These seven weapons add 67 variants/two shared sets, selected by the full
parameter signature rather than names or IDs. Their 70 moves include four
feints but no shieldpoke, consistent with native guard eligibility. All 87 raw
events have StateInfo 0. Variation 700 has 115 namespace-1 behavior rows;
703 has none and inherits family rows without merging shared identity.
Special-category and built-in-pair curved swords remain excluded.

Ordinary great katanas require type 94/motion 61/behavior 6600-6699, 2/12
stances, special 0/catalyst 0/built-in-pair 0/enableThrow 1. Great Katana,
Dragon-Hunter's Great Katana and Reed Great Katana add 27 variants sharing
base 6600. Its 96 namespace-1 behavior rows resolve all 63 moves/75 hit events;
58 definitions come from the graph, plus five representative critical mappings.
All StateInfo values are zero. Rakshasa's Great Katana has special category 953
and remains outside the ordinary profile. No weapon-specific routing is added.

Ordinary light greatswords require type 93/motion 60/behavior 6700–6799,
3/10 stances, special 0/catalyst 0/built-in-pair 0/enableThrow 1. Milady,
Leda’s Sword and Idus Sword add 27 variants sharing base 6700. Its 155
namespace-1 rows resolve 61 moves/153 events: 56 graph definitions and five
representative critical mappings. Mounted charged releases select judges
638/738; the two non-damaging charging entries are omitted. Rellana’s Twin
Blades remain excluded by their special/built-in-pair signature. Production
uses the complete parameter signature, with no per-weapon exception.

Ordinary thrusting shields require type 90/motion 57/behavior 6100–6199,
3/13 stances, special 0/catalyst 0/built-in-pair 0/enableThrow 1. Dueling Shield,
Carian Thrusting Shield and Ritual Thrusting Shield add 39 affinities sharing
base 6100. All 52 moves/55 events have StateInfo 0 and opposing-target eligibility;
47 definitions come from the graph, plus five representative critical mappings.
The full parameter signature selects membership, with no weapon exceptions.
Native `IsEnableDualWielding` excludes this class, so the bridge generates only
ordinary/offhand light chains. Every requested ordinary chain must still be
complete, and any attempted paired definition rejects. Mounted phases retain
630/635 and 730/735. No feints or shieldpoke are enabled.

These mappings describe selected attack states. Offhand assumes an attacking
left-hand request (e.g. the L2 fallback without an active skill); native L1 can
guard with this class. Guarded two-handed R1 (`AttackBothLightGuard`) is a
separate state and is not added to the current normal-attack MoveId vocabulary.
The current tests check ordinary optimization eligibility for all 1204variants
and exclude the 39 thrusting-shield affinities and four ordinary torches from
powerstance (1161 eligible).

Ordinary torches require type 87/motion 21/behavior 2400–2499, 0/10 stances,
special 0/catalyst 0/built-in-pair 0/enableThrow 0. Torch, Steel-Wire Torch,
Beast-Repellent Torch and Sentry’s Torch add four weapons sharing base 2400.
The complete parameter signature selects them; their utility/passive effects
and weapon skills do not select or alter this normal-state catalog. Ghostflame,
St. Trina’s and Nanaya’s Torch have special categories and remain excluded.

`torchCatalog` derives from the unpaired thrusting-shield vocabulary and removes
backstab whiff plus all five critical throws: 46 moves/46 events, all graph-derived.
The 78 namespace-1 behavior rows resolve these with StateInfo 0 and opposing-target
eligibility. The snapshot checks all twelve MV channels and raw provenance.
Mounted phases keep 630/635 and 730/735. No powerstance, critical throws, feints
or shieldpoke are enabled, and explicit torch throw queries reject.

This API maps selected attack states. L1 guards with torches; offhand data assumes
an already selected left-heavy attack request, such as the skill-disabled L2
fallback. Mapping weapon skills or deciding whether that request is available
under the current equipment/input/effect context remains separate.

Heavy thrusting swords have asymmetric authored positions 3/10, now represented
by the stance helper. Their five-stage ordinary/offhand chains and four feints
resolve, but complete automatic import remains disabled: HKS permits a fourth
powerstance stage, while `AttackDualLight4` has no category-39 registration and
the full archive has no a39/34030 TAE. A regression preserves that diagnostic;
the chain is never shortened to three. Specials/catalysts among either class
remain outside the complete ordinary profile.

The retained a38/a50 directories were empty and their original binaries absent.
A workspace copy of the installed 6.9 MB `c0000.anibnd.dcx` was freshly extracted
into `build/mapping-research/pole-tae-audit/` (506 MB). The archive's attack
signatures match every previously staged clip across all twelve supported
families and a24. The new view uses its a38/a50 and their a36 alias source;
installed files and retained inputs were read only. This ignored extraction can
be reused for later class audits.
The development root above uses
directory junctions to the retained parameters/messages/other TAE directories
and to these fresh family extractions, including a21/a22/a24/a26/a27/a28/a31/a36/a37/a38/a39/a42/a43/a50/a57/a60/a61.
It is an input view; do not unpack into it.
The local test cache now uses this root. Other machines need a complete extraction
of `c0000.anibnd.dcx`, including all twenty-four supported family TAEs and their
alias targets (including a36); the negative
twinblade regression also uses a24. No additional game archive is required.
The retained a43 directory is empty; the whips view links it to the already
recovered full archive. This is missing old comparison input, not a comparison
with a prior complete whip extraction. No archive was copied or unpacked again.
The thrusting view likewise links empty retained a27/a39 to that same recovered
archive; a39 is used for explicitly scoped feint/guard and negative-chain tests.
The current curved view preserves the thrusting view and links empty retained a28
to that same extraction. No archive was copied or unpacked again. Never unpack
or recursively delete through these input views.
The great-katana view preserves every curved-view target and links empty retained
a61 to the same recovered archive. The light-greatsword view preserves those
targets and links empty retained a60 to the same extraction. No new archive
was unpacked or copied. The thrusting-shields view preserves these targets
and links empty retained a57 to that same full archive (125 clips). The
torches view preserves those targets and links empty retained a21 to the existing
full extraction (101 clips). The claws-complete view preserves those
targets and links empty retained a22/a42 to156/151 clips. The fists view adds
100a48 clips; the beast-claws view adds148a62 clips, and the current
backhand-blades view adds131a58 clips. The current martial-arts view adds148a55
clips from the same full extraction. No archive was copied or unpacked again.

In the application, use **File → Load Game Data from Directory** (the existing
action opens a file picker) and select this JSON. Then activate its entry under
**File → Choose Game Data**. Select Dagger or Longsword in Calculate to see their
moves, or choose a move in Optimize to compare supported affinities. The bundled
JSON files remain unchanged.

To rerun the complete workflow regression:

```powershell
cmake --preset 'clang-24 Debug' '-DERDO_MOVESET_XML_ROOT=F:/Paul/Programmieren/cpp/elden-ring-damage-optimizer/build/development-xml/straight-specials'
cmake --build --preset 'clang-24 Debug' --target tests -j 4
ctest --test-dir 'build/clang-24 Debug' -R '^normal.UI moves - XML' --no-tests=error --output-on-failure
```

# Move evaluation and optimization

Behavior-graph tests use the same executable. Twenty-three portable cases always run;
the eight real graph comparisons additionally require both `ERDO_MOVESET_XML_ROOT`
and `ERDO_MOVESET_BEHAVIOR_XML`. Configure the latter with the HKLib XML export:

```powershell
cmake --preset 'clang-24 Debug' '-DERDO_MOVESET_BEHAVIOR_XML=F:/Paul/Programmieren/cpp/elden-ring-damage-optimizer/build/mapping-research/c0000-behbnd-dcx/Behaviors/c0000.xml'
cmake --build --preset 'clang-24 Debug' --target tests -j 4
ctest --test-dir 'build/clang-24 Debug' -R '^normal.behavior graph' --no-tests=error --output-on-failure
```

With both local inputs configured the suite has 85 tests; with only local TAE/
parameter XML it has 77, and without local inputs it has 73. The cases compare
graph-derived mappings and their imported hits with the existing catalogs;
another case generates all 30 ordinary light-chain moves across both weapons
from weapon parameters and graph states without catalog seeds, then compares
complete imported hits. Portable cases cover script-derived combo limits,
special-category overrides, unsupported routing, and rejection of partial chains.
Selector cases cover nested fixed/bound branches, named runtime context,
missing/invalid bindings, cycles and reloads. The local graph check follows the
actual offhand first-attack selector and both special-heavy start/end branches.
Heavy-effect tests verify right/left hand selection, script overwrite order,
16 ordinary heavy catalog snapshots, and two charged roar paths through graph,
TAE, encoded behavior lookup, attack MVs and JSON. Encoded lookup tests check
namespace boundaries, exact/family/common precedence, preserved raw judges,
category constraints, and rejection of invalid selected rows without fallback.

The ordinary player-weapon lookup regression compares all 129 catalog moves and
149 hits against the original constrained imports with complete serialized-value
equality. It removes category/behavior-ID constraints from the new path, checks
real variation-201 override/family fallback, and leaves strict collision checks
intact. Existing GameData/UI tests exercise that lookup in production imports.
The parameter-derived category test additionally compares 119 Dagger/Longsword
ordinary, offhand, matching-powerstance, airborne jump, mounted and backstab-whiff moves without explicit category input, and imports 12
Shortsword/Broadsword samples combining base lights with special heavies. Portable
tests check scope guards, parameter defaults, override/fallback and failed clips.
Jump tests exercise all five nested selector routes and refuse landing clips
with dynamic time bindings. Only unbound instantaneous selector transitions
with default modes are accepted; timed or shifted transitions reject.
Mounted tests verify the selected weapon's hand-style selector on both sides,
all twelve MoveIds, preserved charging/release event filters, missing-state
failure and rejection of special categories, catalysts and unsupported stances.
Throw tests compare five explicit target interactions for each weapon to the
existing critical hit data. Exact parametric samples are supported; interpolation,
invalid bindings, cyclic blends and substitute clips reject. These explicit rows
are test scenarios, not an automatic target-size or MoveId classification table.
Whiff tests validate the one audited script wrapper and preserve its strike filter.
Target-query tests match character/throw-type/type-ID/attacker-state fields and
resolve only if every candidate gives the same supported animation/category.
They check deterministic candidate lists, no character-0 fallback, weapon
`enableThrow`, unsupported candidates and genuine conflicting-animation groups.
The real test resolves all135 representative critical queries across27 exact profiles from target queries and weapon
parameters, including eight matching position variants for target c3950/type 22.
The complete GameData bridge case checks all 1204 supported variants against the
default import, writes/loads JSON through the existing serializer, verifies shared
moveset identity, and rejects missing graph states with reports for all eighty-two groups.
It also checks a real unsupported twinblade case: explicit graph traversal selects
four powerstance stages, but a24/34030 is absent even after fresh extraction. The
import reports that missing fourth stage rather than silently shortening the chain.
Twinblades remain outside automatic import until their transition routing is established.
Its portable counterpart changes graph animations away from catalog values,
checks preservation of uncovered moves and rejects mismatched sharing contexts
and incomplete chains. The importer also rejects truncated resolver output.
Movement tests distinguish the script's HeavyCounter entry from the unused
LightCounter shortcut despite their matching clips in the real graph. They also
check all ten 1H/2H movement/counter states, offsets, special/base selection and
failure of missing running/counter entries in the full catalog bridge.
Offhand/powerstance cases verify independent chain lengths, non-arithmetic clips,
dual movement states, rejection of unsupported pair categories and missing states,
and rejection of unaudited special-category priority. Explicit inspection still
works when a caller supplies that context; automatic left/dual selection admits
zero special category and the scoped 55/246 pair described below, assuming the
appropriate offhand or matching-pair request.
These checks do not establish complete conditional reachability or universal
weapon-category precedence.
See [mapping investigation](../docs/moveset-mapping.md) for API and developer
command details.

`MoveEvaluator` calculates the existing `Attack` once per stance and reuses its
base/full attack powers for each move. `evaluate(id)` returns an expected
`MoveAttack`; `evaluate_all()` returns one success or diagnostic per imported
move. Each result contains ordered hits (with their original `ResolvedHit`),
per-channel base/full powers, and totals. Only the five damage channels enter
`total_attack_power`; status buildup stays in its respective channel. No rounding
is applied. These are raw MV-adjusted powers, before critical bonuses, other
damage modifiers, enemy defense, or status-proc effects. Hit windows are not DPS.

```cpp
calculator::MoveEvaluator evaluator(weapon, stats, upgrade_levels);
auto result = evaluator.evaluate(calculator::MoveId::ONE_HAND_R1_1);
if (result) {
    const auto& whole_move = result->selected_powers();
    const auto& first_hit = result->selected_powers(0); // zero-based API index
}
auto all_moves = evaluator.evaluate_all();
```

Supply a fourth constructor argument, `&second_weapon`, for Powerstance. Passing
`&weapon` explicitly models a matching pair. Different affinities within the same
validated motion category/behavior variation are supported; mixed moveset
families are rejected. Offhand/default-source events use the primary weapon
being evaluated. Missing mappings, empty moves, unknown MVs, and missing or
unrecognized weapon sources produce an error, not a fabricated zero result.
The lower-level `calculate_move` accepts already calculated `AttackRating`s;
its caller supplies the correct stance and compatible source-weapon ratings.

`AttackEvent::stateInfo` preserves the raw optional TAE field, including through
aliases and JSON. Missing (including legacy JSON), zero, or negative values are
ungated. Positive values require an explicit `MoveContext::state_info` decision;
an absent decision returns an error. The importer rejects malformed, overflowed,
or duplicate StateInfo parameters and retains all resolved conditional hits.

```cpp
auto context = calculator::move_context(calculator::MoveEnvironment::ORDINARY_COMBAT);
calculator::MoveEvaluator evaluator(weapon, stats, upgrade_levels, nullptr, context);
auto result = evaluator.evaluate(calculator::MoveId::ONE_HAND_R1_1);
// calculate_move(move, primary_rating, secondary_rating, context) uses the same flags.
```

The evaluator owns its context; flags are fixed for the evaluation. Inactive
hits remain at their original indices, with `HitAttack::active == false` and zero
power in every channel. They need neither usable MVs nor a second weapon.
`MoveObjective::context` supplies the same flags to both optimizers; selected-hit
objectives check only that hit's conditions. No flags are inferred from weapon
names, IDs, behavior namespaces, or attack indices. The helper above decides
only StateInfo 187, setting it false for `ORDINARY_COMBAT` and true for
`RYKARD_ARENA_EFFECT`. Both UI tabs supply their selected environment through
this helper. The low-level API still defaults to an empty context, so conditional
data requires an explicit choice. Other unknown conditions and flags changing
during an animation need further research. The arena effect represents the
1908 activation scenario; it does not calculate conditional hitbox contact.

The raw XML audit (`build/mapping-research/audit_state_info.py`) checks all 4,936
selected events in the previous 63 shared movesets across 809 source clips:
none has a positive condition. The now-supported a37 vocabulary contains 57
moves/115 events, including 42 gated by 187; these are not unconditional totals.
The great-spear tests verify both environments on all 15 variants, retain raw
hit indices, and require 1908 to be the sole parameter row with StateInfo 187.

Both existing optimizers accept an optional final `MoveObjective` argument:

```cpp
optimizer::MoveObjective objective{
    .selection = {calculator::MoveId::ONE_HAND_R1_1, std::nullopt}
}; // whole move; use hit_index 0 for the first hit
optimizer::V2<optimizer::Target::TOTAL_ATTACK_POWER> search{
    weapons, attack_options, max_attribute_points, min_stats, max_stat,
    optimize_starting_class, objective
};
auto attacks = search.run_synchronously(weapons);
// search.projection(attack) scores the selected move/hit.
// attack's fields still contain the normal weapon attack rating.
```

The existing per-type targets also support moves, including status buildup;
spell scaling rejects move selection. The move determines one-/two-handed stance.
Multipliers for only the selected move/hit and target channels are prepared once
per weapon before the stat loop. Unsupported candidates are excluded by
`run_synchronously`; `supports` and `unsupported_reason` expose the reason for a
future UI. Directly invoking the optimizer on an unsupported weapon throws.
Iteration counts exclude unsupported candidates. Powerstance optimization needs
`objective.matching_powerstance = true` and compares two copies of each candidate;
an independently equipped second weapon is not part of this search space yet.

Tests cover hand-specific and fractional status values, stance selection,
serialization of evaluated hits, unknown inputs, whole-move versus first-hit
optimal stats, both search algorithms, status objectives, unsupported candidates,
and evaluation of every imported move on all 1204 supported weapon variants.
The Qt application, move-selection widgets and result display are included in
the build and covered by the real-data UI regression.

# Import diagnostics

`importMoveset` returns `MovesetImportReport` containing counts, motion category,
behavior variation, and per-move failures. Each resolution failure retains
event context and candidate behavior IDs/categories/reference IDs. The report
can be saved using `erdo::json::save(path, report)`; no custom serializer is needed.
The callback receives only moves whose attack events all resolved successfully.

Calling the overload without a callback returns `MovesetImportResult` with a
shared `calculator::Moveset` and the same report. `Move` stores the existing
`ResolvedHit` objects, including all 12 MV channels/known flags, scheduled attack
windows, and source IDs. These data types live in `calculator.ixx`; the importer
keeps aliases for its existing public names.

`load_game_data(root, &reports)` assigns complete validated movesets by
parameter signature, without a weapon-ID allowlist. Dagger class requires motion
20 / integer behavior variation 100–199; straight swords require 23 / 200–299;
axes require weapon type 17, motion 30 / variation 1400–1499; hammers require
weapon type 21, motion 33 / variation 1100–1199; flails require weapon type 24,
motion 34 / variation 1300–1399. Greatswords require weapon type 5, motion 25 /
variation 300–399; greataxes require type 19, motion 32 / variation 1500–1599;
great hammers require type 23, motion 35 / variation 1200–1299. Katanas require
type13, motion29 / variation900–999; curved greatswords require type11, motion40 /
variation800–899. Colossal swords require type7/motion26/variation400–499;
colossal weapons require type41/motion31/variation2300–2399. These six heavy
families require positions2/12; the other six families above require positions0/10.
Spears require type25/motion36/variation1600–1699, great spears
type28/motion37/variation1700–1799, halberds type29/motion38/variation1800–1899,
and scythes type31/motion50/variation1900–1999; all four require positions3/13.
All require
special category zero, non-catalyst, non-paired
weapon. Throw-capable profiles require `enableThrow == 1`; ordinary whips
require `enableThrow == 0` and omit all throw/whiff definitions. Whips require
type39/motion43/variation2000–2099 with positions0/10. Thrusting swords require
type15/motion27/variation500–599 with positions0/10. Missing or mismatched routing fields reject
membership; weapon attack power, scaling and passive status remain independent.
Each exact variation receives a separate import using exact → hundred-family →
common behavior lookup. Only weapons exported into GameData participate, so
unused/unnamed parameter rows do not create extra movesets or import failures.

Current eligible base weapons are Dagger, Parrying Dagger, Crystal Knife,
Cinquedea, Bloodstained Dagger, Longsword, Lordsworn's Straight Sword, Weathered
Straight Sword, Crystal Sword, Noble's Slender Sword, Velvet Sword of St. Trina,
Sword of Light and Sword of Darkness. Their 41 dagger and 56 straight-sword
variants share two pointers across weapon names and JSON round-trips. Also
supported are Reduvia (variation 104), Coded Sword (204), Sword of St. Trina (205),
Rotten Crystal Sword (206), Golden Epitaph (207), Sword of Night and Flame (214),
and Regalia of Eochaid (215). These seven retain distinct pointers and variation
metadata through JSON. Their ordinary hits inherit the base family rows in the
retained data; their unique skill rows do not imply skill support. Names
appear only in regression expectations/documentation, not the import policy.

Axes add Battle Axe, Jawbone Axe, Highland Axe and Sacrificial Axe (variation 1400),
Rosus' Axe (1412), and Stormhawk Axe (1414): 54 variants sharing three more sets.
`axeCatalog` reuses the validated 63-entry vocabulary/clip IDs/phase filters;
all hits come from axe TAE and behavior data. Successful criticals select TAE
event type 304, retaining both victim-directed hits. Separate type-1 collision
events are outside that target context, remain in the raw cache, and still fail
an unfiltered import if unresolved. `MoveDefinition::eventType` is optional;
when combined with a judge filter, both must match. Empty selections or invalid
selected hits reject the move. `resolvePlayerThrow` supplies the same explicit
throw-hit context. Existing dagger/straight-sword hit snapshots are unchanged.

Hammers add Curved Club, Warpick, Varré's Bouquet, Hammer and Flowerstone Gavel
(variation 1100), Envoy's Horn (1110), Nox Flowing Hammer (1112), Ringed Finger (1113),
and Marika's Hammer (1115): 45 variants sharing five more sets. `hammerCatalog`
reuses the same 63-entry vocabulary/phase filters. The independent hammer snapshot
checks all hits, including critical events aliased from a30 while resolving a33
weapons' behavior rows. Nonzero special-category hammers still require separate
routing support.

Flails add Nightrider Flail, Flail and Chainlink Flail (variation 1300), Family
Heads (1302), Bastard's Stars (1301), Serpent Flail (1320), and Golden Order Flail
(1330): 43 variants sharing five more sets. Their ordinary/offhand chains have
four stages, while powerstance retains four. `flailCatalog` derives that exact
membership from the shared template using `omitCatalogMoves`; invalid omissions
fail at compile time. The independent snapshot checks all 60 moves / 78 hits,
including the three-hit powerstance finisher and its 50% status MVs. Criticals
alias axe event data and resolve flail behavior rows. Every imported set must be
complete; a shorter legitimate chain is represented by its actual membership.

If a supported catalog fails, import reports are appended before a combined
exception is thrown; `load_game_data` cannot return silently incomplete supported
movesets. Lower-level `importMoveset` still returns a report for inspection.

Portable tests verify weapon/moveset JSON round-trip and pointer identity with
the existing serializer, and compatibility with weapon JSON lacking a moveset.

The catalog requests category 1 for the ambiguous Longsword animations 30505
and 31030. Do not apply that filter to the entire catalog: other contexts use
categories 12 and 2. Unfiltered ambiguity remains an error, including multiple
rows pointing to the same attack.

The Dagger catalog requests category 1 for jumping R1 (31030). Its second 2H
heavy and charged-heavy moves (32515/32510) explicitly require behavior rows
100100310/100100315: the XML labels them `Default - Dagger`, while Blood Tax rows
300100310/300100315 share the same variation, judge, and category. Unfiltered
resolution remains ambiguous; there is no general preference for an ID prefix.

The small-PvE Dagger criticals (31720/31730) similarly require 100100530:
Repeating Thrust's 300100530 shares category 12 and judge 530. Both critical
entries obtain events from a23/31720, retaining the requested animation ID.

# Move coverage and animation phases

`MoveId` covers the 73 move columns in the
[community spreadsheet](https://docs.google.com/spreadsheets/d/1j4bpTbsnp5Xsgw9TP2xv6d8R4qk0ErpE9r_5LGIDraU/edit?gid=0#gid=0).
Its "Paired" attacks are named **Powerstance** here. A defined ID does not imply
a catalog entry for every weapon. Shieldpoke, feints, and Powerstance L1 #5/#6
are not mapped for Dagger or Longsword. Longsword has five light attacks per
1H/2H/offhand chain; Dagger has six. The sheet is a development reference only.

An optional `MoveDefinition::eventJudgeId` selects all matching events from a
shared animation. Mounted 38100 splits into charging (630) and release (635),
and 39100 splits into 730/735. Backstab whiff 31719 selects strike 509, excluding
throw-acquisition event 590 from that move. The raw cache keeps every event.
Selection preserves repeats and original timestamps; a missing phase is an
import failure. It does not bypass behavior/category checks or infer how long
the player holds a charge or how often an active window hits a target.

Critical motion values are raw XML corrections. For example, Dagger's backstab
is 294; the sheet's 382.2 includes its 30% critical bonus. The importer does not
apply this bonus. Small/large PvE names describe the catalog's representative
throw animations, not a complete mapping of enemies to critical attacks.

Offhand and mounted events retain raw `Source=0` (DEFAULT). They are not changed
to a guessed left/right source. Weapon selection in those contexts belongs to
the calculator's move context, not the XML event reader.

# Ordinary paired claws and PCBehavior events

Hookclaws, Venomous Fang and Bloodhound Claws share the audited complete profile
**type37/motion22/behavior2200–2299/stances0,0/special0/catalyst0/built-in-pair1/
enableThrow1**. This adds three bases/39 affinities/one exact-2200 shared set.
Raptor Talons and Claws of Night retain unsupported special-category routing.
The catalog has **68 moves/98 ordered events**, six ordinary/offhand and six
matching-powerstance light stages, mounted charging/release phases, and five
representative critical mappings. It enables no feints or shieldpoke.

Two-handed native Both states use the selected paired weapon. All their raw
weapon attack sources are DEFAULT0; no second equipped weapon is required.
The existing `Weapon::paired` flag suppresses the two-handing Strength bonus.
Powerstance Dual states remain a separate matching-equipped-pair request;
Source2 still requires the explicit second weapon and matching behavior context.

One-handed running/rolling R1 uses TAE **307 (PCBehavior)** with raw selector
**PCBehaviorType16/Condition0x64_GetWeaponData0/Offset0**, and no Source or index.
The pinned [DSAnimStudio event definition](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/Res/TAE.Template.ER.xml#L1004-L1010)
preserves those fields; its [attack simulation and lookup](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/TaeEditor/TaeActionSimulationEnvironment.cs#L523-L535)
selects right-weapon behavior for16, then resolves the existing player behavior
address and attack reference. Only this audited selector combination is admitted.
Other combinations reject explicit type307 requests. `AttackEvent` retains three
optional raw selector fields through the existing reflection serializer; legacy
JSON lacking them keeps its previous behavior.

`extractAttackEvents` and `AnimationCache::get` default to existing type1/304
extraction. Their `includePCBehaviors` option retains type307 too, with separate
cache entries and alias traversal for each mode. `importMoveset` opts in only
when a definition explicitly selects eventType307. Existing categories and
auxiliary body PCBehavior events are not reinterpreted as weapon attacks.
The five claw jumping states also contain type4/judge550 body behaviors; body
attack power/contact is outside the selected weapon-event catalog.

The independent raw snapshot `expected_claw_2200_moves.inc` verifies every
selected event and all12 channels, timing/order/source, aliases and behavior/
attack IDs. All selected events have nonpositive/absent StateInfo and opposing-
target eligibility. Portable tests cover strict selectors, malformed/missing/
duplicate/overflowed fields, legacy JSON, separate cache modes, explicit event
selection, built-in pairing and both optimizer scores. The synthetic complete
bridge uses non-arithmetic clips and rejects missing sixth-stage states.
Five representative target-throw queries and full real XML/JSON/UI checks cover
all new affinities.

The current642-TAE read-only view links **a22 (156 clips) and its a42 critical
alias dependencies** to the existing full extraction. No archive was unpacked
again. Never unpack into or recursively delete through these junctions.

# Ordinary paired fists

The complete ordinary profile requires weapon type35/motion42/behavior2100–2199,
stance0/0, special0/catalyst0, `isDualBlade=1`, `enableThrow=1` and
`bothHandEquipable=1`. Iron Ball and Star Fist share exact2100; Poisoned Hand
and Madding Hand share exact2121: four bases/28 variants, two distinct sets.
The 2121 overrides affect offhand and matching-powerstance behavior rows; both
tracked snapshots preserve these differences. Names/IDs are test expectations;
production uses parameter signatures and the native graph.

`fistCatalog` reuses the 68-move six-stage claw vocabulary, restoring ordinary
type1 for running R1. Only rolling R1 uses307/PCBehavior16/Condition0/Offset0,
with raw Source/index=-1. Each set has96 ordered selected events; two-handed
hits use DEFAULT0 and the selected paired weapon without a second rating or
Strength bonus. Matching-equipped powerstance still requires a second rating.
Body PCBehavior4/judge550 remains outside the selected weapon-event scope.
All selected StateInfo are zero/absent and opposing-target flags are true.

Grafted Dragon has the same ordinary motion signature but `bothHandEquipable=0`;
it remains excluded from the complete catalog. Paired claw/fist admission and
all graph group members now require the native both-hand flag, rejecting missing,
non-1 or inconsistent availability. This avoids a weapon-ID exception. Special
fist categories and unarmed remain outside this audited profile.

Independent `audit_fists.py` and `audit_fist_graph.py` use raw default-merged
params, aliases, native HKS limits and graph registrations, without importer
output. `expected_fist_2100_moves.inc` and `expected_fist_2121_moves.inc` compare
all68/96 moves/events per group, every MV channel, flags, timing/order/source,
behavior/attack IDs and alias provenance. Existing Catch2 cases check strict
signature mutations/deletions, non-arithmetic graph clips, unavailable/missing
flags on later group members, six-stage completeness, critical target queries,
shared JSON identity, every variant's move/hit powers and both UI optimizers.
Baseline83/83 passed in126.75s; final83/83 passed in133.48s
(`build/fist-final-tests.log`); app/core/Catch2 build and CLI JSON round-trip pass.
Full previous-value/shared-identity comparison removes only the new42 assignments.
At the fist milestone:167bases/1151variants/79sets; optimizer eligibility1151ordinary/1108powerstance.

The previous fists junction view preserves claws-complete targets and adds the
100 fresh a48 clips needed by six mounted light aliases. No archive extraction
or production dependency change is needed; never unpack/delete through junctions.

# Ordinary paired beast claws and offhand source roles

The complete ordinary profile is type95/motion62/behavior6800–6899, stances0/0,
special0/catalyst0, paired1/enableThrow1/bothHandEquipable1. Beast Claw and Red
Bear’s Claw add two bases/14 variants sharing exact6800. `beastClawCatalog`
contains64 moves/99 ordered selected weapon events: four ordinary/offhand and
matching-powerstance light stages, four neutral feints, two type307 movement
R1s, mounted charging/releases and the existing representative criticals.
Five jumping body PCBehavior4/judge550 events remain outside weapon-event scope.
All selected StateInfo are zero/absent and opposing-target flags are true.

The raw offhand chain explicitly uses Source2 (left weapon), unlike the earlier
Source0 chains. `hit_uses_secondary_weapon` now takes the MoveId: the selected
primary weapon is left for OFFHAND_R1 moves. DEFAULT0 stays primary; explicit2
is primary/explicit1 secondary for offhand, with the previous right-primary
meaning elsewhere. Audited right-weapon PCBehavior16 follows that same role.
The low-level calculation, MoveEvaluator validation and both optimizer
preflights share this rule, retaining raw source values. Secondary context is
still required when an event actually selects the other hand. Mounted DEFAULT
sources and arbitrary mixed-pair behavior retain their previous scope.

Independent raw `expected_beast_claw_6800_moves.inc` verifies every move/event,
all12 MV channels, flags, timing/order/source and alias/behavior/attack IDs.
Portable tests use different left/right powers to prove the chosen hand, cover
all six offhand IDs and explicit/default/PC sources, secondary context, totals,
selected-hit scores from both optimizers, and both UI tabs without a matching
pair. Existing real XML/JSON/UI cases cover all14 new variants, offhand/Both
single-weapon evaluation, shared identity and five critical target queries.

Baseline83/83 passed in124.15s; final85/85 passed in117.57s
(`build/beast-claw-tests.log`); app/core/Catch2 build and CLI JSON round-trip pass.
Complete previous-value/shared-pointer identity comparison removes only new62
assignments. Coverage169bases/1165variants/80sets; optimizer eligibility1165ordinary/1122powerstance.

The read-only beast-claws junction view preserves previous inputs and supplies
148 fresh a62 clips from the already extracted full archive; retaineda62 had0.
No archive was copied/unpacked and no production dependency changed.

# Ordinary paired backhand blades

The complete audited profile is type92/motion58/behavior6400–6499, stances0/0,
special0/catalyst0/isDualBlade1/enableThrow1/bothHandEquipable1. Backhand Blade
and Reverse-Bladed Sword add two bases/26 affinities sharing exact6400.
Smithscript Cirque and Curseblade’s Cirque remain outside the ordinary profile
because their native special categories are251/265.

Native HKS selects six ordinary/offhand and six matching-powerstance light
stages. Its ordinary special-attack guard does not admit backhand blades.
The complete graph bridge resolves all63 noncritical definitions, preserves
non-arithmetic clip tests, and rejects missing sixth stages and unavailable
both-hand flags on any shared group member. Five explicit target-critical
contexts also resolve the native graph’s selected clips and hit data.

`backhandBladeCatalog` reuses the existing six-stage vocabulary with ordinary
type1 running/rolling R1. Both right mounted charging/release phases select
**730/735**, as do the left phases; this follows raw a58 events rather than
the conventional630/635. All68 moves/105 ordered selected weapon events are
checked by independent `expected_backhand_6400_moves.inc`, including all12 MV
channels, timing, source/index, aliases, behavior/attack IDs and target flags.
Selected StateInfo values are zero/absent and all opposing-target flags true.
Four jumping PCBehavior4/judge550 body events remain outside weapon-event scope.
Both-state and offhand events use DEFAULT0; powerstance retains explicit1/2
and DEFAULT0 events. Existing calculator, serializer, UI and both optimizers
handle them without new models or per-weapon exceptions.

The read-only `backhand-blades` view preserves641 prior TAE targets and links
emptya58 to131 clips from the already recovered full archive. No archive was
unpacked again, retained inputs changed, or bundled JSON regenerated.
`11711000.graph-backhand-blades.json` has14673723bytes/3296weapons/1191with
movesets. The full previous-value/shared-identity comparison removes ONLY
new58 assignments and passes (`build/backhand-data-comparison.log`). Coverage
is171bases/1191variants/81sets; ordinary/powerstance optimizer eligibility is
1191/1148. Existing Catch2 case counts remain85both-input/77XML-only/73portable.
Baseline85/85 passed in116.99s; final85/85 passed in127.05s
(`build/backhand-final-tests.log`), with app/core/Catch2 builds passing.

# Ordinary paired martial arts

The complete audited profile is type88/motion55/behavior6300–6399, stances0/0,
special0/catalyst0/isDualBlade1/enableThrow1/bothHandEquipable1. Dryleaf Arts
adds one base/13 affinities sharing exact6300. Dane’s Footwork uses special246
and remains outside the ordinary profile.

Native HKS selects six ordinary/offhand and six matching-powerstance light
stages, admits matching martial-art pairs and excludes ordinary feints.
`martialArtsCatalog` reuses the existing six-stage vocabulary with type1 for
both 1H movement R1 states and mounted630/635 (right),730/735 (left) phases.
All68 moves/134 ordered selected weapon events are checked against independent
`expected_martial_6300_moves.inc`: all12 MV channels, timing/order/source/index,
aliases, behavior/attack IDs and target flags. All selected StateInfo values
are zero/absent and all opposing-target flags true. Five jumping PCBehavior4/
judge550 body events stay outside weapon-event scope. There are no selected
PCBehavior events, feints or shieldpoke. Both/offhand events use DEFAULT0;
powerstance uses explicit1/2. Existing calculator/serializer/UI/both optimizers
handle this data without new models or per-weapon production exceptions.

Critical and backstab-whiff clips aliasa42; small-PvE riposte31730 ultimately
usesa42/31720’s two hits, preserving exact6300 behavior resolution. The current
read-only `martial-arts` view preserves641 targets and links emptya55 to148
clips from the already recovered archive. Alias dependencies are already in
the previous view. No archive was unpacked, retained inputs altered or bundled
JSON regenerated. Never unpack/delete through junctions or run generate_game_data.

The full graph bridge checks all63 noncritical mappings, missing sixth stages,
non-arithmetic clips, every profile-field mutation/deletion and unavailable
both-hand flags on later group members. Five native target-critical contexts
add135queries over27exact profiles in total. Full real integration checks
all13 affinities, Both/offhand single-rating queries, every move/hit, JSON
pointer identity and both application optimizers.

`11711000.graph-martial-arts.json` has14740384bytes/3296weapons/1204with movesets.
Full previous-value/shared-identity comparison passes, removing ONLY new55
assignments (`build/martial-data-comparison.log`). Coverage172bases/1204variants/
82sets; optimizer eligibility1204ordinary/1161powerstance. Existing Catch2
counts remain85both-input/77XML-only/73portable. Baseline85/85 passed in102.44s;
final85/85 passed in124.60s (`build/martial-tests.log`); existing app/core/
Catch2 build passes.

# Opposing-target eligibility

`ResolvedHit::opposeTarget` retains the optional default-merged AtkParam flag.
The pinned [Paramdex AtkParam definition](https://github.com/soulsmods/Paramdex/blob/ff7245e524329bc3eab00036723d2bd53384cedf/ER/Defs/AtkParam.xml#L480-L500)
identifies it as opposing-target eligibility. XML values must be exactly 0/1;
malformed/non-finite values reject the move. Older JSON lacking the field keeps
its previous evaluation behavior. No custom reflection handling is needed.

For opposing-target powers, a false flag makes an event inactive before source,
MV and StateInfo validation. It retains its hit number and contributes zero in
all twelve channels to evaluation and both optimizers. This is independent of
actual contact, hitboxes, critical bonuses or enemy defense. The UI uses its
existing inactive-hit display.

Base 6700 has 26 such events (attacks 6700992/997/998/999) despite nonzero 100 MVs.
All raw events remain in the snapshot; target flags, rather than their IDs or
indices, decide eligibility. Five existing great-hammer sets each have ten
ineligible helper events (1200001/2), whose MVs were already zero. Portable
checks cover defaults/overrides, malformed flags, legacy JSON, stable ordering,
missing second-weapon/unknown-condition bypass for ineligible events, and both
optimizer whole-move/selected-hit scores. Real JSON/UI cases exercise all variants.

# Referenced animations and retained files

The Dagger's 031750 file uses `ImportOtherAnim` to reference a23 / 031750. The
cache follows that reference for attack events while resolving behavior with
Dagger variation 100. Requested animation IDs and original event-file category/ID
are both retained. Local sound events on that alias are ignored by the attack
cache. `ImportOtherAnim` selects the referenced animation's event source;
local attack rows are inactive and are never merged, including malformed local
attack rows. The cache retains cycle, missing-target and invalid-target checks.
The pinned [DSAnimStudio reference-chain resolver](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/DSAProj/DSAProj.cs#L1513-L1544)
and [playback event-source assignment](https://github.com/Meowmaritus/DSAnimStudio/blob/f1bff06cd422de991b0a0fa8a2da81db43417318/DSAnimStudioNETCore/NewAnimationContainer.cs#L132-L146)
establish this behavior. The real great-katana 031719 alias has local judge
590/509 rows but imports a32/031719; regression expectations retain the target's
timing and provenance. Header layout is documented in the
[TAE format definition](https://github.com/Meowmaritus/SoulsAssetPipeline/blob/d11caf989c917c7a43e2c7559915b1c5af218153/SoulsAssetPipeline/Animation/TAE/TAE.Animation.cs).

No additional archive is required for these two catalogs. Importing reads the
existing unpacked directory and retains only extracted event sequences in the
animation cache; it does not delete or regenerate the input XML. Archive-by-archive
unpacking/cleanup can be added if measured disk usage warrants it, while preserving
the development workflow that reuses retained unpacked data.

# Partial special category regression

The current XML input is `build/development-xml/footwork-audit`. It preserves
the ordinary martial-art view and adds28 special246 clips. Existing exported
coverage remains172bases/1204variants/82sharedsets; Dane's Footwork is still
excluded until its Bullet reference is supported.

The representative `footworkCatalog` and independent
`expected_footwork_6301_moves.inc` verify67 complete moves/136 events. Native
graph resolution chooses26 special overrides and falls back to base55 for
absent registrations. The entire Both guard counter must fail with owned
Behavior106301311/refType1/Bullet200031311 provenance, even though the6300
family has a direct zero-MV helper. Partial moves must not reach GameData.
The native target-critical comparison includes this profile (140 queries in
total). Portable special/base tests cover non-arithmetic clip IDs, fallback
and rejection of unaudited special categories. This is a scoped inferred
priority policy, not a recovered universal native algorithm.

Recovered Bullet XML is under the physical ignored workspace audit directory,
not the junction view. All recovered weapon/behavior/attack rows match retained
data. Bullet200031311 explicitly linksAtk6301311; emission/shared-hit semantics
still need auditing. See [mapping evidence](../docs/moveset-mapping.md#scoped-special-martial-art-routing-and-the-bullet-boundary).

Final85/85 passed in130.63s (`build/footwork-final-tests.log`), with the
application/core/Catch2 builds passing and coverage/benchmarks OFF. Existing
85both-input/77XML-only/73portable counts are unchanged. CLI semantic save/load
`11711000.graph-footwork-audit.json` preserves every prior value/shared identity
(`build/footwork-data-comparison.log`). Bundled JSON and retained inputs unchanged.

# Opt-in Bullet references

Current input view: `build/development-xml/footwork-bullets`. It preserves all642
TAE links and messages and uses recovered, matching regulation XML including
Bullet.param.xml. These optional real checks run when BulletParamFile exists in
the configured XML root. No extra CMake option, test executable or game archive
is needed; older roots still run direct-attack regressions.

Explicitly pass a Bullet ParamTable as the final pointer argument to
resolveAttackEvents/importMoveset to inspect single-shot leaf links. Existing
default calls reject Bullet references. `ResolvedHit::bulletParamId` preserves
the link in existing JSON; active records remain unsupported in calculation
and both optimizers until projectile context is verified. Native graph and raw
XML diagnostic imports resolve68 Footwork moves/140 linked records, while the
original67 direct-move snapshot and no-table whole-counter rejection remain.
No Footwork affinity is exported yet. See [mapping details](../docs/moveset-mapping.md#opt-in-bullet-reference-resolution).

Two new portable Catch2 cases cover link/default/ambiguity/category failures,
unsupported child/interval/multiple/continuous paths, legacy/current JSON and
calculation/optimizer support boundaries. Full **87/87 in 145.78s**
(`build/bullet-reference-tests.log`), plus three targeted Bullet/raw-XML cases after the
final fixture assertions. Counts87both-input/79XML-only/75portable. Existing
app/core/Catch2 builds pass, coverage/benchmarks OFF. GameData coverage remains
172bases/1204variants/82sets. CLI `11711000.graph-footwork-bullets.json` retains
every previous value/shared identity (`build/bullet-reference-data-comparison.log`).

# BulletBehavior event controls

The importer now explicitly reads type2 BulletBehavior through
extractAttackEvents/cache.get's fourth includeBulletBehaviors argument, or
MoveDefinition.eventType=2 with a supplied Bullet table. The third cache argument
still selects PCBehavior. Default calls retain direct/critical event scope.

The portable control case covers strict parsing, raw False/zero JSON preservation,
source/index/order/timing, wrong reference-type rejection and calculation guards.
The existing import case verifies all four cache modes, alias isolation, missing
table rejection and explicit component import. Real XML independently verifies
all54 type2 records over both martial-art vocabularies using
expected_martial_bullet_events.inc, including all12 MV channels and raw controls.
Both guard counters have separate type2 160+160-MV records in addition to the
previously inspected type1 references. Neither the old134/140 record counts nor
the new27-per-vocabulary projectile count establishes contact/emission count.

**Current exported coverage and scores retain direct/critical/verified PC scope;
martial-art projectile contributions are not included.** Active Bullet records
remain unsupported for calculation, with no new production weapon admission.
See [projectile scope and evidence](../docs/moveset-mapping.md#bulletbehavior-events-and-projectile-coverage).

Full **88/88 in180.41s**, five targeted importer/Bullet/raw-XML checks after
the54-record snapshot, and a final focused false/zero JSON check all pass.
Counts88both-input/80XML-only/76portable. App/core/Catch2 builds pass; coverage/
benchmarks OFF. CLI `11711000.graph-bullet-behavior.json` preserves every previous
value/shared identity:172bases/1204variants/82sets. Inputs and bundled JSON unchanged.

# Explicit composed phases

`MoveDefinition::additionalEventSelections` composes explicit event-type and
judge/category/behavior selections into one existing Move. Tests verify original
record order, equal-time records, reversed selector declarations, restrictions,
JSON and graph metadata preservation. Overlap, missing phases and failed
references reject the entire move. Combined missing-phase/reference failures
retain all owned diagnostics; absent extras preserve legacy definition JSON.

Real XML regressions compose six Both-counter records for variations 6300/6301
and two records per mounted release. Charging remains separate; absent charging
Bullets never borrow release data. These records remain diagnostic: native
emission/contact rules and projectile calculation support are unresolved.
See [composition details](../docs/moveset-mapping.md#explicit-composition-of-attack-event-phases).

The existing full suite and app/core/Catch2 builds are used; see the latest
handoff for final results. Coverage/benchmarks OFF. Counts remain 88 with both
inputs / 80 XML-only / 76 portable. CLI `11711000.graph-bullet-phases.json`
preserves all prior data/sharing: 172 bases / 1204 variants / 82 sets with
direct/critical/verified-PC scope. Inputs and bundled JSON remain unchanged.

Final full regression suite: **88/88 in 129.10s**
(`build/bullet-phase-report-tests.log`); final app/core/Catch2 build passed
(`build/bullet-phase-report-build.log`). Diff and UTF-8 checks pass.

# Owned Bullet parameter metadata

The new portable `moveset - Bullet parameter ownership and serialization` case
checks default/row merging, independent damping channels, unknown fields,
false/zero/-1, malformed values and ownership after XML reload. It round-trips
complete GameData through the existing serializer and verifies shared Moveset
identity. The raw XML case checks all 37 distinct selected Bullet rows against
`expected_martial_bullet_parameters.inc`, generated independently from native
XML rather than importer output. Calculation regressions reject metadata missing
its Bullet ID in calculate_move, MoveEvaluator and both optimizers.

Baseline 88/88 in 192.10s; final **89/89 in 176.98s**
(`build/bullet-parameter-tests.log`), existing app/core/Catch2 build passes
(`build/bullet-parameter-build.log`). Coverage/benchmarks OFF. Counts now 89 with
both inputs / 81 XML-only / 77 portable. CLI
`11711000.graph-bullet-parameters.json` preserves all prior data/sharing; production
coverage remains 172 bases / 1204 variants / 82 shared sets. Inputs and bundled
JSON remain unchanged. Projectile calculation and native emission/contact rules
are still unresolved. See [metadata details](../docs/moveset-mapping.md#bullet-parameters-retained-after-import).

# Independent selected-hit evaluation

The portable `move calculation - selected hits isolate unsupported siblings`
case checks Bullet, unknown source/MV/StateInfo and missing-second-weapon siblings
in both orders. It verifies original indices, bounds, empty/missing moves,
matching/mismatched pairs, stance, inactive records and agreement with both
optimizers' selected-hit scores. The new UI regression round-trips mixed direct/
Bullet data and checks stats versus optimizer eligibility. Existing UI cases
also verify independent selection with unknown StateInfo and missing-pair siblings.

Baseline **89/89 in 131.97s** (`build/projectile-contact-baseline.log`); final
**91/91 in 128.66s** (`build/selected-hit-tests.log`), app/core/Catch2 build
passes (`build/selected-hit-build.log`). Coverage/benchmarks OFF. Counts now
91 with both inputs / 83 XML-only / 79 portable. Exported data/scope is unchanged.
All hits remains strict; selected unsupported Bullets remain unscoreable.
See [selected-hit API](../docs/moveset-mapping.md#independent-selected-hit-calculation).

# Special straight-sword category110

The independent `expected_straight_110_201_moves.inc` snapshot covers all63 moves/
72 records for the shared Broadsword/Cane Sword/Stone-Sheathed Sword profile.
Existing portable importer tests verify full-signature admission with arbitrary
IDs, grouping by special context, field mutation/deletion and strict selected-
clip failures. Graph fixtures test non-arithmetic clips, special overrides,
absent-registration fallback and malformed-registration rejection. Native
GameData/JSON/UI/both-optimizer regressions include all27 new variants.

Baseline91/91 in131.08s; final **91/91 in 121.35s**
(`build/straight-110-tests.log`), app/core/Catch2 build passes
(`build/straight-110-build.log`). Coverage/benchmarks OFF. Counts remain91 with
both inputs /83 XML-only /79 portable. Coverage now175 bases/1231 variants/83
shared sets; optimizer eligibility1231 ordinary/1188 matching-powerstance.
CLI `11711000.graph-straight-110.json` preserves every previous value/shared
identity after removing only the new27 assignments. Current root/cache is
`build/development-xml/straight-special-110`; no archives or inputs changed.
See [routing details](../docs/moveset-mapping.md#special-straight-sword-family-110).


# Remaining straight-sword specials 111/112/113/117

The independent expected_straight_111_209_moves.inc, expected_straight_112_210_moves.inc,
expected_straight_113_202_moves.inc and expected_straight_117_208_moves.inc
snapshots cover all63 moves for the Short Sword, Miquellan Knight's Sword,
Carian Knight's Sword + Lazuli Glintstone Sword and Warhawk's Talon profiles
(72/72/72/78 records; Warhawk's Talon's heavies are double-hit). The shared
straightSwordHeavySpecialCatalog(int) sets all eight R2 overrides per special.
Portable tests verify eight overrides, full-signature admission with arbitrary
IDs, grouping by special context and field mutation/deletion (spAtkcategory now
mutates to114, outside the audited set). Graph fixtures retain the non-arithmetic
clip77 and absent-registration fallback checks. Native GameData/JSON/UI/both-
optimizer regressions include all29 new variants.

Baseline was the prior 91/91 (straight-110). Final **91/91 in 310.83s**
(uild/straight-specials-tests.log), app/core/Catch2 build passes
(uild/straight-specials-build.log). Coverage/benchmarks OFF. Counts remain91
with both inputs /83 XML-only /79 portable. Coverage now180 bases/1260 variants/
87 shared sets; optimizer eligibility1260 ordinary/1217 matching-powerstance.
CLI 11711000.graph-straight-specials.json preserves every previous value/shared
identity after removing only the new29 assignments. Current root/cache is
uild/development-xml/straight-specials; no archives or inputs changed.
See [routing details](../docs/moveset-mapping.md#remaining-straight-sword-specials-111112113117).
