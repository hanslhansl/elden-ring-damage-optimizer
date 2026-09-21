module;
#include <pugixml.hpp>
export module erdo:witchy;
import :calculator;

import std;


template <typename CharT>
struct std::formatter<std::filesystem::path, CharT> : std::formatter<std::basic_string_view<CharT>, CharT>
{
    template <typename FormatContext>
    auto format(std::filesystem::path p, FormatContext& ctx) const
    {
        p.make_preferred();

        if constexpr (std::same_as<CharT, char>)
        {
            auto s = "\"" + p.string() + "\"";
            return std::formatter<std::basic_string_view<char>, char>::format(s, ctx);
        }
        else if constexpr (std::same_as<CharT, wchar_t>)
        {
            auto s = L"\"" + p.wstring() + L"\"";
            return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format(s, ctx);
        }
        else
        {
            static_assert(sizeof(CharT) == 0, "Unsupported character type for filesystem::path formatter");
        }
    }
};

using namespace erdo::calculator;

namespace erdo::witchy
{
    const std::set<std::filesystem::path> needed_dcx_files = {
        "regulation.bin",
        "msg/engus/menu.msgbnd.dcx",
        "msg/engus/menu_dlc01.msgbnd.dcx",
        "msg/engus/menu_dlc02.msgbnd.dcx",
        "msg/engus/item.msgbnd.dcx",
        "msg/engus/item_dlc01.msgbnd.dcx",
        "msg/engus/item_dlc02.msgbnd.dcx"
    };

    const std::filesystem::path AttackElementCorrectParamFile = "AttackElementCorrectParam.param";
    const std::filesystem::path CalcCorrectGraphFile = "CalcCorrectGraph.param";
    const std::filesystem::path EquipParamWeaponFile = "EquipParamWeapon.param";
    const std::filesystem::path ReinforceParamWeaponFile = "ReinforceParamWeapon.param";
    const std::filesystem::path SpEffectParamFile = "SpEffectParam.param";
    const std::filesystem::path MenuValueTableParamFile = "MenuValueTableParam.param";
    const std::filesystem::path WeaponNameFile = "WeaponName.fmg";
    const std::filesystem::path WeaponName_dlc01File = "WeaponName_dlc01.fmg";
    const std::filesystem::path GR_MenuTextFile = "GR_MenuText.fmg";

    const std::set<std::filesystem::path> needed_unpacked_files = {
        AttackElementCorrectParamFile,
        CalcCorrectGraphFile,
        EquipParamWeaponFile,
        ReinforceParamWeaponFile,
        SpEffectParamFile,
        MenuValueTableParamFile,
        WeaponNameFile,
        WeaponName_dlc01File,
        GR_MenuTextFile
    };

    long long assert_float_is_llong(double f)
    {
        if (f != (long long)f)
            throw std::runtime_error("float is not long long");
        return f;
    }

    std::string quote_path(const std::filesystem::path& p)
    {
        return std::format("{}", p);
    }


    pugi::xml_document load_file(const std::filesystem::path &file_path)
    {
        pugi::xml_document data;
        auto result = data.load_file(file_path.c_str(), pugi::parse_default, pugi::xml_encoding::encoding_utf8);
        if (!result)
            throw std::runtime_error(std::format("could not load xml file {}: {}", file_path, result.description()));
        return data;
    }

    template<typename T>
    auto get_value(const auto& attr, T def = {})
    {
        if constexpr (std::same_as<T, double>)
            return attr.as_double(def);
        else if constexpr (std::same_as<T, long long>)
            return attr.as_llong(def);
        else if constexpr (std::same_as<T, std::string>)
            return std::string(attr.as_string(def.c_str()));
        else
            static_assert(false, "unsupported type");
    }

    template<typename T>
    T get_element_value(const pugi::xml_document& doc, const std::vector<std::string>& path, T def = {})
    {
        pugi::xml_node node{};
        for (auto&& element : path)
        {
            if (node)
                node = node.child(element.c_str());
            else
                node = doc.child(element.c_str());

            if (!node)
                throw std::runtime_error(std::format("could not find element {}", element));
        }

        return get_value<T>(node.text(), def);
    }

    template<typename T>
        requires (std::same_as<T, double> || std::same_as<T, long long>)
    std::map<long long, std::map<std::string, T>> read_param_file(const std::filesystem::path &file_path)
    {
        pugi::xml_document data = load_file(file_path);

        auto field_nodes = data.child("param").child("fields").children("field");

        std::map<std::string, T> default_values{};
        for (auto &&field_node : field_nodes)
        {
            auto name = field_node.attribute("name").as_string();
            auto defaultValue = get_value<T>(field_node.attribute("defaultValue"), std::numeric_limits<T>::max());

            if (defaultValue != std::numeric_limits<T>::max())
                default_values.emplace(name, defaultValue);
        }

        auto row_nodes = data.child("param").child("rows").children("row");

        std::map<long long, std::map<std::string, T>> ret{};
        for (auto &&row_node : row_nodes)
        {
            auto name = row_node.attribute("name").as_string();

            std::map<std::string, T> row_data = default_values;
            for (auto &&row_attribute : row_node.attributes())
            {
                row_data[row_attribute.name()] = get_value<T>(row_attribute);
            }

            auto id = row_node.attribute("id").as_llong();
            ret.emplace(id, std::move(row_data));
        }

        return ret;
    }
    std::map<long long, std::string> read_fmg_file(const std::filesystem::path &file_path)
    {
        pugi::xml_document data = load_file(file_path);

        auto text_nodes = data.child("fmg").child("entries").children("text");

        std::map<long long, std::string> ret{};
        for (auto &&text_node : text_nodes)
        {
            auto id = text_node.attribute("id").as_llong();
            auto text = text_node.child_value();
            ret.emplace(id, text);
        }

        return ret;
    }


    using AttackElementCorrectsById = std::map<int, std::shared_ptr<AttackElementCorrects>>;
    using ParamRow = std::map<std::string, double>;
    struct CalcCorrectGraphEntry
    {
        long long maxVal;
        double maxGrowVal, adjPt;
    };
    using CalcCorrectGraph = std::array<CalcCorrectGraphEntry, 5>;
    struct ReinforceTypesDict
    {
        AttributeScalings attack;             // index: DamageType
        AttributeScalings attributeScaling;   // index: RelevantAttribute
        std::array<int, 3> statusSpEffectId;            // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3
    };

    const std::map<long long, Weapon::Type> weapon_type_overrides = {{110000, Weapon::Type::FIST}};
    constexpr bool isVanilla = true;
    constexpr long long default_damage_calc_correct_graph_id = 0;
    constexpr long long default_status_calc_correct_graph_id = 6;

    std::string attribute_to_xml_string(RelevantAttribute attr)
    {
        if (attr == RelevantAttribute::STRENGTH)
            return "Strength";
        if (attr == RelevantAttribute::DEXTERITY)
            return "Agility";
        if (attr == RelevantAttribute::INTELLIGENCE)
            return "Magic";
        if (attr == RelevantAttribute::FAITH)
            return "Faith";
        if (attr == RelevantAttribute::ARCAINE)
            return "Luck";

        throw std::invalid_argument("unknown attribute");
    }
    std::string attack_power_type_to_xml_string(AttackPowerType apt)
    {
        if (apt == AttackPowerType::PHYSICAL)
            return "Physics";
        if (apt == AttackPowerType::MAGIC)
            return "Magic";
        if (apt == AttackPowerType::FIRE)
            return "Fire";
        if (apt == AttackPowerType::LIGHTNING)
            return "Thunder";
        if (apt == AttackPowerType::HOLY)
            return "Dark";

        if (apt == AttackPowerType::POISON)
            return "Poison";
        if (apt == AttackPowerType::BLEED)
            return "Bleed";
        if (apt == AttackPowerType::SLEEP)
            return "Sleep";
        if (apt == AttackPowerType::MADNESS)
            return "Madness";

        throw std::invalid_argument("unknown attack power type");
    }
    
    std::map<AttackPowerType, long long> parse_status_sp_effect_params(
        long long statusSpEffectParamId,
        const std::map<long long, std::map<std::string, long long>>& spEffectParams
    )
    {
        if (!spEffectParams.contains(statusSpEffectParamId))
            return {};
        auto &&spEffectRow = spEffectParams.at(statusSpEffectParamId);

        std::map<AttackPowerType, long long> statuses =  {
            {AttackPowerType::POISON, spEffectRow.at("poizonAttackPower")},
            {AttackPowerType::SCARLET_ROT, spEffectRow.at("diseaseAttackPower")},
            {AttackPowerType::BLEED, spEffectRow.at("bloodAttackPower")},
            {AttackPowerType::FROST, spEffectRow.at("freezeAttackPower")},
            {AttackPowerType::SLEEP, spEffectRow.at("sleepAttackPower")},
            {AttackPowerType::MADNESS, spEffectRow.at("madnessAttackPower")},
            {AttackPowerType::DEATH_BLIGHT, spEffectRow.at("curseAttackPower")}
        };

        if (std::ranges::any_of(statuses, [](auto &&v) { return v.second != 0; }))
            return statuses;

        return {};
    }
    CalcCorrectGraph parse_calc_correct_graph(const std::map<std::string, double> &row)
    {
        CalcCorrectGraph ret{};
        for (auto i = 0; i < 5; ++i)
        {
            ret.at(i) = CalcCorrectGraphEntry{
                assert_float_is_llong(row.at(std::format("stageMaxVal{}", i))),
                row.at(std::format("stageMaxGrowVal{}", i)) / 100.,
                row.at(std::format("adjPt_maxGrowVal{}", i))
            };
        }
        return ret;
    }

    std::shared_ptr<AttackElementCorrects> parse_attack_element_correct(const ParamRow &row)
    {
        auto ret = std::make_shared<AttackElementCorrects>();
        for (auto damage_type : enumerators_of<DamageType>())
        {
            auto apt = integral_to_enum<AttackPowerType>(std::to_underlying(damage_type));
            auto apt_str = attack_power_type_to_xml_string(apt);

            auto&& attribute_scaling = ret->at(std::to_underlying(apt));

            for (auto attribute : enumerators_of<RelevantAttribute>())
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
    ReinforceTypesDict parse_reinforce_param_weapon(const ParamRow &row)
    {
        ReinforceTypesDict ret{};
        for (auto damage_type : enumerators_of<DamageType>())
        {
            auto apt = integral_to_enum<AttackPowerType>(std::to_underlying(damage_type));

            auto atk_rate_str = attack_power_type_to_xml_string(apt);
            std::ranges::transform(atk_rate_str, atk_rate_str.begin(),
                [](unsigned char c){ return std::tolower(c); }
                );

            ret.attack.at(std::to_underlying(damage_type)) = row.at(std::format("{}AtkRate", atk_rate_str));
        }
        for (auto attribute : enumerators_of<RelevantAttribute>())
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
    std::shared_ptr<ScalingCurve> evaluate_CalcCorrectGraph(const CalcCorrectGraph &calcCorrectGraph)
    {
        auto arr = std::make_shared<ScalingCurve>();

        for (auto i = 1; i < calcCorrectGraph.size(); i++)
        {
            auto &prevStage = calcCorrectGraph.at(i - 1);
            auto &stage = calcCorrectGraph.at(i);

            auto minAttributeValue = i == 1 ? 1 : prevStage.maxVal + 1;
            auto maxAttributeValue = (i == calcCorrectGraph.size() - 1) ? 148 : stage.maxVal;

            auto attributeValue = minAttributeValue;
            while (attributeValue <= maxAttributeValue)
            {
                if (!arr->at(attributeValue))
                {
                    auto ratio = double(attributeValue - prevStage.maxVal) / double(stage.maxVal - prevStage.maxVal);

                    if (prevStage.adjPt > 0)
                        ratio = std::pow(ratio, prevStage.adjPt);
                    else if (prevStage.adjPt < 0)
                        ratio = 1 - std::pow((1 - ratio), -prevStage.adjPt);

                    arr->at(attributeValue) = prevStage.maxGrowVal + (stage.maxGrowVal - prevStage.maxGrowVal) * ratio;
                }
                attributeValue += 1;
            }
        }

        return arr;
    }

    std::shared_ptr<ScalingTiers> get_scaling_tiers(const std::filesystem::path& menu_text_file, const std::filesystem::path& menu_value_table_params_file)
    {
        auto menu_text = read_fmg_file(menu_text_file);

        auto scaling_tiers = std::make_shared<ScalingTiers>();
        auto  i = 0;
        for (auto &&[id, row] : read_param_file<long long>(menu_value_table_params_file))
            if (row.at("compareType") == 1 && id >= 100)
                scaling_tiers->at(i++) = {row.at("value") / 100., menu_text.at(row.at("textId"))};

        return scaling_tiers;
    }

    AttackElementCorrectsById get_attack_element_corrects_by_id(const std::filesystem::path& attack_element_correct_param_file)
    {
        constexpr AttributeScalings default_{false, false, false, false, true}; // default value
        
        AttackElementCorrectsById attack_element_corrects_by_id{};
        for (auto &&[id, row] : read_param_file<double>(attack_element_correct_param_file))
        {
            auto&& inserted = (attack_element_corrects_by_id[id] = parse_attack_element_correct(row));
            (*inserted)[std::to_underlying(AttackPowerType::POISON)] = default_;
            (*inserted)[std::to_underlying(AttackPowerType::BLEED)] = default_;
            (*inserted)[std::to_underlying(AttackPowerType::MADNESS)] = default_;
            (*inserted)[std::to_underlying(AttackPowerType::SLEEP)] = default_;
        }
        return attack_element_corrects_by_id;
    }

    export GameData load_game_data(const std::filesystem::path &xml_data_directory, const std::string& game_version)
    {
        auto scaling_tiers = get_scaling_tiers(
            xml_data_directory / GR_MenuTextFile += ".xml",
            xml_data_directory / MenuValueTableParamFile += ".xml"
        );

        const auto attackElementCorrectsById = get_attack_element_corrects_by_id(xml_data_directory / AttackElementCorrectParamFile += ".xml");

        const auto spEffectParams = read_param_file<long long>(xml_data_directory / SpEffectParamFile += ".xml");
        const auto calcCorrectGraphs = read_param_file<double>(xml_data_directory / CalcCorrectGraphFile += ".xml");
        const auto equipParamWeapons = read_param_file<double>(xml_data_directory / EquipParamWeaponFile += ".xml");
        const auto reinforceParamWeapons = read_param_file<double>(xml_data_directory / ReinforceParamWeaponFile += ".xml");
        const auto weaponNames = read_fmg_file(xml_data_directory / WeaponNameFile += ".xml");
        const auto dlcWeaponNames = read_fmg_file(xml_data_directory / WeaponName_dlc01File += ".xml");

        std::map<long long, std::vector<ReinforceTypesDict>> reinforce_types;
        for (auto &&[reinforce_param_id, reinforce_param_weapon] : reinforceParamWeapons)
        {
            auto reinforce_level = reinforce_param_id % 50;
            auto reinforce_type_id = reinforce_param_id - reinforce_level;
            auto &&reinforce_type = reinforce_types.try_emplace(reinforce_type_id).first->second;
            if (reinforce_type.size() == reinforce_level)
                reinforce_type.emplace_back(parse_reinforce_param_weapon(reinforce_param_weapon));
        }

        std::map<int, std::map<AttackPowerType, long long>> statusSpEffectParams{};
        for (auto &&[spEffectParamId, _] : spEffectParams)
        {
            auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId, spEffectParams);
            std::erase_if(status_sp_effect_params, [](auto &&v) { return v.second == 0; });
            statusSpEffectParams.try_emplace(spEffectParamId, status_sp_effect_params);
        }

        std::map<int, std::shared_ptr<ScalingCurve>> calcCorrectGraphsById{};
        auto get_calc_correct_graph_by_id = [&](long long calc_correct_graph_id) {
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

        GameData game_data{ ERDO_VERSION, game_version };
        game_data.weapons.reserve(equipParamWeapons.size());
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
                continue;
            }

            if (name.find("[ERROR]") != std::string::npos || name.find("%null%") != std::string::npos)
                continue;

            const auto weaponType = weapon_type_overrides.contains(row_id)
                ? std::to_underlying(weapon_type_overrides.at(row_id))
                : assert_float_is_llong(row.at("wepType"));
            if (!is_valid_enum_integral<Weapon::Type>(weaponType))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weaponType))
                    continue;

                throw std::runtime_error(std::format("unknown weapon type {} for weapon {}", weaponType, name));
            }

            auto reinforce_type_id = assert_float_is_llong(row.at("reinforceTypeId"));
            if (!reinforceParamWeapons.contains(reinforce_type_id))
                throw std::runtime_error(std::format("could not find reinforce param weapon for reinforceTypeId: {}", reinforce_type_id));
            const auto &reinforceParams = reinforce_types.at(reinforce_type_id);

            auto attack_element_correct_id = assert_float_is_llong(row.at("attackElementCorrectId"));
            if (!attackElementCorrectsById.contains(attack_element_correct_id))
                throw std::runtime_error(std::format("could not find attack element correct param for attackElementCorrectId: {}", attack_element_correct_id));
            auto attack_power_types_attribute_scalings = attackElementCorrectsById.at(attack_element_correct_id);

            const auto affinityId = assert_float_is_llong((row_id % 10000) / 100.);

            const auto equipParamWeaponsId = row_id - 100 * affinityId;
            if (!equipParamWeapons.contains(equipParamWeaponsId))
                throw std::runtime_error(std::format("could not find equip param weapon for id: {}", equipParamWeaponsId));
            const auto &uninfusedWeapon = equipParamWeapons.at(equipParamWeaponsId);

            auto is_unique_weapon = uninfusedWeapon.at("gemMountType") == 0 || uninfusedWeapon.at("disableGemAttr") == 1;;
            if (affinityId != 0 && is_unique_weapon)
                throw std::runtime_error("unique weapon cannot have an affinity");

            std::set<AttackPowerType> attackPowerTypes{};
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

            std::vector<std::pair<AttackPowerType, long long>> unupgradedAttack{};
            for (auto damage_type : enumerators_of<DamageType>())
            {
                auto apt = integral_to_enum<AttackPowerType>(std::to_underlying(damage_type));
                auto attack_power = assert_float_is_llong(row.at(std::format("attackBase{}", attack_power_type_to_xml_string(apt))));

                if (attack_power != 0)
                {
                    attackPowerTypes.insert(apt);
                    unupgradedAttack.emplace_back(apt, attack_power);
                }
            }

            if (row.at("enableMagic") || row.at("enableMiracle"))
                for (auto &&damageType : enumerators_of<DamageType>())
                    attackPowerTypes.insert(integral_to_enum<AttackPowerType>(std::to_underlying(damageType)));

            std::map<AttackPowerType, long long> calcCorrectGraphIds{};
            for (auto apt : enumerators_of<AttackPowerType>())
            {
                if (attackPowerTypes.contains(apt))
                {
                    if (!std::set{
                        AttackPowerType::SCARLET_ROT,
                        AttackPowerType::FROST,
                        AttackPowerType::DEATH_BLIGHT}.contains(apt)
                    )
                    {
                        auto xml_str = std::format("correctType_{}", attack_power_type_to_xml_string(apt));
                        auto def = is_valid_enum_integral<DamageType>(std::to_underlying(apt))
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

            std::vector<std::pair<RelevantAttribute, double>> unupgradedAttributeScaling{};
            for (auto attribute : enumerators_of<RelevantAttribute>())
            {
                auto xml_str = std::format("correct{}", attribute_to_xml_string(attribute));
                if (row.at(xml_str))
                    unupgradedAttributeScaling.emplace_back(attribute, row.at(xml_str) / 100.);
            }

            std::array<std::shared_ptr<ScalingCurve>, enumerators_of<AttackPowerType>().size()> weaponCalcCorrectGraphs{};
            for (auto damage_type : enumerators_of<DamageType>())
                weaponCalcCorrectGraphs.at(std::to_underlying(damage_type)) = get_calc_correct_graph_by_id(
                    map_get(
                        calcCorrectGraphIds,
                        integral_to_enum<AttackPowerType>(std::to_underlying(damage_type)),
                        default_damage_calc_correct_graph_id
                    )
                );
            for (auto status_type : enumerators_of<StatusEffectType>())
                weaponCalcCorrectGraphs.at(std::to_underlying(status_type)) = get_calc_correct_graph_by_id(
                    map_get(
                        calcCorrectGraphIds,
                        integral_to_enum<AttackPowerType>(std::to_underlying(status_type)),
                        default_status_calc_correct_graph_id
                    )
                );

            std::vector<std::array<double, enumerators_of<AttackPowerType>().size()>> attack{};
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

            std::vector<AttributeScalings> attributeScalings{};
            for (const auto &reinforceParam : reinforceParams)
            {
                auto &attributeScaling = attributeScalings.emplace_back();
                for (const auto &[attribute, unupgradedScaling] : unupgradedAttributeScaling)
                    attributeScaling.at(std::to_underlying(attribute)) = unupgradedScaling * reinforceParam.attributeScaling.at(std::to_underlying(attribute));
            }

            auto weaponName = weaponNames.contains(uninfusedWeapon.at("id"))
                ? weaponNames.at(uninfusedWeapon.at("id"))
                : dlcWeaponNames.at(uninfusedWeapon.at("id"));
            
            RelevantAttributeLevelsArray required_relevant_stats{};
            for (auto attribute : enumerators_of<RelevantAttribute>())
                required_relevant_stats.at(std::to_underlying(attribute)) = assert_float_is_llong(row.at(std::format("proper{}", attribute_to_xml_string(attribute))));

            Weapon w{
                name,
                weaponName,
                dlc,
                row.at("isDualBlade") == 1,
                row.at("enableMagic") == 1,
                row.at("enableMiracle") == 1,
                integral_to_enum<Weapon::Type>(weaponType),
                integral_to_enum<Weapon::Affinity>(is_unique_weapon ? -1 : affinityId),
                required_relevant_stats,
                attributeScalings,
                attack,
                attack_power_types_attribute_scalings,
                weaponCalcCorrectGraphs,
                scaling_tiers
            };

            game_data.weapons.emplace_back(std::move(w));
        }

        std::ranges::sort(game_data.weapons, {}, &Weapon::full_name);
        std::println("found {} weapons", game_data.weapons.size());
        return game_data;
    }


    std::string witchy_cmd(const std::filesystem::path& witchy_exe_path, const std::filesystem::path& arg, std::filesystem::path location = {})
    {
        auto s = std::format("\"{} --passive", witchy_exe_path);

        if (!location.empty())
            s += std::format(" --location {}", quote_path(location));

        return s + std::format(" {}\"", arg);
    }

    export std::expected<GameData, std::string> run_witchy(
        const std::filesystem::path& input_files_directory,
        const std::filesystem::path& witchy_exe_path,
        bool delete_temp_directory
    ) try
    {
        if (!std::filesystem::is_directory(input_files_directory))
            return std::unexpected(std::format("Not a directory: {}", input_files_directory));

        if (!std::filesystem::is_regular_file(witchy_exe_path))
            return std::unexpected(std::format("Not a file: {}", witchy_exe_path));
        
        // create temporary directory
        auto temp_dir = std::filesystem::temp_directory_path() / "elden-ring-damage-optimizer";
        std::filesystem::remove_all(temp_dir);
        std::filesystem::create_directory(temp_dir);
        std::println("created temporary directory {}", temp_dir);

        // copy needed dcx files to temporary directory
        auto dcx_data_directory = temp_dir / "dcx_data";
        std::filesystem::create_directory(dcx_data_directory);
        auto dcx_file_paths = needed_dcx_files
            | std::views::transform([&](const std::filesystem::path &input_dcx_file) {
                auto input_dcx_file_path = input_files_directory / input_dcx_file;
                auto new_dcx_file_path = dcx_data_directory / input_dcx_file.filename();
                std::filesystem::copy_file(input_dcx_file_path, new_dcx_file_path, std::filesystem::copy_options::overwrite_existing);
                return new_dcx_file_path;
            })
            | std::ranges::to<std::vector>();
        std::println("copied needed dcx files to {}", dcx_data_directory);

        // unpack dcx files, move the resulting unpacked data directories
        auto unpacked_data_directory = temp_dir / "unpacked_data";
        std::filesystem::create_directory(unpacked_data_directory);
        std::vector<std::filesystem::path> unpacked_data_directories{};
        for (auto&& dcx_file_path : dcx_file_paths)
        {
            auto cmd = witchy_cmd(witchy_exe_path, dcx_file_path);
            std::println("executing command: {}", cmd);
            auto result = std::system(cmd.c_str());
            if (result != 0)
                return std::unexpected(std::format("WitchyBND failed with exit code {} for file {}", result, dcx_file_path));
            std::println("unpacked dcx file {} in-place", dcx_file_path);

            auto directory_name = dcx_file_path.filename().string();
            std::ranges::replace(directory_name, '.', '-');
            auto current_path = dcx_data_directory / directory_name;
            auto new_path = unpacked_data_directory / directory_name;
            std::filesystem::rename(current_path, new_path);
            unpacked_data_directories.emplace_back(new_path);
            std::println("copied unpacked data directory {} to {}", current_path, new_path);
        }

        // get game version from /regulation-bin/_witchy-bnd4.xml
        auto game_version = get_element_value<long long>(
            load_file(unpacked_data_directory / "regulation-bin" / "_witchy-bnd4.xml"),
            { "bnd4", "version" }
        );
        std::println("found game version {}", game_version);

        // compute paths of needed unpacked files
        auto unpacked_file_paths = unpacked_data_directories
            | std::views::transform([](const std::filesystem::path& p){ return std::filesystem::directory_iterator(p); })
            | std::views::join
            | std::views::transform(&std::filesystem::directory_entry::path)
            | std::views::filter([](const std::filesystem::path& p){ return needed_unpacked_files.contains(p.filename()); })
            | std::ranges::to<std::set>([](const std::filesystem::path& l, const std::filesystem::path& r){ return std::less{}(l.filename(), r.filename()); });
        if (unpacked_file_paths.size() != needed_unpacked_files.size())
            return std::unexpected(std::format(
                "only {} out of {} needed unpacked files were found in {}",
                unpacked_file_paths.size(),
                needed_unpacked_files.size(),
                unpacked_data_directory
            ));
        std::println("found {} needed unpacked files in {}", unpacked_file_paths.size(), unpacked_data_directory);

        // convert needed unpacked files to xml
        auto xml_data_directory = temp_dir / "xml_data";
        std::filesystem::create_directory(xml_data_directory);
        for (auto&& unpacked_file_path : unpacked_file_paths)
        {
            auto cmd = witchy_cmd(witchy_exe_path, unpacked_file_path, xml_data_directory);
            std::println("executing command: {}", cmd);
            auto result = std::system(cmd.c_str());
            if (result != 0)
                return std::unexpected(std::format("WitchyBND failed with exit code {} for file {}", result, unpacked_file_path));
        }
        std::println("converted needed unpacked files to xml in {}", xml_data_directory);

        // parse xml files
        auto game_data = load_game_data(xml_data_directory, game_version);
        std::println("\nsuccessfully unpacked and parsed game data"); 

        // potentially remove temporary directory
        // if (delete_temp_directory)
        // {
        //     std::filesystem::remove_all(temp_dir);
        //     std::println("removed temporary directory {}", temp_dir_quoted);
        // }

        return game_data;
    }
    catch (const std::exception& e)
    {
        return std::unexpected(e.what());
    }
}
