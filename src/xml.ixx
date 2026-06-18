module;
#include <pugixml.hpp>
#include <stdexcept>
export module erdo:xml;
import :witchy;
import :calculator;

import std;

namespace xml
{
    using ParamRow = std::map<std::string, std::variant<long long, double>>;


    template<typename...Args>
    std::conditional_t<sizeof...(Args) == 1, Args...[0], std::variant<Args...>> get_attribute_value(pugi::xml_attribute attribute)
    {
        template for (constexpr auto r : {^^Args...})
        {
            using T = [:r:];

            T def_val{}, value{};
            if constexpr (std::same_as<T, long long>)
            {
                def_val = std::numeric_limits<T>::max();
                value = attribute.as_llong(def_val);
            }
            else if constexpr (std::same_as<T, double>)
            {
                def_val = std::numeric_limits<T>::max();
                value = attribute.as_double(def_val);
            }
            else if constexpr (std::same_as<T, std::string>)
            {
                value = attribute.as_string(def_val.c_str());
            }
            else
                static_assert(false, "unsupported type");

            if (value != def_val)
                return value;
        }

        throw std::invalid_argument(std::format("attribute {} has no valid value", attribute.name()));
    }

    std::map<long long, ParamRow> read_param_xml(const std::filesystem::path &file_path)
    {
        pugi::xml_document data;
        auto result = data.load_file(file_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
        if (!result)
            throw std::runtime_error(std::format("could not load xml file: {}", result.description()));

        auto field_nodes = data.child("param").child("fields").children("field");

        ParamRow default_values{};
        for (auto &&field_node : field_nodes)
        {
            auto name = get_attribute_value<std::string>(field_node.attribute("name"));
            default_values[name] = get_attribute_value<long long, double>(field_node.attribute("defaultValue"));
        }

        auto row_nodes = data.child("param").child("rows").children("row");

        std::map<long long, ParamRow> ret{};
        for (auto &&row_node : row_nodes)
        {
            auto name = get_attribute_value<std::string>(row_node.attribute("name"));

            ParamRow row_data = default_values;
            for (auto &&row_attribute : row_node.attributes())
                row_data[row_attribute.name()] = get_attribute_value<long long, double>(row_attribute);

            auto id = get_attribute_value<long long>(row_node.attribute("id"));
            ret[id] = std::move(row_data);
        }

        return ret;
    }
    std::map<long long, std::string> read_fmg_xml(const std::filesystem::path &file_path)
    {
        pugi::xml_document data;
        auto result = data.load_file(file_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
        if (!result)
            throw std::runtime_error(std::format("could not load xml file: {}", result.description()));

        auto text_nodes = data.child("fmg").child("entries").children("text");

        std::map<long long, std::string> ret{};
        for (auto &&text_node : text_nodes)
        {
            auto id = get_attribute_value<long long>(text_node.attribute("id"));
            auto text = text_node.child_value();
            ret.emplace(id, text);
        }

        return ret;
    }
    
    const std::map<long long, calculator::Weapon::Type> weapon_type_overrides = {{110000, calculator::Weapon::Type::FIST}};

    class Parser
    {
        std::map<long long, ParamRow> AttackElementCorrectParam, CalcCorrectGraph, EquipParamWeapon, ReinforceParamWeapon, SpEffectParam, MenuValueTableParam;
        std::map<long long, std::string> WeaponName, WeaponName_dlc01, GR_MenuText;

        std::map<calculator::AttackPowerType, double> parse_status_sp_effect_params(long long statusSpEffectParamId) const
        {
            if (!this->SpEffectParam.contains(statusSpEffectParamId))
                return {};
            auto &&spEffectRow = this->SpEffectParam.at(statusSpEffectParamId);

            std::map<calculator::AttackPowerType, double> statuses = {
                {calculator::AttackPowerType::POISON, std::get<double>(spEffectRow.at("poizonAttackPower"))},
                {calculator::AttackPowerType::SCARLET_ROT, std::get<double>(spEffectRow.at("diseaseAttackPower"))},
                {calculator::AttackPowerType::BLEED, std::get<double>(spEffectRow.at("bloodAttackPower"))},
                {calculator::AttackPowerType::FROST, std::get<double>(spEffectRow.at("freezeAttackPower"))},
                {calculator::AttackPowerType::SLEEP, std::get<double>(spEffectRow.at("sleepAttackPower"))},
                {calculator::AttackPowerType::MADNESS, std::get<double>(spEffectRow.at("madnessAttackPower"))},
                {calculator::AttackPowerType::DEATH_BLIGHT, std::get<double>(spEffectRow.at("curseAttackPower"))}
            };

            if (std::ranges::any_of(statuses, [](auto &&v) { return v.second != 0; }))
                return statuses;

            return {};
        }

        auto parse_weapon(const ParamRow &row)
        {
            auto row_id = std::get<long long>(row.at("id"));

            std::string name{};
            bool dlc = false;

            if (this->WeaponName.contains(row_id))
                name = this->WeaponName.at(row_id);
            else if (this->WeaponName_dlc01.contains(row_id))
            {
                name = this->WeaponName_dlc01.at(row_id);
                dlc = true;
            }
            else
                throw std::runtime_error(std::format("could not find weapon name for id: {}", row_id));

            if (name.contains("[ERROR]") || name.contains("%null%"))
                throw std::runtime_error(std::format("ignoring: weapon name: {}, id: {}", name, row_id));

            auto weapon_type = weapon_type_overrides.contains(row_id) ? std::to_underlying(weapon_type_overrides.at(row_id)) : std::get<long long>(row.at("wepType"));
            if (!is_valid_enum_integral<calculator::Weapon::Type>(weapon_type))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weapon_type))
                    std::println("ignoring: weapon {} because no real weapon", name);
                else
                    throw std::runtime_error(std::format("unknown weapon type {} for weapon {}", weapon_type, name));
                
                throw std::runtime_error("unknown weapon type");
            }

            if (!this->ReinforceParamWeapon.contains(std::get<long long>(row.at("reinforceTypeId"))))
                throw std::runtime_error(std::format("could not find reinforce param weapon for reinforceTypeId: {}", std::get<long long>(row.at("reinforceTypeId"))));

            if (!this->AttackElementCorrectParam.contains(std::get<long long>(row.at("attackElementCorrectId"))))
                throw std::runtime_error(std::format("could not find attack element correct param for attackElementCorrectId: {}", std::get<long long>(row.at("attackElementCorrectId"))));

            auto affinityId_ = (row_id % 10000) / 100.;
            auto affinityId = (long long)(affinityId_);
            if (affinityId_ != affinityId)
                throw std::runtime_error(std::format("invalid affinityId {} for weapon {}", affinityId_, name));

            auto equipParamWeaponsId = row_id - 100 * affinityId;
            if (!this->EquipParamWeapon.contains(equipParamWeaponsId))
                throw std::runtime_error(std::format("could not find equip param weapon for id: {}", equipParamWeaponsId));
            const auto &uninfusedWeapon = this->EquipParamWeapon.at(equipParamWeaponsId);

            auto is_unique_weapon = std::get<long long>(uninfusedWeapon.at("gemMountType")) == 0 || std::get<long long>(uninfusedWeapon.at("disableGemAttr")) == 1;
            if (affinityId != 0 && is_unique_weapon)
                throw std::runtime_error("unique weapon cannot have an affinity");

            std::set<calculator::AttackPowerType> attack_power_types{};
            std::vector<long long> status_sp_effect_param_ids{};
            for (auto spEffectBehaviorId : { "spEffectBehaviorId0", "spEffectBehaviorId1", "spEffectBehaviorId2" })
            {
                auto sp_effect_param_id = std::get<long long>(row.at(spEffectBehaviorId));

                auto statusSpEffectParams = parse_status_sp_effect_params(sp_effect_param_id);
                if (!statusSpEffectParams.empty())
                {
                    for (auto &&[k, v] : statusSpEffectParams)
                        attack_power_types.insert(k);

                    status_sp_effect_param_ids.emplace_back(sp_effect_param_id);
                }
                else
                    status_sp_effect_param_ids.emplace_back(0);
            }

            static_assert(false, "unfinished");

            return json{};
        }

    public:

        Parser(const std::filesystem::path &xml_data_directory) : 
            AttackElementCorrectParam(read_param_xml(xml_data_directory / witchy::AttackElementCorrectParamFile += ".xml")),
            CalcCorrectGraph(read_param_xml(xml_data_directory / witchy::CalcCorrectGraphFile += ".xml")),
            EquipParamWeapon(read_param_xml(xml_data_directory / witchy::EquipParamWeaponFile += ".xml")),
            ReinforceParamWeapon(read_param_xml(xml_data_directory / witchy::ReinforceParamWeaponFile += ".xml")),
            SpEffectParam(read_param_xml(xml_data_directory / witchy::SpEffectParamFile += ".xml")),
            MenuValueTableParam(read_param_xml(xml_data_directory / witchy::MenuValueTableParamFile += ".xml")),
            WeaponName(read_fmg_xml(xml_data_directory / witchy::WeaponNameFile += ".xml")),
            WeaponName_dlc01(read_fmg_xml(xml_data_directory / witchy::WeaponName_dlc01File += ".xml")),
            GR_MenuText(read_fmg_xml(xml_data_directory / witchy::GR_MenuTextFile += ".xml"))
        {
            
        }

        void load_xml_data()
        {
            json weapons_json = json::array();
            for (auto &&[k, param_row] : this->EquipParamWeapon)
            {
                auto weapon_json = this->parse_weapon(param_row);
                if (!weapon_json.empty())
                    weapons_json.push_back(weapon_json);
            }

            // std::set<long long> calc_correct_graph_ids{defaultDamageCalcCorrectGraphId, defaultStatusCalcCorrectGraphId};
            // for (auto &&weapon_json : weapons_json)
            //     for (auto &&[apt, calcCorrectGraphId] : weapon_json.at("calcCorrectGraphIds").items())
            //         calc_correct_graph_ids.insert(calcCorrectGraphId.get<long long>());
            // json calc_correct_graphs_json{};
            // for (auto &&[id, calc_correct_graph] : this->calcCorrectGraphs)
            //     if (calc_correct_graph_ids.contains(id))
            //         calc_correct_graphs_json[std::to_string(id)] = parse_calc_correct_graph(calc_correct_graph);

            // std::set<long long> attackElementCorrectIds{};
            // for (auto &&weapon_json : weapons_json)
            //     attackElementCorrectIds.insert(weapon_json.at("attackElementCorrectId").get<long long>());
            // json attack_element_corrects_json{};
            // for (auto &&[id, row] : this->attackElementCorrectParams)
            //     if (attackElementCorrectIds.contains(id))
            //         attack_element_corrects_json[std::to_string(id)] = parse_attack_element_correct(row);

            // std::set<long long> reinforceTypeIds{};
            // for (auto &&weapon_json : weapons_json)
            //     reinforceTypeIds.insert(weapon_json.at("reinforceTypeId").get<long long>());
            // json reinforce_types_json{};
            // for (auto &&[reinforceParamId, reinforceParamWeapon] : this->reinforceParamWeapons)
            // {
            //     auto reinforceLevel = reinforceParamId % 50;
            //     auto reinforceTypeId = reinforceParamId - reinforceLevel;
            //     auto reinforceTypeId_string = std::to_string(reinforceTypeId);

            //     if (reinforceTypeIds.contains(reinforceTypeId))
            //     {
            //         if (!reinforce_types_json.contains(reinforceTypeId_string))
            //             reinforce_types_json[reinforceTypeId_string] = json::array();

            //         auto &&reinforceTypeJson = reinforce_types_json[reinforceTypeId_string];
            //         if (reinforceTypeJson.size() == reinforceLevel)
            //             reinforceTypeJson.push_back(parse_reinforce_param_weapon(reinforceParamWeapon));
            //     }
            // }

            // std::set<long long> statusSpEffectParamIds{};
            // for (auto &&weapon_json : weapons_json)
            // {
            //     auto reinforceTypeId_string = std::to_string(weapon_json.at("reinforceTypeId").get<long long>());
            //     auto &&reinforceParamWeapons = reinforce_types_json.at(reinforceTypeId_string);
            //     for (auto &&reinforceParamWeapon : reinforceParamWeapons)
            //     {
            //         if (weapon_json.contains("statusSpEffectParamIds"))
            //         {
            //             int i = 1;
            //             for (auto &&spEffectParamId : weapon_json.at("statusSpEffectParamIds").get<std::array<long long, 3>>())
            //             {
            //                 if (spEffectParamId)
            //                 {
            //                     auto statusSpEffectId_string = "statusSpEffectId" + std::to_string(i);
            //                     auto offset = reinforceParamWeapon.value(statusSpEffectId_string, 0ll);
            //                     statusSpEffectParamIds.insert(spEffectParamId + offset);
            //                 }
            //                 ++i;
            //             }
            //         }
            //     }
            // }
            // json status_sp_effect_params_json{};
            // for (auto &&[spEffectParamId, _] : this->spEffectParams)
            // {
            //     if (statusSpEffectParamIds.contains(spEffectParamId))
            //     {
            //         auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId);
            //         std::erase_if(status_sp_effect_params, [](auto &&v) { return v.second == 0; });
            //         status_sp_effect_params_json[std::to_string(spEffectParamId)] = status_sp_effect_params;
            //     }
            // }

            // json scaling_tiers_tson = json::array();
            // for (auto &&[id, row] : this->menuValueTableParams)
            //     if (row.at("compareType") == 1 && id >= 100)
            //         scaling_tiers_tson.push_back(json::array({row.at("value") / 100., this->menuText.at(row.at("textId"))}));

            // json regulation_data_json{{"calcCorrectGraphs", calc_correct_graphs_json}, {"attackElementCorrects", attack_element_corrects_json}, {"reinforceTypes", reinforce_types_json}, {"statusSpEffectParams", status_sp_effect_params_json}, {"scalingTiers", scaling_tiers_tson}, {"weapons", weapons_json}};

            // std::println("\nsuccessfully generated regulation data file");

            // return regulation_data_json;
        }
    };

    
}