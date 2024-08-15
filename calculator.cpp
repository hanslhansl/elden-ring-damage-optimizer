
#include "calculator.hpp"



constexpr const char* calculator::attribute_to_json_string(Attribute at)
{
	switch (at)
	{
	case Attribute::STRENGTH:		return "str";
	case Attribute::DEXTERITY:		return "dex";
	case Attribute::INTELLIGENCE:	return "int";
	case Attribute::FAITH:			return "fai";
	case Attribute::ARCAINE:		return "arc";
	}

	throw std::invalid_argument("invalid attribute");
}

bool calculator::Weapon::Filter::operator()(const Weapon& weapon) const
{
	auto satisfies = [](const auto& set, const auto& val) { return set.empty() or set.contains(val); };

	return satisfies(this->base_names, weapon.base_name) and
		satisfies(this->dlc, weapon.dlc) and
		satisfies(this->types, weapon.type) and
		satisfies(this->affinities, weapon.affinity);
}

calculator::Stats calculator::Weapon::adjust_stats_for_two_handing(bool two_handing, Stats stats) const
{
	// Paired weapons do not get the two handing bonus
	if (this->paired)
		two_handing = false;

	// Bows and ballistae can only be two handed
	constexpr std::array<Weapon::Type, 4> bow_types = { Weapon::Type::LIGHT_BOW, Weapon::Type::BOW, Weapon::Type::GREATBOW, Weapon::Type::BALLISTA };
	if (std::ranges::contains(bow_types, this->type))
		two_handing = true;

	if (two_handing)
		stats.at(Attribute::STRENGTH) = stats.at(Attribute::STRENGTH) * 1.5;

	return stats;
}

void calculator::from_json(const json& j, CalcCorrectGraphEntry& c)
{
	j.at("maxVal").get_to(c.maxVal);
	j.at("maxGrowVal").get_to(c.maxGrowVal);
	j.at("adjPt").get_to(c.adjPt);
}

void calculator::to_json(json& j, const CalcCorrectGraphEntry& c)
{
	j = json{
		{ "maxVal", c.maxVal },
		{ "maxGrowVal", c.maxGrowVal },
		{ "adjPt", c.adjPt }
	};
}

void calculator::from_json(const json& j, ReinforceTypesDict& r)
{
	auto&& attack_json = j.at("attack");
	for (auto dmg_type : DamageType::_values())
		r.attack.at(dmg_type._to_integral()) = attack_json.value(std::to_string(dmg_type._to_integral()), floating(0));

	r.attributeScaling = j.at("attributeScaling").get<AttributeScaling>();

	r.statusSpEffectId.at(0) = j.value("statusSpEffectId1", 0);
	r.statusSpEffectId.at(1) = j.value("statusSpEffectId2", 0);
	r.statusSpEffectId.at(2) = j.value("statusSpEffectId3", 0);
}

calculator::AttackRating::full calculator::OptimizationContext::wait_and_get_result()
{
	// wait for all threads to finish
	this->pool.wait();

	misc::printl();

	// loop through all optional attack ratings and get the best one
	auto loop_lambda = [](auto& vec)
		{
			auto sparse_result_it = std::ranges::max_element(vec, {}, &std::remove_reference_t<decltype(vec)>::value_type::value);
			if (sparse_result_it == vec.end())
				return std::pair{ -1ll, Stats{} };
			auto index = std::distance(vec.begin(), sparse_result_it);
			return std::pair{ index, sparse_result_it->stats };
		};

	auto [index, stats] = std::visit(loop_lambda, this->optional_results);

	if (index == -1)
		return AttackRating::full{};

	AttackRating::full best_attack_rating{};
	this->weapons[index]->get_attack_rating(this->attack_options, stats, best_attack_rating);
	return best_attack_rating;
}

calculator::ScalingCurve calculator::WeaponContainer::evaluate_CalcCorrectGraph(const CalcCorrectGraph& calcCorrectGraph)
{
	ScalingCurve arr{};

	for (size_t i = 1; i < calcCorrectGraph.size(); i++)
	{
		auto& prevStage = calcCorrectGraph.at(i - 1);
		auto& stage = calcCorrectGraph.at(i);

		auto minAttributeValue = i == 1 ? 1 : prevStage.maxVal + 1;
		auto maxAttributeValue = (i == calcCorrectGraph.size() - 1) ? 148 : stage.maxVal;

		auto attributeValue = minAttributeValue;
		while (attributeValue <= maxAttributeValue)
		{
			if (not arr.at(attributeValue))
			{
				auto ratio = floating(attributeValue - prevStage.maxVal) / floating(stage.maxVal - prevStage.maxVal);

				if (prevStage.adjPt > 0)
					ratio = std::pow(ratio, prevStage.adjPt);
				else if (prevStage.adjPt < 0)
					ratio = 1 - std::pow((1 - ratio), -prevStage.adjPt);

				arr[attributeValue] = prevStage.maxGrowVal + (stage.maxGrowVal - prevStage.maxGrowVal) * ratio;
			}
			attributeValue += 1;
		}
	}

	return arr;
}

calculator::WeaponContainer::WeaponContainer(const std::filesystem::path& file_path)
{
	json data = json::parse(std::ifstream(file_path));

	for (auto&& [id_, calcCorrectGraph] : data.at("calcCorrectGraphs").items())
		this->calcCorrectGraphsById.emplace(std::stoi(id_), evaluate_CalcCorrectGraph(calcCorrectGraph.get<CalcCorrectGraph>()));

	for (auto&& [id, attackElementCorrect] : data.at("attackElementCorrects").items())
	{
		auto&& [inserted, success] = this->attackElementCorrectsById.emplace(std::stoi(id), attackElementCorrect);
		constexpr auto default_ = AttributeScaling{ false, false, false, false, true }; // default value
		inserted->second[AttackPowerType::POISON] = default_;
		inserted->second[AttackPowerType::BLEED] = default_;
		inserted->second[AttackPowerType::MADNESS] = default_;
		inserted->second[AttackPowerType::SLEEP] = default_;
	}

	map<int, std::vector<ReinforceTypesDict>> reinforceTypes{};
	for (auto&& [id, reinforceType] : data.at("reinforceTypes").items())
		reinforceTypes.emplace(std::stoi(id), reinforceType);

	map<int, map<AttackPowerType, int>> statusSpEffectParams{};
	for (auto&& [key, val] : data.at("statusSpEffectParams").items())
		statusSpEffectParams.try_emplace(std::stoi(key), val.get<map<AttackPowerType, int>>());

	this->scalingTiers = data.at("scalingTiers").get<decltype(this->scalingTiers)>();

	auto create_weapon = [&](const json& weapon_data) {
		auto&& attackElementCorrect = this->attackElementCorrectsById.at(weapon_data.at("attackElementCorrectId").get<int>());

		const auto& reinforceParams = reinforceTypes.at(weapon_data.at("reinforceTypeId").get<int>());

		auto calcCorrectGraphIds = weapon_data.at("calcCorrectGraphIds").get<map<AttackPowerType, int>>();
		std::array<const ScalingCurve*, AttackPowerType::_size()> weaponCalcCorrectGraphs{};
		for (auto damage_type : DamageType::_values())
			weaponCalcCorrectGraphs.at(damage_type._to_integral()) = &this->calcCorrectGraphsById.at(misc::map_get(calcCorrectGraphIds, AttackPowerType::_from_integral_unchecked(damage_type), defaultDamageCalcCorrectGraphId));
		for (auto status_type : StatusType::_values())
			weaponCalcCorrectGraphs.at(status_type._to_integral()) = &this->calcCorrectGraphsById.at(misc::map_get(calcCorrectGraphIds, AttackPowerType::_from_integral_unchecked(status_type), defaultStatusCalcCorrectGraphId));

		auto unupgradedAttack = weapon_data.at("attack").get<std::vector<std::pair<AttackPowerType, int>>>();
		auto statusSpEffectParamIds = weapon_data.value("statusSpEffectParamIds", std::array<int, 3>{});
		std::vector<std::array<floating, AttackPowerType::_size()>> attack{};
		for (const auto& reinforceParam : reinforceParams)
		{
			auto& attack_at_upgrade_level = attack.emplace_back();
			for (const auto& [attackPowerType, unupgradedAttackPower] : unupgradedAttack)
				attack_at_upgrade_level.at(attackPowerType._to_integral()) = unupgradedAttackPower * reinforceParam.attack.at(attackPowerType._to_integral());

			int i = 0;
			for (const auto& spEffectParamId : statusSpEffectParamIds)
			{
				if (spEffectParamId)
				{
					const auto& statusSpEffectParam = statusSpEffectParams.at(spEffectParamId + reinforceParam.statusSpEffectId.at(i));
					for (const auto& [apt, val] : statusSpEffectParam)
						attack_at_upgrade_level.at(apt._to_integral()) = val;
				}
				i++;
			}
		}

		auto unupgradedAttributeScaling = weapon_data.at("attributeScaling").get<std::vector<std::pair<Attribute, floating>>>();
		std::vector<AttributeScaling> attributeScaling{};
		for (const auto& reinforceParam : reinforceParams)
		{
			auto& foo = attributeScaling.emplace_back();
			for (const auto& [attribute, unupgradedScaling] : unupgradedAttributeScaling)
				foo.at(attribute._to_integral()) = unupgradedScaling * reinforceParam.attributeScaling.at(attribute._to_integral());
		}

		std::string weaponName = weapon_data.at("weaponName").get<std::string>();
		auto url_part = weaponName;
		std::ranges::replace(url_part, ' ', '_');

		return Weapon{
			weapon_data.at("name").get<std::string>(),
			std::move(weaponName),
			weapon_data.value("url", "https://eldenring.fandom.com/wiki/" + url_part),
			weapon_data.at("dlc").get<bool>(),
			weapon_data.value("paired", false),
			weapon_data.value("sorceryTool", false),
			weapon_data.value("incantationTool", false),
			weapon_data.at("weaponType").get<Weapon::Type>(),
			weapon_data.at("affinityId").get<Weapon::Affinity>(),
			weapon_data.at("requirements").get<Stats>(),
			std::move(attributeScaling),
			std::move(attack),
			std::move(attackElementCorrect),
			std::move(weaponCalcCorrectGraphs),
			this->scalingTiers };
		};

	const auto& weapons_data = data.at("weapons");
	this->weapons.reserve(weapons_data.size());
	for (auto&& weapon_data : weapons_data)
		this->weapons.emplace_back(create_weapon(weapon_data));

	misc::printl(this->weapons.size(), " weapons");
	misc::printl();
}

calculator::Weapon::AllFilterOptions calculator::WeaponContainer::get_all_filter_options() const
{
	Weapon::Filter filter{};
	for (const auto& weapon : this->weapons)
	{
		filter.dlc.insert(weapon.dlc);
		filter.types.insert(weapon.type);
		filter.affinities.insert(weapon.affinity);
		filter.base_names.insert(weapon.base_name);
	}

	Weapon::AllFilterOptions all_filter_options{ 
		{ filter.dlc.begin(), filter.dlc.end() },
		{ filter.types.begin(), filter.types.end() },
		{ filter.affinities.begin(), filter.affinities.end() },
		{ filter.base_names.begin(), filter.base_names.end() },
	};

	return all_filter_options;
}

calculator::FilteredWeapons calculator::WeaponContainer::apply_filter(const Weapon::Filter& weapon_filter) const
{
	FilteredWeapons filtered{};
	filtered.reserve(this->weapons.size());

	for (const auto& weapon : this->weapons)
		if (weapon_filter(weapon))
			filtered.push_back(&weapon);

	return filtered;
}

void calculator::test1()
{
	auto&& [weap_contain, weap_contain_time] = misc::TimeFunctionExecution([&]() { return WeaponContainer("D:\\Paul\\Computer\\Programmieren\\C++\\elden-ring-damage-optimizer\\damage-optimizer\\regulation-vanilla-v1.12.3.js"); });

	AttackOptions atk_options = { {25, 10}, true };
	auto [stat_variations, stat_variations_time] = misc::TimeFunctionExecution([&]() { return get_stat_variations(1 + 79, ALL_CLASS_STATS.at(Class::WRETCH)); });

	auto [filtered_weaps, filtered_weapons_time] = misc::TimeFunctionExecution([&]() { return weap_contain.apply_filter(Weapon::Filter{ {}, { }, {} }); });
	misc::printl();

	auto [attack_rating, attack_rating_time] = misc::TimeFunctionExecution([&]()
		{ return OptimizationContext(20, stat_variations, filtered_weaps, atk_options, std::type_identity<AttackRating::total>{}).wait_and_get_result(); });
	misc::printl();

	misc::print("stats: ");
	for (auto stat : attack_rating.stats)
		misc::print(stat, " ");
	misc::printl("\n");

	misc::printl(attack_rating.weapon->full_name, ": ", attack_rating.total_attack_power.at(2));
	misc::printl();

	for (int i = 0; i < attack_rating.attack_power.size(); i++)
		if (attack_rating.attack_power.at(i).at(2) != 0)
			misc::printl(DamageType::_from_integral(i)._to_string(), ": ", attack_rating.attack_power.at(i).at(0), " + ", attack_rating.attack_power.at(i).at(1),
				" = ", attack_rating.attack_power.at(i).at(2));
	misc::printl();

	for (int i = 0; i < attack_rating.status_effect.size(); i++)
		if (attack_rating.status_effect.at(i).at(2) != 0)
			misc::printl(StatusType::_from_index(i)._to_string(), ": ", attack_rating.status_effect.at(i).at(0), " + ", attack_rating.status_effect.at(i).at(1),
				" = ", attack_rating.status_effect.at(i).at(2));
	misc::printl();

	misc::printl("spell_scaling: ", attack_rating.spell_scaling, "%");
	misc::printl();

	misc::printl("load weapon container: ", weap_contain_time);
	misc::printl("get stat variations: ", stat_variations_time);
	misc::printl("filter weapons: ", filtered_weapons_time);
	misc::printl("query best stats: ", attack_rating_time);

	Sleep(100000);
}

void nlohmann::adl_serializer<calculator::AttributeScaling>::from_json(const json& j, calculator::AttributeScaling& ac)
{
	for (auto&& [attr_str, val] : j.items())
	{
		auto attr = calculator::attribute_json_string_table.to_enum(attr_str.c_str());

		if (val.is_boolean())
			ac[attr._to_integral()] = val.get<bool>();
		else if (val.is_number())
			ac[attr._to_integral()] = val.get<calculator::floating>();
		else
			throw std::invalid_argument(std::string("invalid json type") + val.type_name());
	}
}

void nlohmann::adl_serializer<calculator::AttackElementCorrects>::from_json(const json& j, calculator::AttackElementCorrects& aec)
{
	for (auto&& [apt_str, ac_j] : j.items())
	{
		auto apt_int = std::stoi(apt_str);
		auto apt = calculator::AttackPowerType::_from_integral(apt_int);

		aec[apt_int] = ac_j.get<calculator::AttributeScaling>();
	}
}

void nlohmann::adl_serializer<calculator::Stats>::from_json(const json& j, calculator::Stats& s)
{
	for (const auto& [attr_str, val] : j.items())
	{
		auto attr = calculator::attribute_json_string_table.to_enum(attr_str.c_str());
		s[attr._to_integral()] = val.get<int>();
	}
}

void nlohmann::adl_serializer<calculator::Stats>::to_json(json& j, const calculator::Stats& s)
{
	j = json::object();
	for (const auto& [attr_index, val] : std::views::enumerate(s))
	{
		if (val != 0)
		{
			auto attr_str = calculator::attribute_json_string_table.from_enum(calculator::Attribute::_from_integral(attr_index));
			j[attr_str] = val;
		}
	}
}

calculator::Attribute nlohmann::adl_serializer<calculator::Attribute>::from_json(const json& j)
{
	return calculator::attribute_json_string_table.to_enum(j.get<std::string>().c_str());
}

void nlohmann::adl_serializer<calculator::Attribute>::to_json(json& j, const calculator::Attribute& opt)
{
	j = calculator::attribute_json_string_table.from_enum(opt);
}

calculator::AttackPowerType nlohmann::adl_serializer<calculator::AttackPowerType>::from_json(const json& j)
{
	return calculator::AttackPowerType::_from_integral(j.get<int>());
}

void nlohmann::adl_serializer<calculator::AttackPowerType>::to_json(json& j, const calculator::AttackPowerType& opt)
{
	j = opt._to_integral();
}

calculator::Weapon::Affinity nlohmann::adl_serializer<calculator::Weapon::Affinity>::from_json(const json& j)
{
	return calculator::Weapon::Affinity::_from_integral(j.get<int>());
}

calculator::Weapon::Type nlohmann::adl_serializer<calculator::Weapon::Type>::from_json(const json& j)
{
	return calculator::Weapon::Type::_from_integral(j.get<int>());
}


template<typename T>
constexpr T assert_floating_is(calculator::floating f)
{
	if (f != (T)f)
		throw std::runtime_error("floating is not T");
	return f;
}
constexpr bool isVanilla = true;

std::vector<std::filesystem::path> calculator::Parser::copy_elden_ring_files(const std::filesystem::path& elden_ring, const std::filesystem::path& to)
{
	std::filesystem::create_directory(to);
	std::vector<std::filesystem::path> ret{};

	for (auto& elden_ring_file_path : Parser::needed_elden_ring_file_paths)
	{
		auto elden_ring_file_path_string = elden_ring_file_path.generic_string();
		std::ranges::replace(elden_ring_file_path_string, '/', '-');

		auto new_file = to / elden_ring_file_path_string;
		std::filesystem::copy_file(elden_ring / elden_ring_file_path, new_file, std::filesystem::copy_options::overwrite_existing);
		ret.push_back(new_file);
	}

	misc::printl("copy er files to ", to.string(), "\n");
	return ret;
}

void calculator::Parser::witchy(const std::filesystem::path& witchy_exe, const std::vector<std::filesystem::path>& files)
{
	std::stringstream ss{};
	ss << "\"" << witchy_exe << " --passive --parallel";
	for (auto& file : files)
		ss << " " << file;
	ss << "\"";

	std::system(ss.str().c_str());
}

std::vector<std::filesystem::path> calculator::Parser::witchy_unpack_files(const std::filesystem::path& witchy_exe, const std::vector<std::filesystem::path>& files_to_unpack)
{
	witchy(witchy_exe, files_to_unpack);

	std::vector<std::filesystem::path> ret{};
	for (auto& unpacked_file : files_to_unpack)
	{
		auto unpacked_filename = unpacked_file.filename().generic_string();
		std::ranges::replace(unpacked_filename, '.', '-');
		ret.push_back(unpacked_file.parent_path() / unpacked_filename);
	}
	return ret;
}

std::vector<std::filesystem::path> calculator::Parser::witchy_to_xml(const std::filesystem::path& witchy_exe, const std::vector<std::filesystem::path>& files_to_xml)
{
	witchy(witchy_exe, files_to_xml);

	std::vector<std::filesystem::path> ret{};
	for (auto& file_to_xml : files_to_xml)
	{
		auto xml_filename = file_to_xml.filename().generic_string() + ".xml";
		ret.push_back(file_to_xml.parent_path() / xml_filename);
	}
	return ret;
}

std::vector<std::filesystem::path> calculator::Parser::get_needed_unpacked_files(const std::vector<std::filesystem::path>& unpacked_directories)
{
	std::vector<std::filesystem::path> ret{};
	for (auto& unpacked_directory : unpacked_directories)
	{
		for (auto& unpacked_file : std::filesystem::directory_iterator(unpacked_directory))
		{
			if (Parser::needed_unpacked_files.contains(unpacked_file.path().filename()))
			{
				ret.push_back(unpacked_file.path());
			}
		}
	}

	return  ret;
}

std::filesystem::path calculator::Parser::copy_xml_files(const std::filesystem::path& parent_path, const std::vector<std::filesystem::path>& xml_files)
{
	auto xml_directory = parent_path / "xml_files";
	std::filesystem::create_directory(xml_directory);

	for (auto& xml_file : xml_files)
	{
		auto new_file_path = xml_directory / xml_file.filename();
		std::filesystem::copy_file(xml_file, new_file_path, std::filesystem::copy_options::overwrite_existing);
	}

	misc::printl("copy xml files to ", xml_directory.string(), "\n");
	return xml_directory;
}

std::map<long long, calculator::Parser::ParamRow> calculator::Parser::read_param_xml(const std::filesystem::path& file_path)
{
	auto actual_path = file_path.parent_path() / (file_path.filename().generic_string() + ".xml");

	pugi::xml_document data;
	auto result = data.load_file(actual_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
	if (!result)
		throw std::runtime_error("could not load xml file: " + std::string(result.description()));

	auto field_nodes = data.child("param").child("fields").children("field");

	ParamRow default_values{};
	for (auto&& field_node : field_nodes)
	{
		auto name = field_node.attribute("name").as_string();
		auto defaultValue = field_node.attribute("defaultValue").as_double(std::numeric_limits<floating>::max());

		if (defaultValue != std::numeric_limits<floating>::max())
			default_values.emplace(name, defaultValue);
	}

	auto row_nodes = data.child("param").child("rows").children("row");

	std::map<long long, ParamRow> ret{};
	for (auto&& row_node : row_nodes)
	{
		auto name = row_node.attribute("name").as_string();

		ParamRow row_data = default_values;
		for (auto&& row_attribute : row_node.attributes())
		{
			row_data[row_attribute.name()] = row_attribute.as_double();
		}

		auto id = row_node.attribute("id").as_llong();
		ret.emplace(id, std::move(row_data));
	}

	return ret;
}

std::map<long long, std::string> calculator::Parser::read_fmg_xml(const std::filesystem::path& file_path)
{
	auto actual_path = file_path.parent_path() / (file_path.filename().generic_string() + ".xml");

	pugi::xml_document data;
	auto result = data.load_file(actual_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
	if (!result)
		throw std::runtime_error("could not load xml file: " + std::string(result.description()));

	auto text_nodes = data.child("fmg").child("entries").children("text");

	std::map<long long, std::string> ret{};
	for (auto&& text_node : text_nodes)
	{
		auto id = text_node.attribute("id").as_llong();
		auto text = text_node.child_value();
		ret.emplace(id, text);
	}

	return ret;
}

bool calculator::Parser::is_unique_weapon(const calculator::Parser::ParamRow& row)
{
	return row.at("gemMountType") == 0 || row.at("disableGemAttr") == 1;
}

bool calculator::Parser::is_supported_weapon_type(size_t weapon_type)
{
	return calculator::Weapon::Type::_from_integral_nothrow(weapon_type);
}

std::map<calculator::AttackPowerType, calculator::floating> calculator::Parser::parse_status_sp_effect_params(long long statusSpEffectParamId) const
{
	if (!this->spEffectParams.contains(statusSpEffectParamId))
		return {};
	auto&& spEffectRow = this->spEffectParams.at(statusSpEffectParamId);

	std::map<calculator::AttackPowerType, floating> statuses = {
		{ AttackPowerType::POISON, spEffectRow.at("poizonAttackPower") },
		{ AttackPowerType::SCARLET_ROT, spEffectRow.at("diseaseAttackPower") },
		{ AttackPowerType::BLEED, spEffectRow.at("bloodAttackPower") },
		{ AttackPowerType::FROST, spEffectRow.at("freezeAttackPower") },
		{ AttackPowerType::SLEEP, spEffectRow.at("sleepAttackPower") },
		{ AttackPowerType::MADNESS, spEffectRow.at("madnessAttackPower") },
		{ AttackPowerType::DEATH_BLIGHT, spEffectRow.at("curseAttackPower") }
	};

	if (std::ranges::any_of(statuses, [](auto&& v) { return v.second != 0; }))
		return statuses;

	return {};
}

calculator::CalcCorrectGraph calculator::Parser::parse_calc_correct_graph(const ParamRow& row) const
{
	return CalcCorrectGraph{
		CalcCorrectGraphEntry{
			assert_floating_is<int>(row.at("stageMaxVal0")),
			row.at("stageMaxGrowVal0") / 100.,
			row.at("adjPt_maxGrowVal0")
		},
		CalcCorrectGraphEntry{
			assert_floating_is<int>(row.at("stageMaxVal1")),
			row.at("stageMaxGrowVal1") / 100.,
			row.at("adjPt_maxGrowVal1")
		},
		CalcCorrectGraphEntry{
			assert_floating_is<int>(row.at("stageMaxVal2")),
			row.at("stageMaxGrowVal2") / 100.,
			row.at("adjPt_maxGrowVal2")
		},
		CalcCorrectGraphEntry{
			assert_floating_is<int>(row.at("stageMaxVal3")),
			row.at("stageMaxGrowVal3") / 100.,
			row.at("adjPt_maxGrowVal3")
		},
		CalcCorrectGraphEntry{
			assert_floating_is<int>(row.at("stageMaxVal4")),
			row.at("stageMaxGrowVal4") / 100.,
			row.at("adjPt_maxGrowVal4")
		},
	};
}

json calculator::Parser::parse_attack_element_correct(const ParamRow& row) const
{
	auto buildAttackElementCorrect = [](std::vector<std::tuple<Attribute, bool, floating>> v) {
		json entries = json::object();
		for (auto&& elem : v)
		{
			auto&& [attribute, isCorrect, overwriteCorrect] = elem;
			if (isCorrect)
			{
				if (overwriteCorrect == -1)
					entries.emplace(calculator::attribute_json_string_table.from_enum(attribute), true);
				else
					entries.emplace(calculator::attribute_json_string_table.from_enum(attribute), overwriteCorrect / 100.);
			}
		}
		return entries;
		};


	return json{
		{std::to_string(AttackPowerType::PHYSICAL), buildAttackElementCorrect({
			{Attribute::STRENGTH, row.at("isStrengthCorrect_byPhysics"), row.at("overwriteStrengthCorrectRate_byPhysics")},
			{Attribute::DEXTERITY, row.at("isDexterityCorrect_byPhysics"), row.at("overwriteDexterityCorrectRate_byPhysics")},
			{Attribute::FAITH, row.at("isFaithCorrect_byPhysics"), row.at("overwriteFaithCorrectRate_byPhysics")},
			{Attribute::INTELLIGENCE, row.at("isMagicCorrect_byPhysics"), row.at("overwriteMagicCorrectRate_byPhysics")},
			{Attribute::ARCAINE, row.at("isLuckCorrect_byPhysics"), row.at("overwriteLuckCorrectRate_byPhysics")}}
		)},
		{std::to_string(AttackPowerType::MAGIC), buildAttackElementCorrect({
			{Attribute::STRENGTH, row.at("isStrengthCorrect_byMagic"), row.at("overwriteStrengthCorrectRate_byMagic")},
			{Attribute::DEXTERITY, row.at("isDexterityCorrect_byMagic"), row.at("overwriteDexterityCorrectRate_byMagic")},
			{Attribute::FAITH, row.at("isFaithCorrect_byMagic"), row.at("overwriteFaithCorrectRate_byMagic")},
			{Attribute::INTELLIGENCE, row.at("isMagicCorrect_byMagic"), row.at("overwriteMagicCorrectRate_byMagic")},
			{Attribute::ARCAINE, row.at("isLuckCorrect_byMagic"), row.at("overwriteLuckCorrectRate_byMagic")}}
		)},
		{std::to_string(AttackPowerType::FIRE), buildAttackElementCorrect({
			{Attribute::STRENGTH, row.at("isStrengthCorrect_byFire"), row.at("overwriteStrengthCorrectRate_byFire")},
			{Attribute::DEXTERITY, row.at("isDexterityCorrect_byFire"), row.at("overwriteDexterityCorrectRate_byFire")},
			{Attribute::FAITH, row.at("isFaithCorrect_byFire"), row.at("overwriteFaithCorrectRate_byFire")},
			{Attribute::INTELLIGENCE, row.at("isMagicCorrect_byFire"), row.at("overwriteMagicCorrectRate_byFire")},
			{Attribute::ARCAINE, row.at("isLuckCorrect_byFire"), row.at("overwriteLuckCorrectRate_byFire")}}
		)},
		{std::to_string(AttackPowerType::LIGHTNING), buildAttackElementCorrect({
			{Attribute::STRENGTH, row.at("isStrengthCorrect_byThunder"), row.at("overwriteStrengthCorrectRate_byThunder")},
			{Attribute::DEXTERITY, row.at("isDexterityCorrect_byThunder"), row.at("overwriteDexterityCorrectRate_byThunder")},
			{Attribute::FAITH, row.at("isFaithCorrect_byThunder"), row.at("overwriteFaithCorrectRate_byThunder")},
			{Attribute::INTELLIGENCE, row.at("isMagicCorrect_byThunder"), row.at("overwriteMagicCorrectRate_byThunder")},
			{Attribute::ARCAINE, row.at("isLuckCorrect_byThunder"), row.at("overwriteLuckCorrectRate_byThunder")}}
		)},
		{std::to_string(AttackPowerType::HOLY), buildAttackElementCorrect({
			{Attribute::STRENGTH, row.at("isStrengthCorrect_byDark"), row.at("overwriteStrengthCorrectRate_byDark")},
			{Attribute::DEXTERITY, row.at("isDexterityCorrect_byDark"), row.at("overwriteDexterityCorrectRate_byDark")},
			{Attribute::FAITH, row.at("isFaithCorrect_byDark"), row.at("overwriteFaithCorrectRate_byDark")},
			{Attribute::INTELLIGENCE, row.at("isMagicCorrect_byDark"), row.at("overwriteMagicCorrectRate_byDark")},
			{Attribute::ARCAINE, row.at("isLuckCorrect_byDark"), row.at("overwriteLuckCorrectRate_byDark")}}
		)}
	};
}

json calculator::Parser::parse_reinforce_param_weapon(const ParamRow& row) const
{
	auto cut_dec = [](floating f)->json { if (f == (long long)f) return (long long)f; return f;  };

	json ret = {
		{"attack", {
			{std::to_string(AttackPowerType::PHYSICAL), cut_dec(row.at("physicsAtkRate"))},
			{std::to_string(AttackPowerType::MAGIC), cut_dec(row.at("magicAtkRate"))},
			{std::to_string(AttackPowerType::FIRE), cut_dec(row.at("fireAtkRate"))},
			{std::to_string(AttackPowerType::LIGHTNING), cut_dec(row.at("thunderAtkRate"))},
			{std::to_string(AttackPowerType::HOLY), cut_dec(row.at("darkAtkRate"))}}
		},
		{"attributeScaling", {
			{calculator::attribute_json_string_table.from_enum(Attribute::STRENGTH), row.at("correctStrengthRate")},
			{calculator::attribute_json_string_table.from_enum(Attribute::DEXTERITY), row.at("correctAgilityRate")},
			{calculator::attribute_json_string_table.from_enum(Attribute::INTELLIGENCE), row.at("correctMagicRate")},
			{calculator::attribute_json_string_table.from_enum(Attribute::FAITH), row.at("correctFaithRate")},
			{calculator::attribute_json_string_table.from_enum(Attribute::ARCAINE), row.at("correctLuckRate")}}
		}
	};

	if (row.contains("spEffectId1"))
		if (row.at("spEffectId1") != 0)
			ret["statusSpEffectId1"] = row.at("spEffectId1");
	if (row.contains("spEffectId2"))
		if (row.at("spEffectId2") != 0)
			ret["statusSpEffectId2"] = row.at("spEffectId2");
	if (row.contains("spEffectId3"))
		if (row.at("spEffectId3") != 0)
			ret["statusSpEffectId3"] = row.at("spEffectId3");

	return ret;
}

json calculator::Parser::parse_weapon(const ParamRow& row) const
	{
		auto row_id = assert_floating_is<long long>(row.at("id"));

		std::string name{};
		bool dlc{};
		if (this->weaponNames.contains(row_id))
			name = this->weaponNames.at(row_id);
		else if (this->dlcWeaponNames.contains(row_id))
		{
			name = this->dlcWeaponNames.at(row_id);
			dlc = isVanilla;
		}
		else
		{
			misc::printl("ignoring: could not find weapon name for id: ", row_id);
			return {};
		}

		if (name.find("[ERROR]") != std::string::npos || name.find("%null%") != std::string::npos)
		{
			misc::printl("ignoring: weapon name: ", name, ", id: ", row_id);
			return {};
		}

		const auto weaponType = wepTypeOverrides.contains(row_id) ? wepTypeOverrides.at(row_id) : assert_floating_is<long long>(row.at("wepType"));
		if (!is_supported_weapon_type(weaponType))
		{
			if (std::set{ 0, 81, 83, 85, 86 }.contains(weaponType))
				misc::printl("ignoring: weapon ", name, " because no real weapon");
			else
			{
				misc::printl("ignoring: Unknown weapon type ", weaponType, " for weapon ", name);
				throw std::runtime_error("unknown weapon type");
			}
			return {};
		}

		if (!this->reinforceParamWeapons.contains(row.at("reinforceTypeId")))
			throw std::runtime_error("could not find reinforce param weapon for reinforceTypeId: " + std::to_string(row.at("reinforceTypeId")));

		if (!this->attackElementCorrectParams.contains(row.at("attackElementCorrectId")))
			throw std::runtime_error("could not find attack element correct param for attackElementCorrectId: " + std::to_string(row.at("attackElementCorrectId")));

		const auto affinityId = assert_floating_is<long long>((row_id % 10000) / 100.);

		const auto equipParamWeaponsId = row_id - 100 * affinityId;
		if (!this->equipParamWeapons.contains(equipParamWeaponsId))
			throw std::runtime_error("could not find equip param weapon for id: " + std::to_string(equipParamWeaponsId));
		const auto& uninfusedWeapon = this->equipParamWeapons.at(equipParamWeaponsId);

		if (affinityId != 0 && is_unique_weapon(uninfusedWeapon))
			throw std::runtime_error("unique weapon cannot have an affinity");

		std::set<calculator::AttackPowerType> attackPowerTypes{};
		std::vector<long long> statusSpEffectParamIds{};
		for (auto&& spEffectParamId : std::vector<long long>{
			assert_floating_is<long long>(row.at("spEffectBehaviorId0")),
			assert_floating_is<long long>(row.at("spEffectBehaviorId1")),
			assert_floating_is<long long>(row.at("spEffectBehaviorId2")),
			})
		{
			auto statusSpEffectParams = parse_status_sp_effect_params(spEffectParamId);
			if (!statusSpEffectParams.empty())
			{
				for (auto&& [k, v] : statusSpEffectParams)
					attackPowerTypes.insert(k);

				statusSpEffectParamIds.emplace_back(spEffectParamId);
			}
			else
				statusSpEffectParamIds.emplace_back(0);
		}

		if (std::ranges::all_of(statusSpEffectParamIds, [](long long id) { return id == 0; }))
			statusSpEffectParamIds.clear();
		
		if (isVanilla && row_id == 32131200) 
			statusSpEffectParamIds = { 0, 0, 0 };
		
		std::vector<std::pair<AttackPowerType, floating>> attack{};
		for (auto&& pair : std::vector<std::pair<AttackPowerType, floating>>{
			{ AttackPowerType::PHYSICAL, row.at("attackBasePhysics") },
			{ AttackPowerType::MAGIC, row.at("attackBaseMagic") },
			{ AttackPowerType::FIRE, row.at("attackBaseFire") },
			{ AttackPowerType::LIGHTNING, row.at("attackBaseThunder") },
			{ AttackPowerType::HOLY, row.at("attackBaseDark") }
			})
		{
			auto&& [attackPowerType, attackPower] = pair;

			if (attackPower != 0)
			{
				attackPowerTypes.insert(attackPowerType);
				attack.emplace_back(pair);
			}
		}

		if (row.at("enableMagic") || row.at("enableMiracle"))
			for (auto&& damageType : DamageType::_values())
				attackPowerTypes.insert(AttackPowerType::_from_integral(damageType._to_integral()));
		

		std::map<AttackPowerType, long long> calcCorrectGraphIds{};
		if (attackPowerTypes.contains(AttackPowerType::PHYSICAL))
			if (row.contains("correctType_Physics"))
				if (row.at("correctType_Physics") != defaultDamageCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::PHYSICAL] = assert_floating_is<long long>(row.at("correctType_Physics"));
		if (attackPowerTypes.contains(AttackPowerType::MAGIC))
			if (row.contains("correctType_Magic"))
				if (row.at("correctType_Magic") != defaultDamageCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::MAGIC] = assert_floating_is<long long>(row.at("correctType_Magic"));
		if (attackPowerTypes.contains(AttackPowerType::FIRE))
			if (row.contains("correctType_Fire"))
				if (row.at("correctType_Fire") != defaultDamageCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::FIRE] = assert_floating_is<long long>(row.at("correctType_Fire"));
		if (attackPowerTypes.contains(AttackPowerType::LIGHTNING))
			if (row.contains("correctType_Thunder"))
				if (row.at("correctType_Thunder") != defaultDamageCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::LIGHTNING] = assert_floating_is<long long>(row.at("correctType_Thunder"));
		if (attackPowerTypes.contains(AttackPowerType::HOLY))
			if (row.contains("correctType_Dark"))
				if (row.at("correctType_Dark") != defaultDamageCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::HOLY] = assert_floating_is<long long>(row.at("correctType_Dark"));

		if (attackPowerTypes.contains(AttackPowerType::POISON))
			if (row.contains("correctType_Poison"))
				if (row.at("correctType_Poison") != defaultStatusCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::POISON] = assert_floating_is<long long>(row.at("correctType_Poison"));
		if (attackPowerTypes.contains(AttackPowerType::BLEED))
			if (row.contains("correctType_Bleed"))
				if (row.at("correctType_Bleed") != defaultStatusCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::BLEED] = assert_floating_is<long long>(row.at("correctType_Bleed"));
		if (attackPowerTypes.contains(AttackPowerType::SLEEP))
			if (row.contains("correctType_Sleep"))
				if (row.at("correctType_Sleep") != defaultStatusCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::SLEEP] = assert_floating_is<long long>(row.at("correctType_Sleep"));
		if (attackPowerTypes.contains(AttackPowerType::MADNESS))
			if (row.contains("correctType_Madness"))
				if (row.at("correctType_Madness") != defaultStatusCalcCorrectGraphId)
					calcCorrectGraphIds[AttackPowerType::MADNESS] = assert_floating_is<long long>(row.at("correctType_Madness"));

		for (auto&& [apt, calcCorrectGraphId] : calcCorrectGraphIds)
		{
			if (DamageType::_from_integral_nothrow(apt))
				if (calcCorrectGraphId != defaultDamageCalcCorrectGraphId && !this->calcCorrectGraphs.contains(calcCorrectGraphId))
					throw std::runtime_error("could not find calc correct graph for id: " + std::to_string(calcCorrectGraphId));
			if (StatusType::_from_integral_nothrow(apt))
				if (calcCorrectGraphId != defaultStatusCalcCorrectGraphId && !this->calcCorrectGraphs.contains(calcCorrectGraphId))
					throw std::runtime_error("could not find calc correct graph for id: " + std::to_string(calcCorrectGraphId));
		}

		std::vector<std::pair<Attribute, floating>> attributeScaling{};
		if (row.at("correctStrength"))
			attributeScaling.emplace_back(Attribute::STRENGTH, row.at("correctStrength") / 100.);
		if (row.at("correctAgility"))
			attributeScaling.emplace_back(Attribute::DEXTERITY, row.at("correctAgility") / 100.);
		if (row.at("correctMagic"))
			attributeScaling.emplace_back(Attribute::INTELLIGENCE, row.at("correctMagic") / 100.);
		if (row.at("correctFaith"))
			attributeScaling.emplace_back(Attribute::FAITH, row.at("correctFaith") / 100.);
		if (row.at("correctLuck"))
			attributeScaling.emplace_back(Attribute::ARCAINE, row.at("correctLuck") / 100.);

		json ret{};
		ret["name"] = name;
		ret["weaponName"] = (weaponNames.contains(uninfusedWeapon.at("id")) ? weaponNames.at(uninfusedWeapon.at("id")) : dlcWeaponNames.at(uninfusedWeapon.at("id")));
		//ret["url"] = "";
		ret["affinityId"] = is_unique_weapon(row) ? -1 : affinityId;
		ret["weaponType"] = weaponType;
		ret["requirements"] = Stats{
			assert_floating_is<int>(row.at("properStrength")),
			assert_floating_is<int>(row.at("properAgility")),
			assert_floating_is<int>(row.at("properMagic")),
			assert_floating_is<int>(row.at("properFaith")),
			assert_floating_is<int>(row.at("properLuck")),
		};
		ret["attack"] = attack;
		ret["attributeScaling"] = attributeScaling;
		if (!statusSpEffectParamIds.empty())
			ret["statusSpEffectParamIds"] = statusSpEffectParamIds;
		ret["reinforceTypeId"] = assert_floating_is<long long>(row.at("reinforceTypeId"));
		ret["attackElementCorrectId"] = assert_floating_is<long long>(row.at("attackElementCorrectId"));
		ret["calcCorrectGraphIds"] = calcCorrectGraphIds;
		if (row.at("isDualBlade") == 1)
			ret["paired"] = true;
		if (row.at("enableMagic") == 1)
			ret["sorceryTool"] = true;
		if (row.at("enableMiracle") == 1)
			ret["incantationTool"] = true;
		ret["dlc"] = dlc;


		misc::printl("weapon: ", name, ", type: ", Weapon::Type::_from_integral(weaponType)._to_string());
		return ret;
	}

calculator::Parser::Parser(const std::filesystem::path& witchy_exe_path, const std::filesystem::path& uxm_target_directory)
{
	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files
	auto copy_files_to_path = std::filesystem::current_path() / "elden_ring_files";

	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation.bin
	auto files_to_unpack = this->copy_elden_ring_files(uxm_target_directory, copy_files_to_path);

	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin
	auto unpacked_directories = this->witchy_unpack_files(witchy_exe_path, files_to_unpack);

	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin\AttackElementCorrectParam.param
	auto needed_unpacked_file_paths = this->get_needed_unpacked_files(unpacked_directories);

	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\regulation-bin\AttackElementCorrectParam.param.xml
	auto xml_files = this->witchy_to_xml(witchy_exe_path, needed_unpacked_file_paths);

	// D:\Paul\Computer\Programmieren\C++\Haupt-Projektmappe\ConsoleApplication\elden_ring_files\xml_files
	auto xml_directory = this->copy_xml_files(copy_files_to_path, xml_files);


	this->attackElementCorrectParams = read_param_xml(xml_directory / attackElementCorrectFile);
	this->calcCorrectGraphs = read_param_xml(xml_directory / calcCorrectGraphFile);
	this->equipParamWeapons = read_param_xml(xml_directory / equipParamWeaponFile);
	this->reinforceParamWeapons = read_param_xml(xml_directory / reinforceParamWeaponFile);
	this->spEffectParams = read_param_xml(xml_directory / spEffectFile);
	this->menuValueTableParams = read_param_xml(xml_directory / menuValueTableFile);
	this->menuText = read_fmg_xml(xml_directory / menuTextFmgFile);
	this->weaponNames = read_fmg_xml(xml_directory / weaponNameFmgFile);
	this->dlcWeaponNames = read_fmg_xml(xml_directory / dlcWeaponNameFmgFile);
}

json calculator::Parser::get_regulation_data_json()
{
	json weapons_json = json::array();
	for (auto&& [k, param_row] : this->equipParamWeapons)
	{
		auto weapon_json = this->parse_weapon(param_row);
		if(!weapon_json.empty())
			weapons_json.push_back(weapon_json);
	}

	std::set<long long> calc_correct_graph_ids{ defaultDamageCalcCorrectGraphId, defaultStatusCalcCorrectGraphId };
	for (auto&& weapon_json : weapons_json)
		for (auto&& [apt, calcCorrectGraphId] : weapon_json.at("calcCorrectGraphIds").items())
			calc_correct_graph_ids.insert(calcCorrectGraphId.get<long long>());
	json calc_correct_graphs_json{};
	for (auto&& [id, calc_correct_graph] : this->calcCorrectGraphs)
		if (calc_correct_graph_ids.contains(id))
			calc_correct_graphs_json[std::to_string(id)] = parse_calc_correct_graph(calc_correct_graph);

	std::set<long long> attackElementCorrectIds{};
	for (auto&& weapon_json : weapons_json)
		attackElementCorrectIds.insert(weapon_json.at("attackElementCorrectId").get<long long>());
	json attack_element_corrects_json{};
	for (auto&& [id, row] : this->attackElementCorrectParams)
		if (attackElementCorrectIds.contains(id))
			attack_element_corrects_json[std::to_string(id)] = parse_attack_element_correct(row);

	std::set<long long> reinforceTypeIds{};
	for (auto&& weapon_json : weapons_json)
		reinforceTypeIds.insert(weapon_json.at("reinforceTypeId").get<long long>());
	json reinforce_types_json{};
	for (auto&& [reinforceParamId, reinforceParamWeapon] : this->reinforceParamWeapons)
	{
		auto reinforceLevel = reinforceParamId % 50;
		auto reinforceTypeId = reinforceParamId - reinforceLevel;
		auto reinforceTypeId_string = std::to_string(reinforceTypeId);

		if (reinforceTypeIds.contains(reinforceTypeId))
		{
			if (!reinforce_types_json.contains(reinforceTypeId_string))
				reinforce_types_json[reinforceTypeId_string] = json::array();

			auto&& reinforceTypeJson = reinforce_types_json[reinforceTypeId_string];
			if (reinforceTypeJson.size() == reinforceLevel)
				reinforceTypeJson.push_back(parse_reinforce_param_weapon(reinforceParamWeapon));
		}
	}

	std::set<long long> statusSpEffectParamIds{};
	for (auto&& weapon_json : weapons_json)
	{
		auto reinforceTypeId_string = std::to_string(weapon_json.at("reinforceTypeId").get<long long>());
		auto&& reinforceParamWeapons = reinforce_types_json.at(reinforceTypeId_string);
		for (auto&& reinforceParamWeapon : reinforceParamWeapons)
		{
			if (weapon_json.contains("statusSpEffectParamIds"))
			{
				int i = 1;
				for (auto&& spEffectParamId : weapon_json.at("statusSpEffectParamIds").get<std::array<long long, 3>>())
				{
					if (spEffectParamId)
					{
						auto statusSpEffectId_string = "statusSpEffectId" + std::to_string(i);
						auto offset = reinforceParamWeapon.value(statusSpEffectId_string, 0ll);
						statusSpEffectParamIds.insert(spEffectParamId + offset);
					}
					++i;
				}
			}
		}
	}
	json status_sp_effect_params_json{};
	for (auto&& [spEffectParamId, _] : this->spEffectParams)
	{
		if (statusSpEffectParamIds.contains(spEffectParamId))
		{
			auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId);
			std::erase_if(status_sp_effect_params, [](auto&& v) { return v.second == 0; });
			status_sp_effect_params_json[std::to_string(spEffectParamId)] = status_sp_effect_params;
		}
	}

	json scaling_tiers_tson = json::array();
	for (auto&& [id, row] : this->menuValueTableParams)
		if (row.at("compareType") == 1 && id >= 100)
			scaling_tiers_tson.push_back(json::array({ row.at("value") / 100., this->menuText.at(row.at("textId")) }));

	json regulation_data_json{
		{ "calcCorrectGraphs", calc_correct_graphs_json },
		{ "attackElementCorrects", attack_element_corrects_json },
		{ "reinforceTypes", reinforce_types_json },
		{ "statusSpEffectParams", status_sp_effect_params_json },
		{ "scalingTiers", scaling_tiers_tson },
		{ "weapons", weapons_json }
	};

	misc::printl("\nsuccessfully generated regulation data file");

	return regulation_data_json;
}

void calculator::test2()
{
	misc::print(std::fixed);
	std::cout.precision(20);

	// Madding Hand & Poisoned Hand differ slightly

	Parser parser(std::filesystem::path("C:/Users/Paul/Downloads/WitchyBND-v2.14.0.3/WitchyBND.exe", std::filesystem::path::format::native_format),
		std::filesystem::path("D:/Programme/Steam/steamapps/common/ELDEN RING/Game", std::filesystem::path::format::native_format));
	auto regulation_data_json = parser.get_regulation_data_json();

	auto weapons_json = regulation_data_json["weapons"];
	auto calc_correct_graphs_json = regulation_data_json["calcCorrectGraphs"];
	auto attack_element_corrects_json = regulation_data_json["attackElementCorrects"];
	auto reinforce_types_json = regulation_data_json["reinforceTypes"];
	auto status_sp_effect_params_json = regulation_data_json["statusSpEffectParams"];
	auto scaling_tiers_tson = regulation_data_json["scalingTiers"];
	
	auto regulation_data = json::parse(std::ifstream("D:/Paul/Computer/Programmieren/C++/elden-ring-damage-optimizer/damage-optimizer/regulation-vanilla-v1.12.3.js"));
	auto weaponJson = regulation_data["weapons"];
	auto calcCorrectGraphs = regulation_data["calcCorrectGraphs"];
	auto attackElementCorrects = regulation_data["attackElementCorrects"];
	auto reinforceTypes = regulation_data["reinforceTypes"];
	auto statusSpEffectParams = regulation_data["statusSpEffectParams"];
	auto scalingTiers = regulation_data["scalingTiers"];

	misc::printl("my weaponJson: ", weapons_json.size());
	misc::printl("weaponJson: ", weaponJson.size());

	misc::printl("my calcCorrectGraphs: ", calc_correct_graphs_json.size());
	misc::printl("calcCorrectGraphs: ", calcCorrectGraphs.size());

	misc::printl("my attackElementCorrects: ", attack_element_corrects_json.size());
	misc::printl("attackElementCorrects: ", attackElementCorrects.size());
	misc::printl();

	misc::printl("my reinforceTypes: ", reinforce_types_json.size());
	misc::printl("reinforceTypes: ", reinforceTypes.size());
	misc::printl();

	misc::printl("my statusSpEffectParams: ", status_sp_effect_params_json.size());
	misc::printl("statusSpEffectParams: ", statusSpEffectParams.size());
	misc::printl();

	misc::printl("my scalingTiers: ", scaling_tiers_tson.size());
	misc::printl("scalingTiers: ", scalingTiers.size());
	misc::printl();

	if (weapons_json != weaponJson)
	{
		misc::printl("weapons");
		for (auto&& [myw, w] : std::views::zip(weapons_json, weaponJson))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}

	if (calc_correct_graphs_json != calcCorrectGraphs)
	{
		misc::printl("calcCorrectGraphs");
		for (auto&& [myw, w] : std::views::zip(calc_correct_graphs_json, calcCorrectGraphs))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}

	if (attack_element_corrects_json != attackElementCorrects)
	{
		misc::printl("attackElementCorrects");
		for (auto&& [myw, w] : std::views::zip(attack_element_corrects_json, attackElementCorrects))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}

	if (reinforce_types_json != reinforceTypes)
	{
		misc::printl("reinforceTypes");
		for (auto&& [myw, w] : std::views::zip(reinforce_types_json, reinforceTypes))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}

	if (status_sp_effect_params_json != statusSpEffectParams)
	{
		misc::printl("statusSpEffectParams");
		for (auto&& [myw, w] : std::views::zip(status_sp_effect_params_json, statusSpEffectParams))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}

	if (scaling_tiers_tson != scalingTiers)
	{
		misc::printl("scalingTiers");
		for (auto&& [myw, w] : std::views::zip(scaling_tiers_tson, scalingTiers))
		{
			if (myw != w)
			{
				misc::printl("my: ", myw);
				misc::printl("other: ", w);
			}
		}
		misc::printl();
	}



	misc::printl("\n\ndone");
	Sleep(1000*1000);
}