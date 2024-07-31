
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

void calculator::weapon::attack_rating::reset()
{
	this->total_attack_power = 0.;
	this->attack_power.fill({});
	this->spell_scaling.fill({});
	this->ineffective_attributes.clear();
	this->ineffective_attributes.reserve(Attribute::_size());
	this->ineffective_attack_power_types.clear();
	this->ineffective_attack_power_types.reserve(AttackPowerType::_size());
}

bool calculator::weapon::filter::operator()(const weapon& weapon) const
{
	auto satisfies = [](const auto& set, const auto& val) { return set.empty() or set.contains(val); };

	return satisfies(this->base_names, weapon.base_name) and
		satisfies(this->dlc, weapon.dlc) and
		//satisfies(this->sorcery_tools, weapon.sorcery_tool) and
		//satisfies(this->incantation_tools, weapon.incantation_tool) and
		satisfies(this->types, weapon.type) and
		satisfies(this->affinities, weapon.affinity);
}

calculator::Stats calculator::weapon::adjust_stats_for_two_handing(bool two_handing, Stats stats) const
{
	// Paired weapons do not get the two handing bonus
	if (this->paired)
		two_handing = false;

	// Bows and ballistae can only be two handed
	constexpr std::array<weapon::Type, 4> bow_types = { weapon::Type::LIGHT_BOW, weapon::Type::BOW, weapon::Type::GREATBOW, weapon::Type::BALLISTA };
	if (std::ranges::contains(bow_types, this->type))
		two_handing = true;

	if (two_handing)
		stats.at(Attribute::STRENGTH) = stats.at(Attribute::STRENGTH) * 1.5;

	return stats;
}

void calculator::weapon::get_attack_rating(const attack_options& attack_options, const Stats& stats, attack_rating& result) const
{
	auto adjusted_stats = this->adjust_stats_for_two_handing(attack_options.two_handing, stats);

	result.weapon = this;
	result.stats = stats;
	result.upgrade_level = attack_options.upgrade_level;

	for (auto attribute : Attribute::_values())
		if (adjusted_stats[attribute._to_integral()] < this->requirements[attribute._to_integral()])
			result.ineffective_attributes.push_back(attribute);

	size_t upgrade_level;
	if (this->base_attack_power.size() == 1)
		upgrade_level = 0;
	else if (this->base_attack_power.size() == 11)
		upgrade_level = attack_options.upgrade_level.at(1);
	else if (this->base_attack_power.size() == 26)
		upgrade_level = attack_options.upgrade_level.at(0);
	else
		throw std::runtime_error("invalid base attack power size");

	for (auto&& attack_power_type : AttackPowerType::_values())
	{
		auto base_attack_power = this->base_attack_power[upgrade_level][attack_power_type._to_integral()];

		if (base_attack_power != 0 or this->sorcery_tool or this->incantation_tool)
		{
			auto is_damage_type = attack_power_type._to_integral() <= AttackPowerType::HOLY;
			auto&& scaling_attributes = this->attack_power_attribute_scaling.at(attack_power_type._to_integral());
			floating total_scaling = 1.;

			if (std::any_of(result.ineffective_attributes.begin(), result.ineffective_attributes.end(),
				[&](Attribute ineffective_attribute) { return scaling_attributes[ineffective_attribute._to_integral()] != 0; }))
			{
				total_scaling = 1. - ineffective_attribute_penalty;
				result.ineffective_attack_power_types.push_back(attack_power_type);
			}
			else
			{
				auto& effective_stats = (!attack_options.disable_two_handing_attack_power_bonus && is_damage_type) ? adjusted_stats : stats;

				for (auto&& attribute : Attribute::_values())
				{
					auto&& attribute_correct = scaling_attributes.at(attribute._to_integral());
					floating scaling{};

					if (attribute_correct != 0)
					{
						if (attribute_correct == 1)
							scaling = this->attribute_scaling.at(upgrade_level).at(attribute._to_integral());
						else
							scaling = attribute_correct * this->attribute_scaling.at(upgrade_level).at(attribute._to_integral()) / this->attribute_scaling.at(0).at(attribute._to_integral());

						if (scaling != 0.)
							total_scaling += this->attack_power_scaling_curves[attack_power_type._to_integral()]->operator[](effective_stats[attribute._to_integral()]) * scaling;
					}
				}
			}

			if (base_attack_power != 0)
			{
				auto res = base_attack_power * total_scaling;
				auto&& att_pwr = result.attack_power.at(attack_power_type._to_integral());
				att_pwr.first = res;
				att_pwr.second.at(0) = base_attack_power;
				att_pwr.second.at(1) = res - base_attack_power;
				result.total_attack_power += res;
			}

			if (is_damage_type and (this->sorcery_tool or this->incantation_tool))
				result.spell_scaling[attack_power_type._to_integral()] = 100. * total_scaling;
		}
	}
}

void calculator::from_json(const json& j, CalcCorrectGraphDict& c)
{
	j.at("maxVal").get_to(c.maxVal);
	j.at("maxGrowVal").get_to(c.maxGrowVal);
	j.at("adjPt").get_to(c.adjPt);
}

void calculator::from_json(const json& j, ReinforceTypesDict& r)
{
	auto&& attack_json = j.at("attack");
	for (auto dmg_type : DamageTypes::_values())
		r.attack.at(dmg_type._to_integral()) = attack_json.value(std::to_string(dmg_type._to_integral()), floating(0));

	r.attributeScaling = j.at("attributeScaling").get<AttributeScaling>();

	r.statusSpEffectId.at(0) = j.value("statusSpEffectId1", 0);
	r.statusSpEffectId.at(1) = j.value("statusSpEffectId2", 0);
	r.statusSpEffectId.at(2) = j.value("statusSpEffectId3", 0);
}

calculator::ScalingCurve calculator::weapon_container::evaluate_CalcCorrectGraph(const std::vector<CalcCorrectGraphDict>& calcCorrectGraph)
{
	ScalingCurve arr{};

	for (size_t i = 1; i < calcCorrectGraph.size(); i++)
	{
		const CalcCorrectGraphDict& prevStage = calcCorrectGraph.at(i - 1);
		const CalcCorrectGraphDict& stage = calcCorrectGraph.at(i);

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

calculator::weapon_container::weapon_container(const std::filesystem::path& file_path)
{
	json data = json::parse(std::ifstream(file_path));

	for (auto&& [id_, calcCorrectGraph] : data.at("calcCorrectGraphs").items())
		this->calcCorrectGraphsById.emplace(std::stoi(id_), evaluate_CalcCorrectGraph(calcCorrectGraph.get<std::vector<CalcCorrectGraphDict>>()));

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
		for (auto damage_type : DamageTypes::_values())
			weaponCalcCorrectGraphs.at(damage_type._to_integral()) = &this->calcCorrectGraphsById.at(misc::map_get(calcCorrectGraphIds, AttackPowerType::_from_integral_unchecked(damage_type), defaultDamageCalcCorrectGraphId));
		for (auto status_type : StatusTypes::_values())
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

		return weapon{
			weapon_data.at("name").get<std::string>(),
			std::move(weaponName),
			weapon_data.value("url", "https://eldenring.fandom.com/wiki/" + url_part),
			weapon_data.at("dlc").get<bool>(),
			weapon_data.value("paired", false),
			weapon_data.value("sorceryTool", false),
			weapon_data.value("incantationTool", false),
			weapon_data.at("weaponType").get<weapon::Type>(),
			weapon_data.at("affinityId").get<weapon::Affinity>(),
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

calculator::weapon::all_filter_options calculator::weapon_container::get_all_filter_options() const
{
	weapon::filter filter{};
	for (const auto& weapon : this->weapons)
	{
		filter.dlc.insert(weapon.dlc);
		//filter.sorcery_tools.insert(weapon.sorcery_tool);
		//filter.incantation_tools.insert(weapon.incantation_tool);
		filter.types.insert(weapon.type);
		filter.affinities.insert(weapon.affinity);
		filter.base_names.insert(weapon.base_name);
	}

	weapon::all_filter_options all_filter_options{ 
		{ filter.dlc.begin(), filter.dlc.end() },
		//{ filter.sorcery_tools.begin(), filter.sorcery_tools.end() },
		//{ filter.incantation_tools.begin(), filter.incantation_tools.end() },
		{ filter.types.begin(), filter.types.end() },
		{ filter.affinities.begin(), filter.affinities.end() },
		{ filter.base_names.begin(), filter.base_names.end() },
	};

	return all_filter_options;
}

void calculator::weapon_container::apply_filter(const weapon::filter& weapon_filter, filtered_weapons& filtered) const
{
	for (const auto& weapon : this->weapons)
		if (weapon_filter(weapon))
			filtered.weapons.push_back(&weapon);
}

void calculator::test()
{
	auto&& [weap_contain, weap_contain_time] = misc::TimeFunctionExecution([&]() { return weapon_container("D:\\Paul\\Computer\\Programmieren\\C++\\elden-ring-damage-optimizer\\regulation-vanilla-v1.12.3.js"); });

	weapon::attack_options atk_options = { 161 + 79, ALL_CLASS_STATS.at(Class::VAGABOND), {24, 10}, true, false };
	auto [stat_variations, stat_variations_time] = misc::TimeFunctionExecution([&]() { return get_stat_variations(atk_options.available_stat_points, atk_options.minimum_stats); });

	auto [filtered_weaps, filtered_weapons_time] = misc::TimeFunctionExecution([&]() { filtered_weapons filtered{}; weap_contain.apply_filter(weapon::filter{ { true } }, filtered); return filtered; });
	misc::printl();

	auto [attack_rating, attack_rating_time] = misc::TimeFunctionExecution([&]()
		{ return filtered_weaps.optimize_attack_rating(stat_variations, [](const weapon::attack_rating& war) { return war.total_attack_power; }, atk_options).wait_and_get_result(); });
	misc::printl();

	misc::print("stats: ");
	for (auto stat : attack_rating.stats)
		misc::print(stat, " ");
	misc::printl("\n");

	misc::printl(attack_rating.weapon->full_name, ": ", attack_rating.total_attack_power);
	misc::printl();
	for (int i = 0; i < attack_rating.attack_power.size(); i++)
		if (attack_rating.attack_power.at(i).first != 0)
			misc::printl(AttackPowerType::_from_integral(i)._to_string(), ": ", attack_rating.attack_power.at(i).second.at(0), " + ", attack_rating.attack_power.at(i).second.at(1),
				" = ", attack_rating.attack_power.at(i).first);
	misc::printl();


	misc::printl("load weapon container: ", weap_contain_time);
	misc::printl("get stat variations: ", stat_variations_time);
	misc::printl("filter weapons: ", filtered_weapons_time);
	misc::printl("query best stats: ", attack_rating_time);
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

calculator::Attribute nlohmann::adl_serializer<calculator::Attribute>::from_json(const json& j)
{
	return calculator::attribute_json_string_table.to_enum(j.get<std::string>().c_str());
}

calculator::AttackPowerType nlohmann::adl_serializer<calculator::AttackPowerType>::from_json(const json& j)
{
	return calculator::AttackPowerType::_from_integral(j.get<int>());
}

calculator::weapon::Affinity nlohmann::adl_serializer<calculator::weapon::Affinity>::from_json(const json& j)
{
	return calculator::weapon::Affinity::_from_integral(j.get<int>());
}

calculator::weapon::Type nlohmann::adl_serializer<calculator::weapon::Type>::from_json(const json& j)
{
	return calculator::weapon::Type::_from_integral(j.get<int>());
}
