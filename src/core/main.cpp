import std;
import erdo;

using namespace erdo::calculator;
using namespace erdo::witchy;


int main(int argc, char *argv[])
{
	auto integer = [](std::string_view text) {
		int value;
		const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
		if (error != std::errc{} || end != text.data() + text.size() || value < 0)
			throw std::runtime_error("Expected a nonnegative integer: " + std::string(text));
		return value;
	};
	if (argc == 10 && std::string_view(argv[1]) == "--resolve-target-throw") {
		try {
			BehaviorGraph graph;
			graph.load(argv[2]);
			ParamTable weapons, throws;
			weapons.load(argv[3]);
			throws.load(argv[4]);
			const PlayerThrowQuery query{integer(argv[6]), integer(argv[7]), integer(argv[8]), integer(argv[9])};
			const auto result = graph.resolveWeaponThrow({MoveId::BACKSTAB, "Target interaction", -1},
				weapons, integer(argv[5]), throws, query);
			if (!result) throw std::runtime_error(result.error());
			const auto candidates = playerThrowCandidates(throws, query);
			std::println("Target c{:04}, {} candidate rows -> a{:03}_{:06} (common hit-data clip; runtime eligibility not evaluated)",
				query.defenderCharacterId, candidates->size(), *result->motionCategory, result->animationId);
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Target throw resolution failed: {}", error.what());
			return 1;
		}
	}
	if (argc >= 6 && std::string_view(argv[1]) == "--resolve-throw") {
		try {
			BehaviorGraph graph;
			graph.load(argv[2]);
			ParamTable throws;
			throws.load(argv[3]);
			std::vector<int> priority;
			for (int i = 5; i < argc; ++i) priority.push_back(integer(argv[i]));
			// Inspection label only; no automatic MoveId/target classification.
			const auto result = graph.resolvePlayerThrow({MoveId::BACKSTAB, "Selected throw", -1},
				throws, integer(argv[4]), priority);
			if (!result) throw std::runtime_error(result.error());
			std::println("Throw row {} -> a{:03}_{:06} (explicit row/category context; target eligibility not evaluated)",
				argv[4], *result->motionCategory, result->animationId);
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Throw resolution failed: {}", error.what());
			return 1;
		}
	}
	if (argc >= 7 && std::string_view(argv[1]) == "--resolve-heavy") {
		try {
			const std::string_view stance = argv[3], phase = argv[5];
			const int stage = integer(argv[4]);
			if ((stance != "1h" && stance != "2h" && stance != "2h-left")
				|| (stage != 1 && stage != 2) || (phase != "charged" && phase != "released"))
				throw std::runtime_error("Expected stance 1h/2h/2h-left, stage 1/2 and phase charged/released");
			const bool two = stance != "1h", charged = phase == "charged";
			const auto id = two
				? (charged ? (stage == 1 ? MoveId::TWO_HAND_CHARGED_R2_1 : MoveId::TWO_HAND_CHARGED_R2_2)
					: (stage == 1 ? MoveId::TWO_HAND_R2_1 : MoveId::TWO_HAND_R2_2))
				: (charged ? (stage == 1 ? MoveId::ONE_HAND_CHARGED_R2_1 : MoveId::ONE_HAND_CHARGED_R2_2)
					: (stage == 1 ? MoveId::ONE_HAND_R2_1 : MoveId::ONE_HAND_R2_2));
			std::vector<int> priority, effects;
			for (int i = 6; i < argc; ++i) {
				const std::string_view argument = argv[i];
				if (argument.starts_with("effect=")) effects.push_back(integer(argument.substr(7)));
				else priority.push_back(integer(argument));
			}
			BehaviorGraph graph;
			graph.load(argv[2]);
			const auto route = heavyAttackRoute(id, effects, stance == "2h-left");
			if (!route) throw std::runtime_error(route.error());
			const auto result = graph.resolveHeavyMove({id, "Heavy", -1}, priority, effects, stance == "2h-left");
			if (!result) throw std::runtime_error(result.error());
			std::println("{} -> a{:03}_{:06} (explicit active effects and category priority; clip routing only)",
				route->stateName, *result->motionCategory, result->animationId);
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Heavy resolution failed: {}", error.what());
			return 1;
		}
	}
	if (argc >= 6 && std::string_view(argv[1]) == "--resolve-chain") {
		try {
			const std::string_view stance = argv[5];
			const auto first = stance == "1h" ? MoveId::ONE_HAND_R1_1
				: stance == "2h" ? MoveId::TWO_HAND_R1_1
				: stance == "offhand" ? MoveId::OFFHAND_R1_1
				: stance == "powerstance" ? MoveId::POWERSTANCE_L1_1
				: throw std::runtime_error("Expected stance 1h, 2h, offhand or powerstance");
			BehaviorGraph graph;
			graph.load(argv[2]);
			ParamTable weapons;
			weapons.load(argv[3]);
			std::vector<int> priority;
			for (int i = 6; i < argc; ++i) priority.push_back(integer(argv[i]));
			const auto chain = priority.empty() ? graph.resolveLightChain(weapons, integer(argv[4]), first)
				: graph.resolveLightChain(weapons, integer(argv[4]), first, priority);
			if (!chain) throw std::runtime_error(chain.error());
			std::println("Ordinary light chain: {} moves (local HKS rules, {}; conditional routing excluded)", chain->size(),
				priority.empty() ? "weapon-parameter category policy" : "explicit category priority");
			if (stance == "powerstance") std::println("Assumes a matching weapon pair.");
			if (stance == "offhand") std::println("Assumes an attacking left-hand request, not a guard or powerstance request.");
			for (const auto& definition : *chain)
				std::println("{} -> a{:03}_{:06}", definition.name, *definition.motionCategory, definition.animationId);
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Chain resolution failed: {}", error.what());
			return 1;
		}
	}
	if (argc >= 6 && std::string_view(argv[1]) == "--resolve-state") {
		try {
			BehaviorGraph graph;
			graph.load(argv[2]);
			std::vector<int> priority;
			BehaviorVariables variables;
			for (int i = 5; i < argc; ++i) {
				const std::string_view argument = argv[i];
				const auto equals = argument.find('=');
				if (equals == std::string_view::npos) priority.push_back(integer(argument));
				else if (equals == 0 || !variables.emplace(std::string(argument.substr(0, equals)), integer(argument.substr(equals + 1))).second)
					throw std::runtime_error("Expected a unique nonempty behavior variable name");
			}
			// Only inspect the selected state/clip; this command does not publish
			// a MoveId or assert that HKS allows the weapon to reach this state.
			const auto result = graph.resolve({MoveId::ONE_HAND_R1_1, argv[3], -1}, argv[3], integer(argv[4]), priority, variables);
			if (!result)
				throw std::runtime_error(result.error());
			std::println("{} -> a{:03}_{:06} (explicit category priority; reachability not evaluated)",
				argv[3], *result->motionCategory, result->animationId);
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Graph resolution failed: {}", error.what());
			return 1;
		}
	}

	// Reuse already-unpacked XML without invoking Witchy or modifying the bundled data.
	if (argc > 1) {
		const bool graphImport = argc == 5 && std::string_view(argv[1]) == "--import-graph";
		if (!graphImport && (argc != 4 || std::string_view(argv[1]) != "--import")) {
			std::println(std::cerr, "Usage: erdo_core_exe --import <unpacked-xml-directory> <output.json>");
			std::println(std::cerr, "       erdo_core_exe --import-graph <unpacked-xml-directory> <behavior.xml> <output.json>");
			std::println(std::cerr, "       erdo_core_exe --resolve-state <behavior.xml> <state> <offset-type> <category> [fallback-category ...] [variable=value ...]");
			std::println(std::cerr, "       erdo_core_exe --resolve-chain <behavior.xml> <EquipParamWeapon.xml> <weapon-id> <1h|2h|offhand|powerstance> [category ...] (special-category offhand/powerstance requires explicit categories)");
			std::println(std::cerr, "       erdo_core_exe --resolve-heavy <behavior.xml> <1h|2h|2h-left> <1|2> <charged|released> <category> [fallback-category ...] [effect=id ...]");
			return 1;
		}
		try {
			const std::filesystem::path output = argv[graphImport ? 4 : 3];
			if (std::filesystem::exists(output))
				throw std::runtime_error("Output already exists; choose a new development JSON path");
			const auto game_data = [&] {
				if (!graphImport) return load_game_data(argv[2]);
				BehaviorGraph graph;
				graph.load(argv[3]);
				return load_game_data(argv[2], graph);
			}();
			if (output.has_parent_path())
				std::filesystem::create_directories(output.parent_path());
			erdo::json::save(output, game_data);
			const auto restored = erdo::json::load<GameData>(output);
			if (game_data.game_version != restored.game_version || game_data.weapons.size() != restored.weapons.size())
				throw std::runtime_error("Generated game data metadata changed during JSON round-trip");
			for (const auto& [original, loaded] : std::views::zip(game_data.weapons, restored.weapons)) {
				if (original.full_name != loaded.full_name || bool(original.moveset) != bool(loaded.moveset)
					|| (original.moveset && erdo::json::write(*original.moveset) != erdo::json::write(*loaded.moveset)))
					throw std::runtime_error("Generated weapon moves changed during JSON round-trip");
			}
			std::println("Saved game version {}: {} weapons, {} with movesets, to {}",
				restored.game_version, restored.weapons.size(),
				std::ranges::count_if(restored.weapons, [](const auto& weapon) { return bool(weapon.moveset); }),
				std::filesystem::absolute(output).string());
			if (graphImport)
				std::println("Ordinary, offhand and matching-powerstance mappings come from the graph; remaining moves use validated catalogs.");
			return 0;
		} catch (const std::exception& error) {
			std::println(std::cerr, "Import failed: {}", error.what());
			return 1;
		}
	}

	// auto gd = erdo::witchy::generate_game_data(
	// 	"F:/Programme/Steam/steamapps/common/ELDEN RING/Game/eldenring.exe",
	// 	"C:/Users/Paul/Downloads/WitchyBND-3.0.1.0-win-x64/WitchyBND.exe",
	// 	false
	// );
	// if (!gd)
	// 	throw std::runtime_error(std::format("Error: {}", gd.error()));
	// auto game_data = erdo::witchy::load_game_data(
	// 	"C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data"
	// );



	std::filesystem::path temp_dir = "C:/Users/Paul/AppData/Local/Temp/elden-ring-damage-optimizer/unpacked_data";

	ParamTable attacks;
	attacks.load((temp_dir / AtkParam_PcFile).string().c_str());

	ParamTable behaviors;
	behaviors.load((temp_dir / BehaviorParam_PcFile).string().c_str());

	BehaviorIndex behaviorIndex;
	behaviorIndex.build(behaviors);

	xml_document tae;

	if (!tae.load_file((temp_dir / "chr/c0000-anibnd-dcx-wanibnd/INTERROOT_win64/chr/c0000/tae/a20-tae/anim-034000.xml").string().c_str()))
		throw std::runtime_error("Could not load Dagger TAE");

	AnimationCache animationCache(temp_dir);

	/*importMoveset(
		animationCache,
		23,   // Straight-sword motion category
		200,  // Longsword behavior variation
		straightSwordCatalog,
		behaviorIndex,
		attacks,
		[](const MoveDefinition& move,
		const std::vector<ResolvedHit>& hits)
		{
			std::println(
				"\n{} | Animation {:06} | {} hit(s)",
				move.name,
				move.animationId,
				hits.size()
			);

			for (const auto& hit : hits) {
				constexpr auto physical =
					std::to_underlying(AttackPowerType::PHYSICAL);
				constexpr auto poison =
					std::to_underlying(AttackPowerType::POISON);

				std::println(
					"  judge={:<4} source={:<2} "
					"time={:.3f}-{:.3f} "
					"physical={:<5} status={:<5} "
					"AtkParam={}",
					hit.event.behaviorJudgeId,
					hit.event.source,
					hit.event.startTime,
					hit.event.endTime,
					hit.motionValues.values[physical],
					hit.motionValues.values[poison],
					hit.atkParamId
				);
			}
		}
	);*/

	for (int judge : {100, 150})
	{
		const auto& matches = behaviorIndex.find(200, judge);

		std::println(
			"\nVariation 200, judge {}: {} behavior row(s)",
			judge, matches.size()
		);

		for (const auto row : matches)
		{
			const int refType = row.attribute("refType").as_int(-1);
			const int refId   = row.attribute("refId").as_int(-1);

			std::println(
				"  Behavior={} refType={} refId={} "
				"category={} stamina={}",
				row.attribute("id").value(),
				refType,
				refId,
				row.attribute("category").value(),
				row.attribute("stamina").value()
			);

			if (refType == 0)
			{
				const auto attack = attacks.find(refId);

				if (attack)
				{
					std::println(
						"    AtkParam={} physical={} status={}",
						refId,
						attacks.number(attack, "atkPhysCorrection")
							.value_or(-1),
						attacks.number(
							attack,
							"statusAilmentAtkPowerCorrectRate"
						).value_or(-1)
					);
				}
				else
				{
					std::println("    AtkParam row not found");
				}
			}
		}
	}

	return 0;
}
