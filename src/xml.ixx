module;
#include <pugixml.hpp>
#include <nlohmann/json.hpp>
export module erdo:xml;
import :witchy;
import :calculator;

import std;

namespace xml
{
    using ParamRow = std::map<std::string, double>;

    const std::map<long long, calculator::Weapon::Type> weapon_type_overrides = {{110000, calculator::Weapon::Type::FIST}};
    constexpr bool isVanilla = true;
    constexpr long long default_damage_calc_correct_graph_id = 0;
    constexpr long long default_status_calc_correct_graph_id = 6;

    template<typename T>
        requires (std::same_as<T, double> || std::same_as<T, long long>)
    std::map<long long, std::map<std::string, T>> read_param_xml(const std::filesystem::path &file_path)
    {
        auto get_as = [](const pugi::xml_attribute& attr, T def = 0){
            if constexpr (std::same_as<T, double>)
                return attr.as_double(def);
            else if constexpr (std::same_as<T, long long>)
                return attr.as_llong(def);
        };

        pugi::xml_document data;
        auto result = data.load_file(file_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
        if (!result)
            throw std::runtime_error(std::format("could not load xml file: {}", result.description()));

        auto field_nodes = data.child("param").child("fields").children("field");

        std::map<std::string, T> default_values{};
        for (auto &&field_node : field_nodes)
        {
            auto name = field_node.attribute("name").as_string();
            auto defaultValue = get_as(field_node.attribute("defaultValue"), std::numeric_limits<T>::max());

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
                row_data[row_attribute.name()] = get_as(row_attribute);
            }

            auto id = row_node.attribute("id").as_llong();
            ret.emplace(id, std::move(row_data));
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
            auto id = text_node.attribute("id").as_llong();
            auto text = text_node.child_value();
            ret.emplace(id, text);
        }

        return ret;
    }

    std::string attribute_to_xml_string(calculator::Attribute attr) {
        if (attr == calculator::Attribute::STRENGTH)
            return "Strength";
        if (attr == calculator::Attribute::DEXTERITY)
            return "Agility";
        if (attr == calculator::Attribute::INTELLIGENCE)
            return "Magic";
        if (attr == calculator::Attribute::FAITH)
            return "Faith";
        if (attr == calculator::Attribute::ARCAINE)
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

    struct WeaponContainer : calculator::WeaponContainer
    {
        using WeaponDict = json;

        template<typename T>
        static bool is_unique_weapon(const std::map<std::string, T> &row)
        {
            return row.at("gemMountType") == 0 || row.at("disableGemAttr") == 1;
        }

        std::map<long long, std::map<std::string, double>> attackElementCorrectParams;
        std::map<long long, std::map<std::string, double>> calcCorrectGraphs;
        std::map<long long, std::map<std::string, double>> equipParamWeapons;
        std::map<long long, std::map<std::string, double>> reinforceParamWeapons;
        std::map<long long, std::map<std::string, long long>> spEffectParams;
        std::map<long long, std::map<std::string, double>> menuValueTableParams;
        std::map<long long, std::string> menuText;
        std::map<long long, std::string> weaponNames;
        std::map<long long, std::string> dlcWeaponNames;

        std::map<calculator::AttackPowerType, long long> parse_status_sp_effect_params(long long statusSpEffectParamId) const
        {
            if (!this->spEffectParams.contains(statusSpEffectParamId))
                return {};
            auto &&spEffectRow = this->spEffectParams.at(statusSpEffectParamId);

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
        calculator::CalcCorrectGraph parse_calc_correct_graph(const ParamRow &row) const
        {
            return calculator::CalcCorrectGraph{
                calculator::CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal0")), row.at("stageMaxGrowVal0") / 100., row.at("adjPt_maxGrowVal0")},
                calculator::CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal1")), row.at("stageMaxGrowVal1") / 100., row.at("adjPt_maxGrowVal1")},
                calculator::CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal2")), row.at("stageMaxGrowVal2") / 100., row.at("adjPt_maxGrowVal2")},
                calculator::CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal3")), row.at("stageMaxGrowVal3") / 100., row.at("adjPt_maxGrowVal3")},
                calculator::CalcCorrectGraphEntry{assert_floating_is<int>(row.at("stageMaxVal4")), row.at("stageMaxGrowVal4") / 100., row.at("adjPt_maxGrowVal4")},
            };
        }
        json parse_attack_element_correct(const ParamRow &row) const
        {
            using namespace calculator;

            auto buildAttackElementCorrect = [](std::vector<std::tuple<Attribute, bool, double>> v) {
                json entries = json::object();
                for (auto &&elem : v)
                {
                    auto &&[attribute, isCorrect, overwriteCorrect] = elem;
                    if (isCorrect)
                    {
                        if (overwriteCorrect == -1)
                            entries.emplace(
                                enum_to_string(attribute), true);
                        else
                            entries.emplace(enum_to_string(attribute), overwriteCorrect / 100.);
                    }
                }
                return entries; };

            return json{
                {std::to_string(std::to_underlying(AttackPowerType::PHYSICAL)), buildAttackElementCorrect({
                    {Attribute::STRENGTH, row.at("isStrengthCorrect_byPhysics"), row.at("overwriteStrengthCorrectRate_byPhysics")},
                    {Attribute::DEXTERITY, row.at("isDexterityCorrect_byPhysics"), row.at("overwriteDexterityCorrectRate_byPhysics")},
                    {Attribute::FAITH, row.at("isFaithCorrect_byPhysics"), row.at("overwriteFaithCorrectRate_byPhysics")},
                    {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byPhysics"), row.at("overwriteMagicCorrectRate_byPhysics")},
                    {Attribute::ARCAINE, row.at("isLuckCorrect_byPhysics"), row.at("overwriteLuckCorrectRate_byPhysics")}}
                )},
                {std::to_string(std::to_underlying(AttackPowerType::MAGIC)), buildAttackElementCorrect({
                    {Attribute::STRENGTH, row.at("isStrengthCorrect_byMagic"), row.at("overwriteStrengthCorrectRate_byMagic")},
                    {Attribute::DEXTERITY, row.at("isDexterityCorrect_byMagic"), row.at("overwriteDexterityCorrectRate_byMagic")},
                    {Attribute::FAITH, row.at("isFaithCorrect_byMagic"), row.at("overwriteFaithCorrectRate_byMagic")},
                    {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byMagic"), row.at("overwriteMagicCorrectRate_byMagic")},
                    {Attribute::ARCAINE, row.at("isLuckCorrect_byMagic"), row.at("overwriteLuckCorrectRate_byMagic")}}
                )},
                {std::to_string(std::to_underlying(AttackPowerType::FIRE)), buildAttackElementCorrect({
                    {Attribute::STRENGTH, row.at("isStrengthCorrect_byFire"), row.at("overwriteStrengthCorrectRate_byFire")},
                    {Attribute::DEXTERITY, row.at("isDexterityCorrect_byFire"), row.at("overwriteDexterityCorrectRate_byFire")},
                    {Attribute::FAITH, row.at("isFaithCorrect_byFire"), row.at("overwriteFaithCorrectRate_byFire")},
                    {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byFire"), row.at("overwriteMagicCorrectRate_byFire")},
                    {Attribute::ARCAINE, row.at("isLuckCorrect_byFire"), row.at("overwriteLuckCorrectRate_byFire")}}
                )},
                {std::to_string(std::to_underlying(AttackPowerType::LIGHTNING)), buildAttackElementCorrect({
                    {Attribute::STRENGTH, row.at("isStrengthCorrect_byThunder"), row.at("overwriteStrengthCorrectRate_byThunder")},
                    {Attribute::DEXTERITY, row.at("isDexterityCorrect_byThunder"), row.at("overwriteDexterityCorrectRate_byThunder")},
                    {Attribute::FAITH, row.at("isFaithCorrect_byThunder"), row.at("overwriteFaithCorrectRate_byThunder")},
                    {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byThunder"), row.at("overwriteMagicCorrectRate_byThunder")},
                    {Attribute::ARCAINE, row.at("isLuckCorrect_byThunder"), row.at("overwriteLuckCorrectRate_byThunder")}}
                )},
                {std::to_string(std::to_underlying(AttackPowerType::HOLY)), buildAttackElementCorrect({
                    {Attribute::STRENGTH, row.at("isStrengthCorrect_byDark"), row.at("overwriteStrengthCorrectRate_byDark")},
                    {Attribute::DEXTERITY, row.at("isDexterityCorrect_byDark"), row.at("overwriteDexterityCorrectRate_byDark")},
                    {Attribute::FAITH, row.at("isFaithCorrect_byDark"), row.at("overwriteFaithCorrectRate_byDark")},
                    {Attribute::INTELLIGENCE, row.at("isMagicCorrect_byDark"), row.at("overwriteMagicCorrectRate_byDark")},
                    {Attribute::ARCAINE, row.at("isLuckCorrect_byDark"), row.at("overwriteLuckCorrectRate_byDark")}}
                )}
            };
        }
        calculator::ReinforceTypesDict parse_reinforce_param_weapon(const ParamRow &row) const
        {
            calculator::ReinforceTypesDict ret{};
            for (auto damage_type : enumerators_of<calculator::DamageType>())
            {
                auto apt = integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type));

                auto atk_rate_str = attack_power_type_to_xml_string(apt);
                std::ranges::transform(atk_rate_str, atk_rate_str.begin(),
                   [](unsigned char c){ return std::tolower(c); }
                  );

                ret.attack.at(std::to_underlying(damage_type)) = row.at(std::format("{}AtkRate", atk_rate_str));
            }
            for (auto attribute : enumerators_of<calculator::Attribute>())
            {
                auto rate_str = std::format("correct{}Rate", attribute_to_xml_string(attribute));
                ret.attributeScaling.at(std::to_underlying(attribute)) = row.at(rate_str);
            }
            for (size_t i = 0; i < 3; ++i)
            {
                auto s = std::format("spEffectId{}", i+1);
                if (row.contains(s))
                    if (row.at(s) != 0)
                        ret.statusSpEffectId.at(i) = row.at(s);
            }
            return ret;
        }

        json parse_weapon(const std::map<std::string, double> &row) const
        {
            using namespace calculator;

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
                std::println("ignoring: could not find weapon name for id: {}", row_id);
                return {};
            }

            if (name.find("[ERROR]") != std::string::npos || name.find("%null%") != std::string::npos)
            {
                std::println("ignoring: weapon name: {}, id: {}", name, row_id);
                return {};
            }

            const auto weaponType = weapon_type_overrides.contains(row_id) ? std::to_underlying(weapon_type_overrides.at(row_id)) : assert_floating_is<long long>(row.at("wepType"));
            if (!is_valid_enum_integral<calculator::Weapon::Type>(weaponType))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weaponType))
                    std::println("ignoring: weapon {} because no real weapon", name);
                else
                {
                    std::println("ignoring: Unknown weapon type {} for weapon {}", weaponType, name);
                    throw std::runtime_error("unknown weapon type");
                }
                return {};
            }

            if (!this->reinforceParamWeapons.contains(row.at("reinforceTypeId")))
                throw std::runtime_error(std::format("could not find reinforce param weapon for reinforceTypeId: {}", std::to_string(row.at("reinforceTypeId"))));

            if (!this->attackElementCorrectParams.contains(row.at("attackElementCorrectId")))
                throw std::runtime_error(std::format("could not find attack element correct param for attackElementCorrectId: {}", std::to_string(row.at("attackElementCorrectId"))));

            const auto affinityId = assert_floating_is<long long>((row_id % 10000) / 100.);

            const auto equipParamWeaponsId = row_id - 100 * affinityId;
            if (!this->equipParamWeapons.contains(equipParamWeaponsId))
                throw std::runtime_error(std::format("could not find equip param weapon for id: {}", std::to_string(equipParamWeaponsId)));
            const auto &uninfusedWeapon = this->equipParamWeapons.at(equipParamWeaponsId);

            if (affinityId != 0 && is_unique_weapon(uninfusedWeapon))
                throw std::runtime_error("unique weapon cannot have an affinity");

            std::set<calculator::AttackPowerType> attackPowerTypes{};
            std::vector<long long> statusSpEffectParamIds{};
            for (size_t i = 0; i < 3; ++i)
            {
                auto spEffectParamId = assert_floating_is<long long>(row.at(std::format("spEffectBehaviorId{}", i)));
                auto statusSpEffectParams = parse_status_sp_effect_params(spEffectParamId);
                if (!statusSpEffectParams.empty())
                {
                    for (auto &&[k, v] : statusSpEffectParams)
                        attackPowerTypes.insert(k);

                    statusSpEffectParamIds.emplace_back(spEffectParamId);
                }
                else
                    statusSpEffectParamIds.emplace_back(0);
            }

            if (std::ranges::all_of(statusSpEffectParamIds, [](long long id) { return id == 0; }))
                statusSpEffectParamIds.clear();

            if (isVanilla && row_id == 32131200)
                statusSpEffectParamIds = {0, 0, 0};

            std::vector<std::pair<AttackPowerType, double>> attack{};
            for (auto damage_type : enumerators_of<DamageType>())
            {
                auto apt = integral_to_enum<AttackPowerType>(std::to_underlying(damage_type));
                auto attack_power = row.at(std::format("attackBase{}", attack_power_type_to_xml_string(apt)));

                if (attack_power != 0)
                {
                    attackPowerTypes.insert(apt);
                    attack.emplace_back(apt, attack_power);
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
                    if (!std::set{ AttackPowerType::SCARLET_ROT, AttackPowerType::FROST, AttackPowerType::DEATH_BLIGHT }.contains(apt))
                    {
                        auto xml_str = std::format("correctType_{}", attack_power_type_to_xml_string(apt));
                        auto def = is_valid_enum_integral<DamageType>(std::to_underlying(apt)) ? default_damage_calc_correct_graph_id : default_status_calc_correct_graph_id;
                        if (row.contains(xml_str))
                        {
                            auto calcCorrectGraphId = assert_floating_is<long long>(row.at(xml_str));

                            if (!this->calcCorrectGraphs.contains(calcCorrectGraphId))
                                throw std::runtime_error(std::format("could not find calc correct graph for id: {}", calcCorrectGraphId));

                            calcCorrectGraphIds[apt] = calcCorrectGraphId;
                        } 
                    }
                }
            }

            std::vector<std::pair<Attribute, double>> attributeScaling{};
            for (auto attribute : enumerators_of<Attribute>())
            {
                auto xml_str = std::format("correct{}", attribute_to_xml_string(attribute));
                if (row.at(xml_str))
                    attributeScaling.emplace_back(attribute, row.at(xml_str) / 100.);
            }

            json ret{};
            ret["name"] = name;
            ret["weaponName"] = (weaponNames.contains(uninfusedWeapon.at("id")) ? weaponNames.at(uninfusedWeapon.at("id")) : dlcWeaponNames.at(uninfusedWeapon.at("id")));
            // ret["url"] = "";
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

            std::println("weapon: {}, type: {}", name, enum_to_string(integral_to_enum<Weapon::Type>(weaponType)));
            return ret;
        }

        static calculator::ScalingCurve evaluate_CalcCorrectGraph(const calculator::CalcCorrectGraph &calcCorrectGraph)
        {
            calculator::ScalingCurve arr{};

            for (size_t i = 1; i < calcCorrectGraph.size(); i++)
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

        WeaponContainer(const std::filesystem::path &xml_data_directory) : 
            attackElementCorrectParams(read_param_xml<double>(xml_data_directory / witchy::AttackElementCorrectParamFile += ".xml")),
            calcCorrectGraphs(read_param_xml<double>(xml_data_directory / witchy::CalcCorrectGraphFile += ".xml")),
            equipParamWeapons(read_param_xml<double>(xml_data_directory / witchy::EquipParamWeaponFile += ".xml")),
            reinforceParamWeapons(read_param_xml<double>(xml_data_directory / witchy::ReinforceParamWeaponFile += ".xml")),
            spEffectParams(read_param_xml<long long>(xml_data_directory / witchy::SpEffectParamFile += ".xml")),
            menuValueTableParams(read_param_xml<double>(xml_data_directory / witchy::MenuValueTableParamFile += ".xml")),
            weaponNames(read_fmg_xml(xml_data_directory / witchy::WeaponNameFile += ".xml")),
            dlcWeaponNames(read_fmg_xml(xml_data_directory / witchy::WeaponName_dlc01File += ".xml")),
            menuText(read_fmg_xml(xml_data_directory / witchy::GR_MenuTextFile += ".xml"))
        {
            std::map<long long, std::vector<calculator::ReinforceTypesDict>> reinforce_types;
            for (auto &&[reinforce_param_id, reinforce_param_weapon] : this->reinforceParamWeapons)
            {
                auto reinforce_level = reinforce_param_id % 50;
                auto reinforce_type_id = reinforce_param_id - reinforce_level;
                auto &&reinforce_type = reinforce_types.try_emplace(reinforce_type_id).first->second;
                if (reinforce_type.size() == reinforce_level)
                    reinforce_type.push_back(parse_reinforce_param_weapon(reinforce_param_weapon));
            }

            size_t  i = 0;
            for (auto &&[id, row] : this->menuValueTableParams)
                if (row.at("compareType") == 1 && id >= 100)
                    this->scalingTiers.at(i++) = {row.at("value") / 100., this->menuText.at(row.at("textId"))};

            std::map<int, std::map<calculator::AttackPowerType, long long>> statusSpEffectParams{};
            for (auto &&[spEffectParamId, _] : this->spEffectParams)
            {
                auto status_sp_effect_params = parse_status_sp_effect_params(spEffectParamId);
                std::erase_if(status_sp_effect_params, [](auto &&v) { return v.second == 0; });
                statusSpEffectParams.try_emplace(spEffectParamId, status_sp_effect_params);
            }
                
            for (auto &&[id, row] : this->attackElementCorrectParams)
            {
                auto attackElementCorrect = parse_attack_element_correct(row);

                auto &&[inserted, success] = this->attackElementCorrectsById.emplace(id, attackElementCorrect);
                constexpr auto default_ = calculator::AttributeScaling{false, false, false, false, true}; // default value
                inserted->second[std::to_underlying(calculator::AttackPowerType::POISON)] = default_;
                inserted->second[std::to_underlying(calculator::AttackPowerType::BLEED)] = default_;
                inserted->second[std::to_underlying(calculator::AttackPowerType::MADNESS)] = default_;
                inserted->second[std::to_underlying(calculator::AttackPowerType::SLEEP)] = default_;
            }


            this->weapons.reserve(this->equipParamWeapons.size());
            json weapons_json = json::array();
            std::set<long long> calc_correct_graph_ids{default_damage_calc_correct_graph_id, default_status_calc_correct_graph_id};
            std::set<long long> reinforceTypeIds{};
            std::set<long long> attackElementCorrectIds{};
            std::set<long long> statusSpEffectParamIds{};
            for (auto &&[k, param_row] : this->equipParamWeapons)
            {
                auto weapon_json = this->parse_weapon(param_row);
                if (!weapon_json.empty())
                {
                    weapons_json.push_back(weapon_json);

                    for (auto &&[apt, calcCorrectGraphId] : weapon_json.at("calcCorrectGraphIds").items())
                    {
                        auto calc_correct_graph_id = calcCorrectGraphId.get<long long>();    
                        calc_correct_graph_ids.insert(calc_correct_graph_id);
                        this->calcCorrectGraphsById.emplace(
                            calc_correct_graph_id,
                            evaluate_CalcCorrectGraph(
                                parse_calc_correct_graph(
                                    this->calcCorrectGraphs.at(calc_correct_graph_id)
                                )
                            )
                        );
                    }

                    attackElementCorrectIds.insert(weapon_json.at("attackElementCorrectId").get<long long>());  
                    auto reinforceTypeId = weapon_json.at("reinforceTypeId").get<long long>();
                    reinforceTypeIds.insert(reinforceTypeId);  

                    for (auto &&reinforceParamWeapon : reinforce_types.at(reinforceTypeId))
                    {
                        if (weapon_json.contains("statusSpEffectParamIds"))
                        {
                            int i = 1;
                            for (auto &&spEffectParamId : weapon_json.at("statusSpEffectParamIds").get<std::array<long long, 3>>())
                            {
                                if (spEffectParamId)
                                {
                                    auto offset = reinforceParamWeapon.statusSpEffectId.at(i-1);
                                    statusSpEffectParamIds.insert(spEffectParamId + offset);
                                }
                                ++i;
                            }
                        }
                    }






                }
            }


            for (const auto &weapon_json : weapons_json)
            {
                    auto &&attackElementCorrect = this->attackElementCorrectsById.at(weapon_json.at("attackElementCorrectId").get<int>());

                    const auto &reinforceParams = reinforce_types.at(weapon_json.at("reinforceTypeId").get<int>());

                    auto calcCorrectGraphIds = weapon_json.at("calcCorrectGraphIds").get<std::map<calculator::AttackPowerType, int>>();
                    std::array<calculator::ScalingCurve, std::meta::enumerators_of(^^calculator::AttackPowerType).size()> weaponCalcCorrectGraphs{};
                    for (auto damage_type : enumerators_of<calculator::DamageType>())
                        weaponCalcCorrectGraphs.at(std::to_underlying(damage_type)) = this->calcCorrectGraphsById.at(map_get(calcCorrectGraphIds,
                            integral_to_enum<calculator::AttackPowerType>(std::to_underlying(damage_type)),
                            default_damage_calc_correct_graph_id)
                        );
                    for (auto status_type : enumerators_of<calculator::StatusType>())
                        weaponCalcCorrectGraphs.at(std::to_underlying(status_type)) = this->calcCorrectGraphsById.at(map_get(calcCorrectGraphIds,
                            integral_to_enum<calculator::AttackPowerType>(std::to_underlying(status_type)),
                            default_status_calc_correct_graph_id)
                        );

                    auto unupgradedAttack = weapon_json.at("attack").get<std::vector<std::pair<calculator::AttackPowerType, int>>>();
                    auto statusSpEffectParamIds = weapon_json.value("statusSpEffectParamIds", std::array<int, 3>{});
                    std::vector<std::array<double, std::meta::enumerators_of(^^calculator::AttackPowerType).size()>> attack{};
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

                    auto unupgradedAttributeScaling = weapon_json.at("attributeScaling").get<std::vector<std::pair<calculator::Attribute, double>>>();
                    std::vector<calculator::AttributeScaling> attributeScaling{};
                    for (const auto &reinforceParam : reinforceParams)
                    {
                        auto &foo = attributeScaling.emplace_back();
                        for (const auto &[attribute, unupgradedScaling] : unupgradedAttributeScaling)
                            foo.at(std::to_underlying(attribute)) =
                                unupgradedScaling *
                                reinforceParam.attributeScaling.at(std::to_underlying(attribute));
                    }

                    std::string weaponName = weapon_json.at("weaponName").get<std::string>();
                    auto url_part = weaponName;
                    std::ranges::replace(url_part, ' ', '_');

                    this->weapons.emplace_back(calculator::Weapon{
                        .full_name = weapon_json.at("name").get<std::string>(),
                        .base_name = std::move(weaponName),
                        .url = weapon_json.value("url", "https://eldenring.fandom.com/wiki/" + url_part),
                        .dlc = weapon_json.at("dlc").get<bool>(),
                        .paired = weapon_json.value("paired", false),
                        .sorcery_tool = weapon_json.value("sorceryTool", false),
                        .incantation_tool = weapon_json.value("incantationTool", false),
                        .type = weapon_json.at("weaponType").get<calculator::Weapon::Type>(),
                        .affinity = weapon_json.at("affinityId").get<calculator::Weapon::Affinity>(),
                        .requirements = weapon_json.at("requirements").get<calculator::Stats>(),
                        .attribute_scaling = std::move(attributeScaling),
                        .base_attack_power = std::move(attack),
                        .attack_power_attribute_scaling = std::move(attackElementCorrect),
                        .attack_power_scaling_curves = std::move(weaponCalcCorrectGraphs),
                        .scaling_tiers = this->scalingTiers
                    });
                // }
            }

            std::println("{} weapons\n", this->weapons.size());
        }
    };
}