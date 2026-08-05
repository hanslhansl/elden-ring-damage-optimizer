export module erdo:parser;
import :xml;
import :witchy;
import :calculator;

import std;


long long assert_float_is_llong(double f) {
    if (f != (long long)f)
        throw std::runtime_error("float is not long long");
    return f;
}

template <typename Map, typename Key, typename Default>
auto map_get(Map &&m, Key &&key, Default &&default_) {
    using result_type = std::common_reference_t<typename std::remove_cvref_t<Map>::mapped_type, Default &&>;

    auto it = m.find(std::forward<Key>(key));
    if (it == m.end())
        return result_type(std::forward<Default>(default_));
    return result_type(it->second);
}

namespace erdo::parser
{
    using ParamRow = std::map<std::string, double>;
    struct CalcCorrectGraphEntry
    {
        long long maxVal;
        double maxGrowVal, adjPt;
    };
    using CalcCorrectGraph = std::array<CalcCorrectGraphEntry, 5>;
    struct ReinforceTypesDict
    {
        calculator::AttributeScaling attack;             // index: DamageType
        calculator::AttributeScaling attributeScaling;   // index: RelevantAttribute
        std::array<int, 3> statusSpEffectId;            // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3
    };


    const std::map<long long, calculator::Weapon::Type> weapon_type_overrides = {{110000, calculator::Weapon::Type::FIST}};
    constexpr bool isVanilla = true;
    constexpr long long default_damage_calc_correct_graph_id = 0;
    constexpr long long default_status_calc_correct_graph_id = 6;


    std::string attribute_to_xml_string(calculator::RelevantAttribute attr) {
        if (attr == calculator::RelevantAttribute::STRENGTH)
            return "Strength";
        if (attr == calculator::RelevantAttribute::DEXTERITY)
            return "Agility";
        if (attr == calculator::RelevantAttribute::INTELLIGENCE)
            return "Magic";
        if (attr == calculator::RelevantAttribute::FAITH)
            return "Faith";
        if (attr == calculator::RelevantAttribute::ARCAINE)
            return "Luck";

        throw std::invalid_argument("unknown attribute");
    }
    std::string attack_power_type_to_xml_string(calculator::AttackPowerType apt) {
        if (apt == calculator::AttackPowerType::PHYSICAL)
            return "Physics";
        if (apt == calculator::AttackPowerType::MAGIC)
            return "Magic";
        if (apt == calculator::AttackPowerType::FIRE)
            return "Fire";
        if (apt == calculator::AttackPowerType::LIGHTNING)
            return "Thunder";
        if (apt == calculator::AttackPowerType::HOLY)
            return "Dark";

        if (apt == calculator::AttackPowerType::POISON)
            return "Poison";
        if (apt == calculator::AttackPowerType::BLEED)
            return "Bleed";
        if (apt == calculator::AttackPowerType::SLEEP)
            return "Sleep";
        if (apt == calculator::AttackPowerType::MADNESS)
            return "Madness";

        throw std::invalid_argument("unknown attack power type");
    }
    
    std::map<calculator::AttackPowerType, long long> parse_status_sp_effect_params(
        long long statusSpEffectParamId,
        const std::map<long long, std::map<std::string, long long>>& spEffectParams) {
        if (!spEffectParams.contains(statusSpEffectParamId))
            return {};
        auto &&spEffectRow = spEffectParams.at(statusSpEffectParamId);

        std::map<calculator::AttackPowerType, long long> statuses =  {
            {calculator::AttackPowerType::POISON, spEffectRow.at("poizonAttackPower")},
            {calculator::AttackPowerType::SCARLET_ROT, spEffectRow.at("diseaseAttackPower")},
            {calculator::AttackPowerType::BLEED, spEffectRow.at("bloodAttackPower")},
            {calculator::AttackPowerType::FROST, spEffectRow.at("freezeAttackPower")},
            {calculator::AttackPowerType::SLEEP, spEffectRow.at("sleepAttackPower")},
            {calculator::AttackPowerType::MADNESS, spEffectRow.at("madnessAttackPower")},
            {calculator::AttackPowerType::DEATH_BLIGHT, spEffectRow.at("curseAttackPower")}
        };

        if (std::ranges::any_of(statuses, [](auto &&v) { return v.second != 0; }))
            return statuses;

        return {};
    }
    CalcCorrectGraph parse_calc_correct_graph(const std::map<std::string, double> &row) {
        CalcCorrectGraph ret{};
        for (auto i = 0; i < 5; ++i)
        {
            auto maxVal = assert_float_is_llong(row.at(std::format("stageMaxVal{}", i)));
            auto maxGrowVal = row.at(std::format("stageMaxGrowVal{}", i)) / 100.;
            auto adjPt = row.at(std::format("adjPt_maxGrowVal{}", i));
            ret.at(i) = CalcCorrectGraphEntry{maxVal, maxGrowVal, adjPt};
        }
        return ret;
    }
    calculator::AttackElementCorrects parse_attack_element_correct(const ParamRow &row) {
        calculator::AttackElementCorrects ret{};
        for (auto damage_type : enumerators_of<calculator::DamageType>())
        {
            auto apt = integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type));
            auto apt_str = attack_power_type_to_xml_string(apt);

            auto&& attribute_scaling = ret.at(std::to_underlying(apt));

            for (auto attribute : enumerators_of<calculator::RelevantAttribute>())
            {
                auto attribute_str = attribute_to_xml_string(attribute);
                if (attribute_str == "Agility")
                    attribute_str = "Dexterity";
                bool is_correct = row.at(std::format("is{}Correct_by{}", attribute_str, apt_str));
                auto overwrite_correct = row.at(std::format("overwrite{}CorrectRate_by{}", attribute_str, apt_str));

                if (is_correct)
                {
                    if (overwrite_correct == -1)
                        attribute_scaling.at(std::to_underlying(attribute)) = true;
                    else
                        attribute_scaling.at(std::to_underlying(attribute)) = overwrite_correct / 100.;
                }
            }
        }
        return ret;
    }
    ReinforceTypesDict parse_reinforce_param_weapon(const ParamRow &row) {
        ReinforceTypesDict ret{};
        for (auto damage_type : enumerators_of<calculator::DamageType>())
        {
            auto apt = integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type));

            auto atk_rate_str = attack_power_type_to_xml_string(apt);
            std::ranges::transform(atk_rate_str, atk_rate_str.begin(),
                [](unsigned char c){ return std::tolower(c); }
                );

            ret.attack.at(std::to_underlying(damage_type)) = row.at(std::format("{}AtkRate", atk_rate_str));
        }
        for (auto attribute : enumerators_of<calculator::RelevantAttribute>())
        {
            auto rate_str = std::format("correct{}Rate", attribute_to_xml_string(attribute));
            ret.attributeScaling.at(std::to_underlying(attribute)) = row.at(rate_str);
        }
        for (auto i = 0; i < 3; ++i)
        {
            auto s = std::format("spEffectId{}", i+1);
            if (row.contains(s))
                if (row.at(s) != 0)
                    ret.statusSpEffectId.at(i) = row.at(s);
        }
        return ret;
    }
    calculator::ScalingCurve evaluate_CalcCorrectGraph(const CalcCorrectGraph &calcCorrectGraph) {
        calculator::ScalingCurve arr{};

        for (auto i = 1; i < calcCorrectGraph.size(); i++)
        {
            auto &prevStage = calcCorrectGraph.at(i - 1);
            auto &stage = calcCorrectGraph.at(i);

            auto minAttributeValue = i == 1 ? 1 : prevStage.maxVal + 1;
            auto maxAttributeValue = (i == calcCorrectGraph.size() - 1) ? 148 : stage.maxVal;

            auto attributeValue = minAttributeValue;
            while (attributeValue <= maxAttributeValue)
            {
                if (not arr.at(attributeValue))
                {
                    auto ratio = double(attributeValue - prevStage.maxVal) / double(stage.maxVal - prevStage.maxVal);

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

    std::array<std::pair<double, std::string>, 6> get_scaling_tiers(const std::filesystem::path& menu_text_file, const std::filesystem::path& menu_value_table_params_file) {
        auto menu_text = xml::read_fmg_file(menu_text_file);

        std::array<std::pair<double, std::string>, 6> scaling_tiers{};
        auto  i = 0;
        for (auto &&[id, row] : xml::read_param_file<long long>(menu_value_table_params_file))
            if (row.at("compareType") == 1 && id >= 100)
                scaling_tiers.at(i++) = {row.at("value") / 100., menu_text.at(row.at("textId"))};

        return scaling_tiers;
    }

    calculator::AttackElementCorrectsById get_attack_element_corrects_by_id(const std::filesystem::path& attack_element_correct_param_file) {
        constexpr calculator::AttributeScaling default_{false, false, false, false, true}; // default value
        
        calculator::AttackElementCorrectsById attack_element_corrects_by_id{};
        for (auto &&[id, row] : xml::read_param_file<double>(attack_element_correct_param_file))
        {
            auto&& inserted = (attack_element_corrects_by_id[id] = parse_attack_element_correct(row));
            inserted[std::to_underlying(calculator::AttackPowerType::POISON)] = default_;
            inserted[std::to_underlying(calculator::AttackPowerType::BLEED)] = default_;
            inserted[std::to_underlying(calculator::AttackPowerType::MADNESS)] = default_;
            inserted[std::to_underlying(calculator::AttackPowerType::SLEEP)] = default_;
        }
        return attack_element_corrects_by_id;
    }

    export auto load_weapons(const std::filesystem::path &xml_data_directory) {
        auto scalingTiers = get_scaling_tiers(
            xml_data_directory / witchy::GR_MenuTextFile += ".xml",
            xml_data_directory / witchy::MenuValueTableParamFile += ".xml"
        );
        auto attackElementCorrectsById = get_attack_element_corrects_by_id(xml_data_directory / witchy::AttackElementCorrectParamFile += ".xml");

        auto spEffectParams = xml::read_param_file<long long>(xml_data_directory / witchy::SpEffectParamFile += ".xml");
        auto calcCorrectGraphs = xml::read_param_file<double>(xml_data_directory / witchy::CalcCorrectGraphFile += ".xml");
        auto equipParamWeapons = xml::read_param_file<double>(xml_data_directory / witchy::EquipParamWeaponFile += ".xml");
        auto reinforceParamWeapons = xml::read_param_file<double>(xml_data_directory / witchy::ReinforceParamWeaponFile += ".xml");
        auto weaponNames = xml::read_fmg_file(xml_data_directory / witchy::WeaponNameFile += ".xml");
        auto dlcWeaponNames = xml::read_fmg_file(xml_data_directory / witchy::WeaponName_dlc01File += ".xml");

        std::map<long long, std::vector<ReinforceTypesDict>> reinforce_types;
        for (auto &&[reinforce_param_id, reinforce_param_weapon] : reinforceParamWeapons)
        {
            auto reinforce_level = reinforce_param_id % 50;
            auto reinforce_type_id = reinforce_param_id - reinforce_level;
            auto &&reinforce_type = reinforce_types.try_emplace(reinforce_type_id).first->second;
            if (reinforce_type.size() == reinforce_level)
                reinforce_type.emplace_back(parse_reinforce_param_weapon(reinforce_param_weapon));
        }

        std::map<int, std::map<calculator::AttackPowerType, long long>> statusSpEffectParams{};
        for (auto &&[spEffectParamId, _] : spEffectParams)
        {
            auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId, spEffectParams);
            std::erase_if(status_sp_effect_params, [](auto &&v) { return v.second == 0; });
            statusSpEffectParams.try_emplace(spEffectParamId, status_sp_effect_params);
        }

        std::map<int, calculator::ScalingCurve> calcCorrectGraphsById{};
        auto get_calc_correct_graph_by_id = [&](long long calc_correct_graph_id)->const calculator::ScalingCurve& {
            auto&& [iterator, success] = calcCorrectGraphsById.try_emplace(calc_correct_graph_id);
            auto&& [_, scaling_curve] = *iterator;
            if (success)
                scaling_curve = evaluate_CalcCorrectGraph(
                    parse_calc_correct_graph(
                        calcCorrectGraphs.at(calc_correct_graph_id)
                    )
                );

            return scaling_curve;
        };

        std::vector<calculator::Weapon> weapons{};
        weapons.reserve(equipParamWeapons.size());
        for (auto &&[k, row] : equipParamWeapons)
        {
            auto row_id = assert_float_is_llong(row.at("id"));

            std::string name{};
            bool dlc{};
            if (weaponNames.contains(row_id))
                name = weaponNames.at(row_id);
            else if (dlcWeaponNames.contains(row_id))
            {
                name = dlcWeaponNames.at(row_id);
                dlc = isVanilla;
            }
            else
            {
                // std::println("ignoring: could not find weapon name for id: {}", row_id);
                continue;
            }

            if (name.find("[ERROR]") != std::string::npos || name.find("%null%") != std::string::npos)
            {
                // std::println("ignoring: weapon name: {}, id: {}", name, row_id);
                continue;
            }

            const auto weaponType = weapon_type_overrides.contains(row_id)
                ? std::to_underlying(weapon_type_overrides.at(row_id))
                : assert_float_is_llong(row.at("wepType"));
            if (!is_valid_enum_integral<calculator::Weapon::Type>(weaponType))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weaponType))
                {
                    // std::println("ignoring: weapon {} because no real weapon", name);
                    continue;
                }

                throw std::runtime_error(std::format("unknown weapon type {} for weapon {}", weaponType, name));
            }

            if (!reinforceParamWeapons.contains(row.at("reinforceTypeId")))
                throw std::runtime_error(std::format("could not find reinforce param weapon for reinforceTypeId: {}", std::to_string(row.at("reinforceTypeId"))));

            if (!attackElementCorrectsById.contains(row.at("attackElementCorrectId")))
                throw std::runtime_error(std::format("could not find attack element correct param for attackElementCorrectId: {}", std::to_string(row.at("attackElementCorrectId"))));

            const auto affinityId = assert_float_is_llong((row_id % 10000) / 100.);

            const auto equipParamWeaponsId = row_id - 100 * affinityId;
            if (!equipParamWeapons.contains(equipParamWeaponsId))
                throw std::runtime_error(std::format("could not find equip param weapon for id: {}", std::to_string(equipParamWeaponsId)));
            const auto &uninfusedWeapon = equipParamWeapons.at(equipParamWeaponsId);

            auto is_unique_weapon = uninfusedWeapon.at("gemMountType") == 0 || uninfusedWeapon.at("disableGemAttr") == 1;;
            if (affinityId != 0 && is_unique_weapon)
                throw std::runtime_error("unique weapon cannot have an affinity");

            std::set<calculator::AttackPowerType> attackPowerTypes{};
            std::array<long long, 3> statusSpEffectParamIds{};
            for (auto i = 0; i < 3; ++i)
            {
                auto spEffectParamId = assert_float_is_llong(row.at(std::format("spEffectBehaviorId{}", i)));
                auto statusSpEffectParams = parse_status_sp_effect_params(spEffectParamId, spEffectParams);
                if (!statusSpEffectParams.empty())
                {
                    for (auto &&[k, v] : statusSpEffectParams)
                        attackPowerTypes.insert(k);

                    statusSpEffectParamIds.at(i) = spEffectParamId;
                }
            }

            if (isVanilla && row_id == 32131200)
                statusSpEffectParamIds = {};

            std::vector<std::pair<calculator::AttackPowerType, long long>> unupgradedAttack{};
            for (auto damage_type : enumerators_of<calculator::DamageType>())
            {
                auto apt = integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type));
                auto attack_power = assert_float_is_llong(row.at(std::format("attackBase{}", attack_power_type_to_xml_string(apt))));

                if (attack_power != 0)
                {
                    attackPowerTypes.insert(apt);
                    unupgradedAttack.emplace_back(apt, attack_power);
                }
            }

            if (row.at("enableMagic") || row.at("enableMiracle"))
                for (auto &&damageType : enumerators_of<calculator::DamageType>())
                    attackPowerTypes.insert(integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damageType)));

            std::map<calculator::AttackPowerType, long long> calcCorrectGraphIds{};
            for (auto apt : enumerators_of<calculator::AttackPowerType>())
            {
                if (attackPowerTypes.contains(apt))
                {
                    if (!std::set{
                        calculator::AttackPowerType::SCARLET_ROT,
                        calculator::AttackPowerType::FROST,
                        calculator::AttackPowerType::DEATH_BLIGHT}.contains(apt)
                    )
                    {
                        auto xml_str = std::format("correctType_{}", attack_power_type_to_xml_string(apt));
                        auto def = is_valid_enum_integral<calculator::DamageType>(std::to_underlying(apt))
                            ? default_damage_calc_correct_graph_id
                            : default_status_calc_correct_graph_id;
                        if (row.contains(xml_str))
                        {
                            auto calcCorrectGraphId = assert_float_is_llong(row.at(xml_str));

                            if (!calcCorrectGraphs.contains(calcCorrectGraphId))
                                throw std::runtime_error(std::format("could not find calc correct graph for id: {}", calcCorrectGraphId));

                            calcCorrectGraphIds[apt] = calcCorrectGraphId;
                        } 
                    }
                }
            }

            std::vector<std::pair<calculator::RelevantAttribute, double>> unupgradedAttributeScaling{};
            for (auto attribute : enumerators_of<calculator::RelevantAttribute>())
            {
                auto xml_str = std::format("correct{}", attribute_to_xml_string(attribute));
                if (row.at(xml_str))
                    unupgradedAttributeScaling.emplace_back(attribute, row.at(xml_str) / 100.);
            }

            const auto &reinforceParams = reinforce_types.at(assert_float_is_llong(row.at("reinforceTypeId")));

            std::array<calculator::ScalingCurve, enumerators_of<calculator::AttackPowerType>().size()> weaponCalcCorrectGraphs{};
            for (auto damage_type : enumerators_of<calculator::DamageType>())
                weaponCalcCorrectGraphs.at(std::to_underlying(damage_type)) = get_calc_correct_graph_by_id(
                    map_get(
                        calcCorrectGraphIds,
                        integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type)),
                        default_damage_calc_correct_graph_id
                    )
                );
            for (auto status_type : enumerators_of<calculator::StatusEffectType>())
                weaponCalcCorrectGraphs.at(std::to_underlying(status_type)) = get_calc_correct_graph_by_id(
                    map_get(
                        calcCorrectGraphIds,
                        integral_to_enum<calculator::AttackPowerType>(std::to_underlying(status_type)),
                        default_status_calc_correct_graph_id
                    )
                );

            std::vector<std::array<double, enumerators_of<calculator::AttackPowerType>().size()>> attack{};
            for (const auto &reinforceParam : reinforceParams)
            {
                auto &attack_at_upgrade_level = attack.emplace_back();
                for (const auto &[attackPowerType, unupgradedAttackPower] : unupgradedAttack)
                    attack_at_upgrade_level.at(std::to_underlying(attackPowerType)) = unupgradedAttackPower * reinforceParam.attack.at(std::to_underlying(attackPowerType));

                int i = 0;
                for (const auto &spEffectParamId : statusSpEffectParamIds)
                {
                    if (spEffectParamId)
                    {
                        const auto &statusSpEffectParam = statusSpEffectParams.at(spEffectParamId + reinforceParam.statusSpEffectId.at(i));
                        for (const auto &[apt, val] : statusSpEffectParam)
                            attack_at_upgrade_level.at(std::to_underlying(apt)) = val;
                    }
                    i++;
                }
            }

            std::vector<calculator::AttributeScaling> attributeScaling{};
            for (const auto &reinforceParam : reinforceParams)
            {
                auto &foo = attributeScaling.emplace_back();
                for (const auto &[attribute, unupgradedScaling] : unupgradedAttributeScaling)
                    foo.at(std::to_underlying(attribute)) = unupgradedScaling * reinforceParam.attributeScaling.at(std::to_underlying(attribute));
            }

            auto weaponName = weaponNames.contains(uninfusedWeapon.at("id"))
                ? weaponNames.at(uninfusedWeapon.at("id"))
                : dlcWeaponNames.at(uninfusedWeapon.at("id"));
            
            auto url_part = weaponName;
            std::ranges::replace(url_part, ' ', '_');

            calculator::Stats required_stats{};
            for (auto attribute : enumerators_of<calculator::RelevantAttribute>())
                required_stats.at(std::to_underlying(attribute)) = assert_float_is_llong(row.at(std::format("proper{}", attribute_to_xml_string(attribute))));

            calculator::Weapon w{
                name,
                weaponName,
                "https://eldenring.fandom.com/wiki/" + url_part,
                dlc,
                row.at("isDualBlade") == 1,
                row.at("enableMagic") == 1,
                row.at("enableMiracle") == 1,
                integral_to_enum<calculator::Weapon::Type>(weaponType),
                integral_to_enum<calculator::Weapon::Affinity>(is_unique_weapon ? -1 : affinityId),
                required_stats,
                attributeScaling,
                attack,
                attackElementCorrectsById.at(assert_float_is_llong(row.at("attackElementCorrectId"))),
                weaponCalcCorrectGraphs,
                scalingTiers
            };

            weapons.emplace_back(std::move(w));
        }

        std::ranges::sort(weapons, {}, &calculator::Weapon::full_name);
        std::println("found {} weapons", weapons.size());
        return weapons;
    }
}