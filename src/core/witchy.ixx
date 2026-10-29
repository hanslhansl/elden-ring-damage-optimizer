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

export namespace erdo::witchy
{
    const std::set<std::filesystem::path> needed_packed_files = {
        "regulation.bin",
        "msg/engus/menu.msgbnd.dcx",
        "msg/engus/menu_dlc01.msgbnd.dcx",
        "msg/engus/menu_dlc02.msgbnd.dcx",
        "msg/engus/item.msgbnd.dcx",
        "msg/engus/item_dlc01.msgbnd.dcx",
        "msg/engus/item_dlc02.msgbnd.dcx",

        "chr/c0000.anibnd.dcx",
    };

    const std::filesystem::path WitchyBnd4File = "regulation-bin/_witchy-bnd4.xml";
    const std::filesystem::path AttackElementCorrectParamFile = "regulation-bin/AttackElementCorrectParam.param.xml";
    const std::filesystem::path CalcCorrectGraphFile = "regulation-bin/CalcCorrectGraph.param.xml";
    const std::filesystem::path EquipParamWeaponFile = "regulation-bin/EquipParamWeapon.param.xml";
    const std::filesystem::path ReinforceParamWeaponFile = "regulation-bin/ReinforceParamWeapon.param.xml";
    const std::filesystem::path SpEffectParamFile = "regulation-bin/SpEffectParam.param.xml";
    const std::filesystem::path MenuValueTableParamFile = "regulation-bin/MenuValueTableParam.param.xml";

    const std::filesystem::path AtkParam_PcFile = "regulation-bin/AtkParam_Pc.param.xml"; //
    const std::filesystem::path BehaviorParam_PcFile = "regulation-bin/BehaviorParam_PC.param.xml";
    // Optional diagnostic dependency; production admission still requires a
    // verified projectile calculation policy before this becomes mandatory.
    const std::filesystem::path BulletParamFile = "regulation-bin/Bullet.param.xml";

    const std::filesystem::path WeaponNameFile = "msg/engus/item-msgbnd-dcx/WeaponName.fmg.xml";
    const std::filesystem::path WeaponName_dlc01File = "msg/engus/item_dlc01-msgbnd-dcx/WeaponName_dlc01.fmg.xml";
    const std::filesystem::path WeaponName_dlc02File = "msg/engus/item_dlc02-msgbnd-dcx/WeaponName_dlc02.fmg.xml";

    const std::filesystem::path GR_MenuTextFile = "msg/engus/menu-msgbnd-dcx/GR_MenuText.fmg.xml";
    const std::filesystem::path GR_MenuText_dlc01File = "msg/engus/menu_dlc01-msgbnd-dcx/GR_MenuText_dlc01.fmg.xml";
    const std::filesystem::path GR_MenuText_dlc02File = "msg/engus/menu_dlc02-msgbnd-dcx/GR_MenuText_dlc02.fmg.xml";

    // just used for checking if the files are present
    const std::vector needed_unpacked_files = {
        WitchyBnd4File,
        AttackElementCorrectParamFile,
        CalcCorrectGraphFile,
        EquipParamWeaponFile,
        ReinforceParamWeaponFile,
        SpEffectParamFile,
        MenuValueTableParamFile,

        AtkParam_PcFile,
        BehaviorParam_PcFile,

        WeaponNameFile,
        WeaponName_dlc01File,
        WeaponName_dlc02File,

        GR_MenuTextFile,
        GR_MenuText_dlc01File,
        GR_MenuText_dlc02File
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

    struct AttackEventSelection {
        int eventType = -1;
        std::optional<int> eventJudgeId;
        std::optional<int> requiredCategory;
        std::optional<int> requiredBehaviorParamId;
    };

    struct MoveDefinition {
        MoveId id;
        std::string_view name;
        int animationId;
        // Context-specific restriction; an unset category never breaks a tie.
        std::optional<int> requiredCategory = std::nullopt;
        // For verified single-behavior moves whose candidates share a category.
        // This never bypasses variation/judge/category/reference validation.
        std::optional<int> requiredBehaviorParamId = std::nullopt;
        // Select a phase of a shared animation (e.g. mounted charge/release).
        // Keep every matching event, its order, and its original timestamps.
        std::optional<int> eventJudgeId = std::nullopt;
        // A behavior-graph selection may use a weapon's special TAE category.
        std::optional<int> motionCategory = std::nullopt;
        // Successful criticals target the throw victim (TAE type 304). Ordinary
        // collision attacks in the same clip are a separate target context.
        std::optional<int> eventType = std::nullopt;
        // Explicit additional event families/phases. Absent preserves the
        // existing direct-event scope and legacy definition JSON. No automatic
        // temporal association or projectile/contact-count inference.
        std::optional<std::vector<AttackEventSelection>> additionalEventSelections;
    };

    // Resolves a complete supported catalog before hit import. All listed
    // weapon rows will share the result; a provider must validate that context.
    using MovesetDefinitionResolver = std::function<std::vector<MoveDefinition>(
        std::span<const long long>, std::span<const MoveDefinition>)>;

    struct MovesetImportReport;
    std::map<long long, std::shared_ptr<Moveset>> importWeaponMovesets(
        const std::filesystem::path& root,
        const std::map<long long, std::map<std::string, double>>& weapons,
        std::vector<MovesetImportReport>* reports,
        const MovesetDefinitionResolver& resolveDefinitions = {});

    GameData load_game_data(const std::filesystem::path &xml_data_directory,
        std::vector<MovesetImportReport>* movesetReports = nullptr,
        const MovesetDefinitionResolver& resolveDefinitions = {})
    {
        // get game version from /regulation-bin/_witchy-bnd4.xml
        const auto game_version = get_element_value<long long>(
            load_file(xml_data_directory / WitchyBnd4File),
            { "bnd4", "version" }
        );
        std::println("found game version {}", game_version);

        const auto scaling_tiers = get_scaling_tiers(
            xml_data_directory / GR_MenuTextFile,
            xml_data_directory / MenuValueTableParamFile
        );

        const auto attackElementCorrectsById = get_attack_element_corrects_by_id(xml_data_directory / AttackElementCorrectParamFile);

        const auto spEffectParams = read_param_file<long long>(xml_data_directory / SpEffectParamFile);
        const auto calcCorrectGraphs = read_param_file<double>(xml_data_directory / CalcCorrectGraphFile);
        const auto equipParamWeapons = read_param_file<double>(xml_data_directory / EquipParamWeaponFile);
        const auto reinforceParamWeapons = read_param_file<double>(xml_data_directory / ReinforceParamWeaponFile);

        const auto base_game_weapon_names = read_fmg_file(xml_data_directory / WeaponNameFile);
        const auto dlc1_weapon_names = read_fmg_file(xml_data_directory / WeaponName_dlc01File);
        const auto dlc2_weapon_names = read_fmg_file(xml_data_directory / WeaponName_dlc02File);
        auto get_name_and_dlc = [&](long long id) -> std::optional<std::pair<std::string, int>> {
            if (dlc2_weapon_names.contains(id))
                return {{dlc2_weapon_names.at(id), 2}};
            if (dlc1_weapon_names.contains(id))
                return {{dlc1_weapon_names.at(id), 1}};
            if (base_game_weapon_names.contains(id))
                return {{base_game_weapon_names.at(id), 0}};
            return std::nullopt;
        };


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
        std::vector<long long> weaponRowIds;
        for (auto &&[k, row] : equipParamWeapons)
        {
            auto row_id = assert_float_is_llong(row.at("id"));

            auto name_and_dlc = get_name_and_dlc(row_id);
            if (!name_and_dlc)
                continue;
            auto [full_name, dlc] = *name_and_dlc;
            if (full_name.contains("[ERROR]") || full_name.contains("%null%"))
                continue;

            const auto weaponType = weapon_type_overrides.contains(row_id)
                ? std::to_underlying(weapon_type_overrides.at(row_id))
                : assert_float_is_llong(row.at("wepType"));
            if (!is_valid_enum_integral<Weapon::Type>(weaponType))
            {
                if (std::set{0, 81, 83, 85, 86}.contains(weaponType))
                    continue;

                throw std::runtime_error(std::format("unknown weapon type {} for weapon {}", weaponType, full_name));
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

            RelevantAttributeLevelsArray required_relevant_stats{};
            for (auto attribute : enumerators_of<RelevantAttribute>())
                required_relevant_stats.at(std::to_underlying(attribute)) = assert_float_is_llong(row.at(std::format("proper{}", attribute_to_xml_string(attribute))));

            Weapon w{
                full_name,
                get_name_and_dlc(uninfusedWeapon.at("id")).value().first,
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

            weaponRowIds.push_back(row_id);
            game_data.weapons.emplace_back(std::move(w));
        }

        // Only exported weapons participate. Unnamed/debug parameter rows must
        // not create unused movesets or invalidate a complete GameData import.
        std::map<long long, std::map<std::string, double>> exportedWeaponRows;
        for (const auto id : weaponRowIds) exportedWeaponRows.emplace(id, equipParamWeapons.at(id));
        const auto weaponMovesets = importWeaponMovesets(xml_data_directory, exportedWeaponRows, movesetReports, resolveDefinitions);
        for (std::size_t i = 0; i < weaponRowIds.size(); ++i)
            if (const auto moveset = weaponMovesets.find(weaponRowIds[i]); moveset != weaponMovesets.end())
                game_data.weapons[i].moveset = moveset->second;
        std::ranges::sort(game_data.weapons, {}, &Weapon::full_name);
        std::println("found {} weapons", game_data.weapons.size());
        return game_data;
    }



    using pugi::xml_document;

    using calculator::MotionValues;
    using calculator::AttackEvent;
    using calculator::ResolvedHit;
    using calculator::MoveId;

    // Owned provenance: reports remain valid after the XML tables are destroyed.
    struct BehaviorCandidate {
        int behaviorParamId = -1;
        std::optional<int> category;
        std::optional<int> refType;
        std::optional<int> refId;
    };

    struct UnresolvedAttack {
        AttackEvent event;
        int behaviorVariationId;
        std::optional<int> requiredCategory;
        std::vector<BehaviorCandidate> candidates;
        std::optional<int> requiredBehaviorParamId;
    };

    struct ResolutionResult {
        std::vector<ResolvedHit> hits;
        std::vector<std::string> diagnostics;
        std::vector<UnresolvedAttack> unresolved;
    };

    std::string_view xmlValue(
        pugi::xml_node node,
        const char* name
    ) {
        if (auto attribute = node.attribute(name))
            return attribute.value();

        if (auto child = node.child(name))
            return child.text().get();

        return {};
    }

    int parseInt(std::string_view value) {
        int result{};
        const auto [ptr, ec] =
            std::from_chars(value.data(), value.data() + value.size(), result);

        if (ec != std::errc{} || ptr != value.data() + value.size())
            throw std::runtime_error("Invalid integer: " + std::string(value));

        return result;
    }

    std::optional<int> xmlInt(
        pugi::xml_node node,
        const char* name
    ) {
        const auto value = xmlValue(node, name);

        if (value.empty())
            return std::nullopt;

        return parseInt(value);
    }

    std::optional<int> optionalInt(pugi::xml_attribute attribute) {
        if (!attribute || !*attribute.value())
            return std::nullopt;

        return parseInt(attribute.value());
    }

    struct ParamTable {
        pugi::xml_document document;

        std::unordered_map<int, pugi::xml_node> rows;
        std::unordered_map<std::string, std::string> defaults;

        void load(const char* filename) {
            rows.clear();
            defaults.clear();
            const auto result = document.load_file(filename);

            if (!result)
                throw std::runtime_error(
                    std::string("Could not load ") + filename);

            indexDocument();
        }

        void loadXml(const char* xml) {
            rows.clear();
            defaults.clear();
            if (const auto result = document.load_string(xml); !result)
                throw std::runtime_error(result.description());
            indexDocument();
        }

        void indexDocument() {
            rows.clear();
            defaults.clear();

            // Witchy stores row values as attributes.
            for (auto row : document.select_nodes("//row")) {
                const auto node = row.node();
                const auto id = optionalInt(node.attribute("id"));

                if (id)
                    rows.emplace(*id, node);
            }

            for (auto field : document.select_nodes("//field")) {
                const auto node = field.node();
                const auto name = node.attribute("name");
                const auto value = node.attribute("defaultValue");

                if (name && value)
                    defaults.emplace(name.value(), value.value());
            }
        }

        pugi::xml_node find(int id) const {
            const auto it = rows.find(id);
            return it == rows.end() ? pugi::xml_node{} : it->second;
        }

        std::optional<int> integer(pugi::xml_node row, const char* field) const {
            if (!row)
                return std::nullopt;
            if (const auto attribute = row.attribute(field))
                return parseInt(attribute.value());
            const auto it = defaults.find(field);
            return it == defaults.end() ? std::nullopt : std::optional{parseInt(it->second)};
        }

        std::optional<float> number(
            pugi::xml_node row,
            const char* field
        ) const {
            if (!row)
                return std::nullopt;

            const auto attribute = row.attribute(field);

            const auto it = defaults.find(field);
            if (!attribute && it == defaults.end())
                return std::nullopt;

            const std::string_view value = attribute
                ? std::string_view(attribute.value()) : std::string_view(it->second);
            float parsed{};
            const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (ec != std::errc{} || end != value.data() + value.size() || !std::isfinite(parsed))
                throw std::runtime_error(std::format("Invalid numeric field {}: {}", field, value));
            return parsed;
        }
    };

    struct BehaviorKey {
        int variationId;
        int judgeId;

        bool operator==(const BehaviorKey&) const = default;
    };

    struct BehaviorKeyHash {
        std::size_t operator()(const BehaviorKey& key) const {
            const auto a = static_cast<std::uint32_t>(key.variationId);
            const auto b = static_cast<std::uint32_t>(key.judgeId);

            return (static_cast<std::uint64_t>(a) << 32) | b;
        }
    };

    class BehaviorIndex {
    public:
        void build(const ParamTable& behaviors) {
            byKey_.clear();
            byId_ = behaviors.rows;
            defaults_ = behaviors.defaults;
            for (const auto& [id, row] : behaviors.rows) {
                const auto variation =
                    behaviors.integer(row, "variationId");
                const auto judge =
                    behaviors.integer(row, "behaviorJudgeId");

                if (variation && judge)
                    byKey_[{*variation, *judge}].push_back(row);
            }
            for (auto& [key, rows] : byKey_)
                std::ranges::sort(rows, {}, [](auto row) { return parseInt(row.attribute("id").value()); });
        }

        std::optional<int> integer(pugi::xml_node row, const char* field) const {
            if (const auto attribute = row.attribute(field))
                return parseInt(attribute.value());
            const auto it = defaults_.find(field);
            return it == defaults_.end() ? std::nullopt : std::optional{parseInt(it->second)};
        }

        const std::vector<pugi::xml_node>& find(
            int variation,
            int judge
        ) const {
            const auto it = byKey_.find({variation, judge});
            return it == byKey_.end() ? empty_ : it->second;
        }

        // Player weapon behavior lookup. Ordinary judges use namespace 1;
        // judges 1000..9999 encode the namespace in their thousands digit.
        // Follow DSAnimStudio's ER lookup: weapon variation, its hundred
        // group, then common variation 0. First existing row wins; an invalid
        // reference/category at that row is an error, not a reason to fall back.
        // Source and equipped-weapon selection remain the caller's context.
        std::vector<pugi::xml_node> findPlayerWeapon(int variation, int judge) const {
            if (variation < 0 || judge < 0 || judge > 9999)
                return {};
            const long long prefix = static_cast<long long>(judge < 1000 ? 1 : judge / 1000) * 100000000;
            for (int candidate : {variation, variation / 100 * 100, 0}) {
                const long long id = prefix + static_cast<long long>(candidate) * 1000 + judge % 1000;
                if (id > std::numeric_limits<int>::max()) return {};
                const auto found = byId_.find(static_cast<int>(id));
                if (found != byId_.end()) return {found->second};
            }
            return {};
        }

        std::vector<pugi::xml_node> findEncoded(int variation, int judge) const {
            return judge >= 1000 ? findPlayerWeapon(variation, judge) : std::vector<pugi::xml_node>{};
        }

    private:
        std::unordered_map<int, pugi::xml_node> byId_;
        std::unordered_map<std::string, std::string> defaults_;
        std::unordered_map<
            BehaviorKey,
            std::vector<pugi::xml_node>,
            BehaviorKeyHash
        > byKey_;

        std::vector<pugi::xml_node> empty_;
    };

    // Read a TAE event parameter by name.
    // In the Witchy XML inspected so far, parameters are:
    // <param name="BehaviorJudgeID" value="805" />
    std::optional<int> eventParameter(
        pugi::xml_node event,
        const char* name
    ) {
        for (const auto param : event.child("params").children("param")) {
            if (std::string_view(param.attribute("name").value()) == name)
                return optionalInt(param.attribute("value"));
        }

        return std::nullopt;
    }

    std::vector<AttackEvent> extractAttackEvents(
        const pugi::xml_document& tae,
        int animationId,
        bool includePCBehaviors = false,
        bool includeBulletBehaviors = false
    ) {
        std::vector<AttackEvent> events;

        for (const auto event :
            tae.child("anim").child("events").children("event"))
        {
            const auto type = xmlInt(event, "type");

            if (!type || (*type != 1 && *type != 304 && (!includePCBehaviors || *type != 307)
                && (!includeBulletBehaviors || *type != 2)))
                continue;

            const auto judge =
                eventParameter(event, "BehaviorJudgeID");

            if (!judge)
                throw std::runtime_error("TAE attack event is missing BehaviorJudgeID");

            AttackEvent attack;

            attack.animationId = animationId;
            attack.eventType = *type;
            attack.behaviorJudgeId = *judge;

            attack.attackIndex =
                eventParameter(
                    event,
                    *type == 304 ? "Index" : "AttackIndex"
                ).value_or(-1);

            attack.source =
                eventParameter(event, "Source").value_or(-1);

            if (*type == 307 || *type == 2) {
                auto selector = [&](const char* name) {
                    std::optional<int> parsed;
                    for (const auto param : event.child("params").children("param")) {
                        if (std::string_view(param.attribute("name").value()) != name) continue;
                        const std::string_view value = param.attribute("value").value();
                        int number{};
                        const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), number);
                        if (parsed || ec != std::errc{} || end != value.data() + value.size())
                            throw std::runtime_error(std::format("Invalid or duplicate TAE {} value", name));
                        parsed = number;
                    }
                    if (!parsed) throw std::runtime_error(std::format("Missing TAE {}", name));
                    return *parsed;
                };
                if (*type == 307) {
                    attack.pcBehaviorType = selector("PCBehaviorType");
                    attack.pcBehaviorCondition = selector("Condition0x64_GetWeaponData");
                    attack.pcBehaviorOffset = selector("Offset");
                } else {
                    attack.bulletDummyPolyId = selector("DummyPolyID");
                    attack.bulletAttachmentType = selector("AttachmentType");
                    attack.bulletOffset = selector("Offset");
                    attack.bulletUnk12 = selector("Unk12");
                    for (const auto param : event.child("params").children("param")) {
                        if (std::string_view(param.attribute("name").value()) != "Enable") continue;
                        const std::string_view value = param.attribute("value").value();
                        if (attack.bulletEnable || (value != "False" && value != "false" && value != "0"
                            && value != "True" && value != "true" && value != "1"))
                            throw std::runtime_error("Invalid or duplicate TAE Enable value");
                        attack.bulletEnable = value == "True" || value == "true" || value == "1";
                    }
                    if (!attack.bulletEnable) throw std::runtime_error("Missing TAE Enable");
                }
            }

            for (const auto param : event.child("params").children("param")) {
                if (std::string_view(param.attribute("name").value()) != "StateInfo")
                    continue;
                const std::string_view value = param.attribute("value").value();
                int condition{};
                const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), condition);
                if (ec != std::errc{} || end != value.data() + value.size() || attack.stateInfo)
                    throw std::runtime_error("Invalid or duplicate TAE StateInfo value");
                attack.stateInfo = condition;
            }

            const auto start = xmlValue(event, "startTime");
            const auto end   = xmlValue(event, "endTime");

            if (start.empty() || end.empty())
                throw std::runtime_error(
                    "TAE attack event is missing timing data"
                );

            auto parseTime = [](std::string_view value) {
                double parsed{};
                const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), parsed);
                if (ec != std::errc{} || end != value.data() + value.size())
                    throw std::runtime_error("Invalid TAE timing value: " + std::string(value));
                return parsed;
            };
            attack.startTime = parseTime(start);
            attack.endTime = parseTime(end);

            if (!std::isfinite(attack.startTime) || !std::isfinite(attack.endTime) ||
                attack.startTime < 0 || attack.endTime < attack.startTime)
                throw std::runtime_error("Invalid TAE attack window");

            events.push_back(attack);
        }

        std::stable_sort(
            events.begin(),
            events.end(),
            [](const AttackEvent& a, const AttackEvent& b) {
                return a.startTime < b.startTime;
            }
        );

        return events;
    }

    MotionValues extractMotionValues(
        const ParamTable& attacks,
        pugi::xml_node row
    ) {
        MotionValues result;

        constexpr std::array<const char*, 5> damageFields = {
            "atkPhysCorrection",
            "atkMagCorrection",
            "atkFireCorrection",
            "atkThunCorrection",
            "atkDarkCorrection"
        };

        for (std::size_t i = 0; i < damageFields.size(); ++i) {
            if (const auto value = attacks.number(row, damageFields[i])) {
                result.values[i] = *value;
                result.known[i] = true;
            }
        }

        // Validated against the spreadsheet for ordinary powerstance
        // attacks. Retain independent channels for future exceptions.
        if (const auto status = attacks.number(
                row, "statusAilmentAtkPowerCorrectRate")) {

            for (std::size_t i = 5; i < enumerators_of<AttackPowerType>().size(); ++i) {
                result.values[i] = *status;
                result.known[i] = true;
            }
        }

        return result;
    }

    BulletParameters extractBulletParameters(const ParamTable& bullets, pugi::xml_node row)
    {
        BulletParameters result;
        result.life = bullets.number(row, "life");
        result.lifeRandomRange = bullets.number(row, "lifeRandomRange");
        result.dmgHitRecordLifeTime = bullets.number(row, "dmgHitRecordLifeTime");
        result.dist = bullets.number(row, "dist");
        constexpr std::array dampingFields{"damageDamp", "spelDamageDamp", "fireDamageDamp", "thunderDamageDamp", "darkDamageDamp"};
        static_assert(dampingFields.size() == enumerators_of<DamageType>().size());
        for (std::size_t i = 0; i < dampingFields.size(); ++i)
            result.damageDamping[i] = bullets.number(row, dampingFields[i]);
        if (const auto shared = bullets.integer(row, "isUseSharedHitList")) {
            if (*shared != 0 && *shared != 1)
                throw std::runtime_error("Invalid Bullet isUseSharedHitList flag");
            result.isUseSharedHitList = *shared == 1;
        }
        result.launchConditionType = bullets.integer(row, "launchConditionType");
        result.spEffectIDForShooter = bullets.integer(row, "spEffectIDForShooter");
        for (std::size_t i = 0; i < result.targetSpEffectIds.size(); ++i)
            result.targetSpEffectIds[i] = bullets.integer(row, std::format("spEffectId{}", i).c_str());
        return result;
    }

    enum class BehaviorLookup {
        // Diagnostic matching by fields; ordinary collisions remain explicit.
        STRICT_FIELDS,
        // The caller has selected a player weapon's behavior variation. This
        // does not select a different weapon for mixed-pair or body events.
        PLAYER_WEAPON
    };

    ResolutionResult resolveAttackEvents(
        const std::vector<AttackEvent>& events,
        int behaviorVariationId,
        const BehaviorIndex& behaviors,
        const ParamTable& attacks,
        std::optional<int> requiredCategory = std::nullopt,
        std::optional<int> requiredBehaviorParamId = std::nullopt,
        BehaviorLookup lookup = BehaviorLookup::STRICT_FIELDS,
        const ParamTable* bullets = nullptr
    ) {
        ResolutionResult result;

        for (const auto& event : events) {
            const auto matches = lookup == BehaviorLookup::PLAYER_WEAPON
                ? behaviors.findPlayerWeapon(behaviorVariationId, event.behaviorJudgeId)
                : event.behaviorJudgeId >= 1000 && event.behaviorJudgeId <= 9999
                    ? behaviors.findEncoded(behaviorVariationId, event.behaviorJudgeId)
                    : behaviors.find(behaviorVariationId, event.behaviorJudgeId);

            auto fail = [&](std::string message) {
                UnresolvedAttack unresolved{event, behaviorVariationId, requiredCategory, {}, requiredBehaviorParamId};
                message += std::format(" [animation {}, variation {}, judge {}, required category {}]",
                    event.animationId, behaviorVariationId, event.behaviorJudgeId,
                    requiredCategory ? std::to_string(*requiredCategory) : "any");
                if (requiredBehaviorParamId)
                    message += std::format(" [required behavior {}]", *requiredBehaviorParamId);
                for (const auto row : matches) {
                    BehaviorCandidate candidate{
                        parseInt(row.attribute("id").value()),
                        behaviors.integer(row, "category"),
                        behaviors.integer(row, "refType"),
                        behaviors.integer(row, "refId")
                    };
                    auto display = [](std::optional<int> value) {
                        return value ? std::to_string(*value) : "missing";
                    };
                    message += std::format("; candidate behavior={} category={} refType={} refId={}",
                        candidate.behaviorParamId, display(candidate.category),
                        display(candidate.refType), display(candidate.refId));
                    unresolved.candidates.push_back(candidate);
                }
                result.diagnostics.push_back(std::move(message));
                result.unresolved.push_back(std::move(unresolved));
            };

            if (event.eventType == 307) {
                const auto source = calculator::pc_behavior_uses_secondary_weapon(event);
                if (!source) {
                    fail(source.error());
                    continue;
                }
            }

            // Do not silently select one if the behavior mapping
            // is ambiguous or uses an unsupported reference type.
            std::vector<pugi::xml_node> attackBehaviors;

            for (const auto row : matches) {
                const auto referenceType = behaviors.integer(row, "refType");
                if (((referenceType == 0 && event.eventType != 2) || (referenceType == 1 && bullets)) &&
                    (!requiredCategory ||
                     behaviors.integer(row, "category") == requiredCategory) &&
                    (!requiredBehaviorParamId ||
                     optionalInt(row.attribute("id")) == requiredBehaviorParamId))
                    attackBehaviors.push_back(row);
            }

            if (attackBehaviors.size() != 1) {
                fail(event.eventType == 2 ? "Unresolved or ambiguous BulletBehavior" : "Unresolved or ambiguous attack behavior");
                continue;
            }

            const auto behavior = attackBehaviors.front();
            const auto refId = behaviors.integer(behavior, "refId");
            const auto behaviorId =
                optionalInt(behavior.attribute("id"));

            if (!refId) {
                fail("Behavior has no valid refId");
                continue;
            }

            int attackId = *refId;
            std::optional<int> bulletId;
            std::optional<BulletParameters> bulletParameters;
            if (behaviors.integer(behavior, "refType") == 1) {
                try {
                    const auto bullet = bullets->find(*refId);
                    if (!bullet) throw std::runtime_error("Missing Bullet row " + std::to_string(*refId));
                    // Diagnostic resolution of one leaf reference only. Do not
                    // flatten child/interval/continuous/multiple-shot graphs or
                    // invent per-projectile events and activation windows.
                    if (bullets->integer(bullet, "numShoot") != 1 ||
                        bullets->number(bullet, "shootInterval") != 0 ||
                        bullets->integer(bullet, "HitBulletID") != -1 ||
                        bullets->integer(bullet, "intervalCreateBulletId") != -1 ||
                        bullets->integer(bullet, "isEndlessHit") != 0)
                        throw std::runtime_error("Unsupported Bullet emission or child-reference graph " + std::to_string(*refId));
                    const auto linkedAttack = bullets->integer(bullet, "atkId_Bullet");
                    if (!linkedAttack || *linkedAttack < 0)
                        throw std::runtime_error("Bullet has no valid atkId_Bullet " + std::to_string(*refId));
                    attackId = *linkedAttack;
                    bulletId = *refId;
                    bulletParameters = extractBulletParameters(*bullets, bullet);
                } catch (const std::runtime_error& error) {
                    fail(error.what());
                    continue;
                }
            }
            const auto attack = attacks.find(attackId);

            if (!attack) {
                fail("Missing AtkParam_Pc row " + std::to_string(attackId));
                continue;
            }

            ResolvedHit hit;
            hit.event = event;
            hit.behaviorParamId = behaviorId.value_or(-1);
            hit.behaviorCategory = behaviors.integer(behavior, "category").value_or(-1);
            hit.atkParamId = attackId;
            hit.bulletParamId = bulletId;
            hit.bulletParameters = std::move(bulletParameters);
            try {
                hit.motionValues = extractMotionValues(attacks, attack);
                if (const auto opposing = attacks.number(attack, "opposeTarget")) {
                    if (*opposing != 0 && *opposing != 1)
                        throw std::runtime_error("Invalid AtkParam opposeTarget flag");
                    hit.opposeTarget = *opposing == 1;
                }
            } catch (const std::runtime_error& error) {
                fail(error.what());
                continue;
            }

            result.hits.push_back(std::move(hit));
        }

        return result;
    }


    // Category 1 is established for the two otherwise ambiguous 1H entries.
    // Other entries require a unique match: 2H/riposte and powerstance events
    // use different categories, so there is deliberately no catalog-wide default.
    inline constexpr std::array straightSwordCatalog = {
        MoveDefinition{MoveId::ONE_HAND_R1_1, "1H R1 #1", 30000},
        MoveDefinition{MoveId::ONE_HAND_R1_2, "1H R1 #2", 30010},
        MoveDefinition{MoveId::ONE_HAND_R1_3, "1H R1 #3", 30020},
        MoveDefinition{MoveId::ONE_HAND_R1_4, "1H R1 #4", 30030},
        MoveDefinition{MoveId::ONE_HAND_R1_5, "1H R1 #5", 30040},

        MoveDefinition{MoveId::ONE_HAND_R2_1, "1H R2 #1", 30505, 1},
        MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_1, "1H Charged R2 #1", 30500},
        MoveDefinition{MoveId::ONE_HAND_R2_2, "1H R2 #2", 30515},
        MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_2, "1H Charged R2 #2", 30510},

        MoveDefinition{MoveId::ONE_HAND_RUNNING_R1, "1H Running R1", 30200},
        MoveDefinition{MoveId::ONE_HAND_RUNNING_R2, "1H Running R2", 30210},
        MoveDefinition{MoveId::ONE_HAND_ROLLING_R1, "1H Rolling R1", 30300},
        MoveDefinition{MoveId::ONE_HAND_BACKSTEP_R1, "1H Backstep R1", 30400},
        MoveDefinition{MoveId::ONE_HAND_JUMPING_R1, "1H Jumping R1", 31030, 1},
        MoveDefinition{MoveId::ONE_HAND_JUMPING_R2, "1H Jumping R2", 31230},
        MoveDefinition{MoveId::ONE_HAND_GUARD_COUNTER, "1H Guard Counter", 30700},

        MoveDefinition{MoveId::TWO_HAND_R1_1, "2H R1 #1", 32000},
        MoveDefinition{MoveId::TWO_HAND_R1_2, "2H R1 #2", 32010},
        MoveDefinition{MoveId::TWO_HAND_R1_3, "2H R1 #3", 32020},
        MoveDefinition{MoveId::TWO_HAND_R1_4, "2H R1 #4", 32030},
        MoveDefinition{MoveId::TWO_HAND_R1_5, "2H R1 #5", 32040},

        MoveDefinition{MoveId::TWO_HAND_R2_1, "2H R2 #1", 32505},
        MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_1, "2H Charged R2 #1", 32500},
        MoveDefinition{MoveId::TWO_HAND_R2_2, "2H R2 #2", 32515},
        MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_2, "2H Charged R2 #2", 32510},

        MoveDefinition{MoveId::TWO_HAND_RUNNING_R1, "2H Running R1", 32200},
        MoveDefinition{MoveId::TWO_HAND_RUNNING_R2, "2H Running R2", 32210},
        MoveDefinition{MoveId::TWO_HAND_ROLLING_R1, "2H Rolling R1", 32300},
        MoveDefinition{MoveId::TWO_HAND_BACKSTEP_R1, "2H Backstep R1", 32400},
        MoveDefinition{MoveId::TWO_HAND_JUMPING_R1, "2H Jumping R1", 33030},
        MoveDefinition{MoveId::TWO_HAND_JUMPING_R2, "2H Jumping R2", 33230},
        MoveDefinition{MoveId::TWO_HAND_GUARD_COUNTER, "2H Guard Counter", 32700},

        MoveDefinition{MoveId::POWERSTANCE_L1_1, "Powerstance L1 #1", 34000},
        MoveDefinition{MoveId::POWERSTANCE_L1_2, "Powerstance L1 #2", 34010},
        MoveDefinition{MoveId::POWERSTANCE_L1_3, "Powerstance L1 #3", 34020},
        MoveDefinition{MoveId::POWERSTANCE_L1_4, "Powerstance L1 #4", 34030},
        MoveDefinition{MoveId::POWERSTANCE_RUNNING_L1, "Powerstance Running L1", 34200},
        MoveDefinition{MoveId::POWERSTANCE_ROLLING_L1, "Powerstance Rolling L1", 34300},
        MoveDefinition{MoveId::POWERSTANCE_BACKSTEP_L1, "Powerstance Backstep L1", 34400},
        MoveDefinition{MoveId::POWERSTANCE_JUMPING_L1, "Powerstance Jumping L1", 34530},

        MoveDefinition{MoveId::OFFHAND_R1_1, "Offhand R1 #1", 35000},
        MoveDefinition{MoveId::OFFHAND_R1_2, "Offhand R1 #2", 35010},
        MoveDefinition{MoveId::OFFHAND_R1_3, "Offhand R1 #3", 35020},
        MoveDefinition{MoveId::OFFHAND_R1_4, "Offhand R1 #4", 35030},
        MoveDefinition{MoveId::OFFHAND_R1_5, "Offhand R1 #5", 35040},

        MoveDefinition{MoveId::MOUNTED_R1_1, "Mounted R1 #1", 38000},
        MoveDefinition{MoveId::MOUNTED_R1_2, "Mounted R1 #2", 38010},
        MoveDefinition{MoveId::MOUNTED_R1_3, "Mounted R1 #3", 38020},
        MoveDefinition{MoveId::MOUNTED_R2, "Mounted R2", 38110},
        MoveDefinition{MoveId::MOUNTED_R2_CHARGING, "Mounted R2 Charging", 38100, 12, std::nullopt, 630},
        MoveDefinition{MoveId::MOUNTED_CHARGED_R2, "Mounted Charged R2", 38100, 12, std::nullopt, 635},
        MoveDefinition{MoveId::MOUNTED_L1_1, "Mounted L1 #1", 39000},
        MoveDefinition{MoveId::MOUNTED_L1_2, "Mounted L1 #2", 39010},
        MoveDefinition{MoveId::MOUNTED_L1_3, "Mounted L1 #3", 39020},
        MoveDefinition{MoveId::MOUNTED_L2, "Mounted L2", 39110},
        MoveDefinition{MoveId::MOUNTED_L2_CHARGING, "Mounted L2 Charging", 39100, 12, std::nullopt, 730},
        MoveDefinition{MoveId::MOUNTED_CHARGED_L2, "Mounted Charged L2", 39100, 12, std::nullopt, 735},

        // Judge 590 is the zero-damage throw acquisition; 509 is the whiff strike.
        MoveDefinition{MoveId::BACKSTAB_WHIFF, "Backstab Whiff", 31719, 12, std::nullopt, 509},
        MoveDefinition{MoveId::BACKSTAB, "Backstab", 31710},
        MoveDefinition{MoveId::RIPOSTE, "Riposte", 31700},
        // ThrowParam selects 31720 for small-target backstabs, 31730 for ripostes.
        MoveDefinition{MoveId::SMALL_PVE_BACKSTAB, "Small PvE Backstab", 31720},
        MoveDefinition{MoveId::SMALL_PVE_RIPOSTE, "Small PvE Riposte", 31730},
        MoveDefinition{MoveId::LARGE_PVE_RIPOSTE, "Large PvE Riposte", 31750}
    };

    // Ordinary axes share this 63-move vocabulary, graph clip IDs and mounted
    // phase judges with straight swords. Hit data comes from their own TAE and
    // behavior rows. Criticals select victim-directed throw hits explicitly.
    inline constexpr auto axeCatalog = [] {
        auto catalog = straightSwordCatalog;
        for (auto& definition : catalog) {
            definition.requiredCategory.reset();
            definition.requiredBehaviorParamId.reset();
            switch (definition.id) {
            case MoveId::BACKSTAB: case MoveId::RIPOSTE:
            case MoveId::SMALL_PVE_BACKSTAB: case MoveId::SMALL_PVE_RIPOSTE:
            case MoveId::LARGE_PVE_RIPOSTE:
                definition.eventType = 304;
                break;
            default: break;
            }
        }
        return catalog;
    }();

    // Ordinary hammers use the same complete move vocabulary and phase filters.
    // Their TAE aliases and behavior rows retain hammer-specific hit data.
    inline constexpr auto hammerCatalog = axeCatalog;
    inline constexpr auto katanaCatalog = axeCatalog;

    // Derive complete class membership from the shared move vocabulary. A
    // missing/duplicate omission fails at compile time instead of leaving a
    // default definition or quietly changing the expected catalog size.
    template<std::size_t N, std::size_t M>
    consteval auto omitCatalogMoves(const std::array<MoveDefinition, N>& source,
        const std::array<MoveId, M>& omitted) {
        std::array<MoveDefinition, N - M> result;
        std::size_t next = 0;
        for (const auto& definition : source) {
            if (std::ranges::find(omitted, definition.id) != omitted.end()) continue;
            if (next == result.size()) throw "Invalid omitted catalog membership";
            result[next++] = definition;
        }
        if (next != result.size()) throw "Invalid omitted catalog membership";
        return result;
    }

    // HKS limits ordinary/offhand flail chains to four stages; powerstance
    // still has four. Other move families retain the shared phase definitions.
    inline constexpr auto flailCatalog = omitCatalogMoves(axeCatalog, std::array{
        MoveId::ONE_HAND_R1_5, MoveId::TWO_HAND_R1_5, MoveId::OFFHAND_R1_5});

    // HKS gives these ordinary heavy families four single-weapon lights and
    // three paired lights. Graph import independently verifies membership.
    inline constexpr auto greatswordCatalog = omitCatalogMoves(flailCatalog, std::array{
        MoveId::POWERSTANCE_L1_4});
    inline constexpr auto greataxeCatalog = greatswordCatalog;
    inline constexpr auto greathammerCatalog = greatswordCatalog;
    inline constexpr auto curvedGreatswordCatalog = greatswordCatalog;

    // Colossal classes have three ordinary/offhand and paired light stages.
    inline constexpr auto colossalSwordCatalog = omitCatalogMoves(greatswordCatalog, std::array{
        MoveId::ONE_HAND_R1_4, MoveId::TWO_HAND_R1_4, MoveId::OFFHAND_R1_4});
    inline constexpr auto colossalWeaponCatalog = colossalSwordCatalog;

    inline constexpr auto halberdCatalog = greatswordCatalog;
    // Scythe mounted charge clips contain release judges 635/735 only;
    // holding the charge has no attack event to import as a damaging move.
    inline constexpr auto scytheCatalog = omitCatalogMoves(halberdCatalog, std::array{
        MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING});

    inline constexpr auto spearCatalog = []() consteval {
        std::array<MoveDefinition, greatswordCatalog.size() + 1> result;
        std::ranges::copy(greatswordCatalog, result.begin());
        // Explicit ordinary guard-style sample, not inferred left equipment.
        result.back() = {MoveId::SHIELDPOKE, "Shieldpoke (normal guard)", 30900};
        return result;
    }();

    // Three ordinary/offhand lights, three paired lights and explicit shieldpoke.
    // Conditional a37 events remain in the hit data and require evaluation context.
    inline constexpr auto greatSpearCatalog = omitCatalogMoves(spearCatalog, std::array{
        MoveId::ONE_HAND_R1_4, MoveId::TWO_HAND_R1_4, MoveId::OFFHAND_R1_4});

    // Whips cannot initiate critical throws (enableThrow=0). Their mounted
    // charge clips contain release hits only, as in the scythe vocabulary.
    inline constexpr auto whipCatalog = omitCatalogMoves(scytheCatalog, std::array{
        MoveId::BACKSTAB_WHIFF, MoveId::BACKSTAB, MoveId::RIPOSTE,
        MoveId::SMALL_PVE_BACKSTAB, MoveId::SMALL_PVE_RIPOSTE, MoveId::LARGE_PVE_RIPOSTE});

    // Six ordinary/offhand lights, four paired lights, shieldpoke and four
    // charged-heavy feints. Neutral definitions retain the shared phase filters.
    inline constexpr auto thrustingSwordCatalog = []() consteval {
        std::array<MoveDefinition, axeCatalog.size() + 8> result;
        std::ranges::copy(axeCatalog, result.begin());
        std::size_t next = axeCatalog.size();
        for (const auto& definition : std::array{
                MoveDefinition{MoveId::ONE_HAND_R1_6, "1H R1 #6", 30050},
                MoveDefinition{MoveId::TWO_HAND_R1_6, "2H R1 #6", 32050},
                MoveDefinition{MoveId::OFFHAND_R1_6, "Offhand R1 #6", 35050},
                MoveDefinition{MoveId::SHIELDPOKE, "Shieldpoke (normal guard)", 30900},
                MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_1_FEINT, "1H Charged R2 #1 Feint", 30950},
                MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_2_FEINT, "1H Charged R2 #2 Feint", 30955},
                MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_1_FEINT, "2H Charged R2 #1 Feint", 32950},
                MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_2_FEINT, "2H Charged R2 #2 Feint", 32955}})
            result[next++] = definition;
        return result;
    }();

    // Curved swords share the six-stage lights and four feints, but the native
    // guard-attack eligibility excludes this category.
    inline constexpr auto curvedSwordCatalog = omitCatalogMoves(thrustingSwordCatalog, std::array{
        MoveId::SHIELDPOKE});

    // Great katanas have four ordinary/offhand and three paired light stages,
    // with the same four requested feints and explicit mounted phase filters.
    inline constexpr auto greatKatanaCatalog = omitCatalogMoves(curvedSwordCatalog, std::array{
        MoveId::ONE_HAND_R1_5, MoveId::TWO_HAND_R1_5, MoveId::OFFHAND_R1_5,
        MoveId::ONE_HAND_R1_6, MoveId::TWO_HAND_R1_6, MoveId::OFFHAND_R1_6,
        MoveId::POWERSTANCE_L1_4});

    // Mounted Start has release judges 638/738 after its charge window and
    // no charging attack phase. Other events retain raw opposing-target flags.
    inline constexpr auto lightGreatswordCatalog = []() consteval {
        auto result = omitCatalogMoves(axeCatalog, std::array{
            MoveId::MOUNTED_R2_CHARGING, MoveId::MOUNTED_L2_CHARGING});
        for (auto& definition : result) {
            if (definition.id == MoveId::MOUNTED_CHARGED_R2) definition.eventJudgeId = 638;
            if (definition.id == MoveId::MOUNTED_CHARGED_L2) definition.eventJudgeId = 738;
        }
        return result;
    }();

    // HKS excludes matching powerstance for thrusting shields. Their four
    // ordinary/offhand stages and mounted charging phases remain available.
    inline constexpr auto thrustingShieldCatalog = omitCatalogMoves(flailCatalog, std::array{
        MoveId::POWERSTANCE_L1_1, MoveId::POWERSTANCE_L1_2, MoveId::POWERSTANCE_L1_3,
        MoveId::POWERSTANCE_L1_4, MoveId::POWERSTANCE_RUNNING_L1, MoveId::POWERSTANCE_ROLLING_L1,
        MoveId::POWERSTANCE_BACKSTEP_L1, MoveId::POWERSTANCE_JUMPING_L1});

    // Torches have the same ordinary selected-state vocabulary, but their
    // weapon parameters disable critical throws and backstab acquisition.
    inline constexpr auto torchCatalog = omitCatalogMoves(thrustingShieldCatalog, std::array{
        MoveId::BACKSTAB_WHIFF, MoveId::BACKSTAB, MoveId::RIPOSTE,
        MoveId::SMALL_PVE_BACKSTAB, MoveId::SMALL_PVE_RIPOSTE, MoveId::LARGE_PVE_RIPOSTE});

    constexpr bool ordinaryWeaponUsesBuiltInPair(int motionCategory) {
        return motionCategory == 22 || motionCategory == 42 || motionCategory == 55 || motionCategory == 58 || motionCategory == 62;
    }

    // Audited ordinary stance signatures, not arbitrary native pose routing.
    // Attack graph selectors still use the weapon category (offsets 13/16).
    constexpr int ordinaryWeaponMotionPosition(int motionCategory, bool twoHanded) {
        // Audited built-in pairs retain stance zero in either style.
        if (ordinaryWeaponUsesBuiltInPair(motionCategory)) return 0;
        // Heavy thrusting swords have asymmetric authored stance positions.
        if (motionCategory == 39 || motionCategory == 60) return twoHanded ? 10 : 3;
        if (motionCategory == 36 || motionCategory == 37 || motionCategory == 38 || motionCategory == 50
            || motionCategory == 57)
            return twoHanded ? 13 : 3;
        const bool heavy = motionCategory == 25 || motionCategory == 26 || motionCategory == 31
            || motionCategory == 32 || motionCategory == 35 || motionCategory == 40 || motionCategory == 61;
        return (twoHanded ? 10 : 0) + (heavy ? 2 : 0);
    }

    // Verified for the base Dagger's variation 100, not all dagger-type weapons.
    inline constexpr std::array daggerCatalog = {
        MoveDefinition{MoveId::ONE_HAND_R1_1, "1H R1 #1", 30000},
        MoveDefinition{MoveId::ONE_HAND_R1_2, "1H R1 #2", 30010},
        MoveDefinition{MoveId::ONE_HAND_R1_3, "1H R1 #3", 30020},
        MoveDefinition{MoveId::ONE_HAND_R1_4, "1H R1 #4", 30030},
        MoveDefinition{MoveId::ONE_HAND_R1_5, "1H R1 #5", 30040},
        MoveDefinition{MoveId::ONE_HAND_R1_6, "1H R1 #6", 30050},
        MoveDefinition{MoveId::ONE_HAND_R2_1, "1H R2 #1", 30505},
        MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_1, "1H Charged R2 #1", 30500},
        MoveDefinition{MoveId::ONE_HAND_R2_2, "1H R2 #2", 30515},
        MoveDefinition{MoveId::ONE_HAND_CHARGED_R2_2, "1H Charged R2 #2", 30510},
        MoveDefinition{MoveId::ONE_HAND_RUNNING_R1, "1H Running R1", 30200},
        MoveDefinition{MoveId::ONE_HAND_RUNNING_R2, "1H Running R2", 30210},
        MoveDefinition{MoveId::ONE_HAND_ROLLING_R1, "1H Rolling R1", 30300},
        MoveDefinition{MoveId::ONE_HAND_BACKSTEP_R1, "1H Backstep R1", 30400},
        MoveDefinition{MoveId::ONE_HAND_JUMPING_R1, "1H Jumping R1", 31030, 1},
        MoveDefinition{MoveId::ONE_HAND_JUMPING_R2, "1H Jumping R2", 31230},
        MoveDefinition{MoveId::ONE_HAND_GUARD_COUNTER, "1H Guard Counter", 30700},

        MoveDefinition{MoveId::TWO_HAND_R1_1, "2H R1 #1", 32000},
        MoveDefinition{MoveId::TWO_HAND_R1_2, "2H R1 #2", 32010},
        MoveDefinition{MoveId::TWO_HAND_R1_3, "2H R1 #3", 32020},
        MoveDefinition{MoveId::TWO_HAND_R1_4, "2H R1 #4", 32030},
        MoveDefinition{MoveId::TWO_HAND_R1_5, "2H R1 #5", 32040},
        MoveDefinition{MoveId::TWO_HAND_R1_6, "2H R1 #6", 32050},
        MoveDefinition{MoveId::TWO_HAND_R2_1, "2H R2 #1", 32505},
        MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_1, "2H Charged R2 #1", 32500},
        // XML labels these rows "Default - Dagger". Blood Tax has the same
        // variation, judge AND category, so category 12 alone is insufficient.
        MoveDefinition{MoveId::TWO_HAND_R2_2, "2H R2 #2", 32515, 12, 100100310},
        MoveDefinition{MoveId::TWO_HAND_CHARGED_R2_2, "2H Charged R2 #2", 32510, 12, 100100315},
        MoveDefinition{MoveId::TWO_HAND_RUNNING_R1, "2H Running R1", 32200},
        MoveDefinition{MoveId::TWO_HAND_RUNNING_R2, "2H Running R2", 32210},
        MoveDefinition{MoveId::TWO_HAND_ROLLING_R1, "2H Rolling R1", 32300},
        MoveDefinition{MoveId::TWO_HAND_BACKSTEP_R1, "2H Backstep R1", 32400},
        MoveDefinition{MoveId::TWO_HAND_JUMPING_R1, "2H Jumping R1", 33030},
        MoveDefinition{MoveId::TWO_HAND_JUMPING_R2, "2H Jumping R2", 33230},
        MoveDefinition{MoveId::TWO_HAND_GUARD_COUNTER, "2H Guard Counter", 32700},

        MoveDefinition{MoveId::POWERSTANCE_L1_1, "Powerstance L1 #1", 34000},
        MoveDefinition{MoveId::POWERSTANCE_L1_2, "Powerstance L1 #2", 34010},
        MoveDefinition{MoveId::POWERSTANCE_L1_3, "Powerstance L1 #3", 34020},
        MoveDefinition{MoveId::POWERSTANCE_L1_4, "Powerstance L1 #4", 34030},
        MoveDefinition{MoveId::POWERSTANCE_RUNNING_L1, "Powerstance Running L1", 34200},
        MoveDefinition{MoveId::POWERSTANCE_ROLLING_L1, "Powerstance Rolling L1", 34300},
        MoveDefinition{MoveId::POWERSTANCE_BACKSTEP_L1, "Powerstance Backstep L1", 34400},
        MoveDefinition{MoveId::POWERSTANCE_JUMPING_L1, "Powerstance Jumping L1", 34530},

        MoveDefinition{MoveId::OFFHAND_R1_1, "Offhand R1 #1", 35000},
        MoveDefinition{MoveId::OFFHAND_R1_2, "Offhand R1 #2", 35010},
        MoveDefinition{MoveId::OFFHAND_R1_3, "Offhand R1 #3", 35020},
        MoveDefinition{MoveId::OFFHAND_R1_4, "Offhand R1 #4", 35030},
        MoveDefinition{MoveId::OFFHAND_R1_5, "Offhand R1 #5", 35040},
        MoveDefinition{MoveId::OFFHAND_R1_6, "Offhand R1 #6", 35050},

        MoveDefinition{MoveId::MOUNTED_R1_1, "Mounted R1 #1", 38000},
        MoveDefinition{MoveId::MOUNTED_R1_2, "Mounted R1 #2", 38010},
        MoveDefinition{MoveId::MOUNTED_R1_3, "Mounted R1 #3", 38020},
        MoveDefinition{MoveId::MOUNTED_R2, "Mounted R2", 38110},
        MoveDefinition{MoveId::MOUNTED_R2_CHARGING, "Mounted R2 Charging", 38100, 12, std::nullopt, 630},
        MoveDefinition{MoveId::MOUNTED_CHARGED_R2, "Mounted Charged R2", 38100, 12, std::nullopt, 635},
        MoveDefinition{MoveId::MOUNTED_L1_1, "Mounted L1 #1", 39000},
        MoveDefinition{MoveId::MOUNTED_L1_2, "Mounted L1 #2", 39010},
        MoveDefinition{MoveId::MOUNTED_L1_3, "Mounted L1 #3", 39020},
        MoveDefinition{MoveId::MOUNTED_L2, "Mounted L2", 39110},
        MoveDefinition{MoveId::MOUNTED_L2_CHARGING, "Mounted L2 Charging", 39100, 12, std::nullopt, 730},
        MoveDefinition{MoveId::MOUNTED_CHARGED_L2, "Mounted Charged L2", 39100, 12, std::nullopt, 735},

        MoveDefinition{MoveId::BACKSTAB_WHIFF, "Backstab Whiff", 31719, 12, std::nullopt, 509},
        MoveDefinition{MoveId::BACKSTAB, "Backstab", 31710},
        MoveDefinition{MoveId::RIPOSTE, "Riposte", 31700},
        // Default - Dagger and Repeating Thrust share judge 530 and category 12.
        MoveDefinition{MoveId::SMALL_PVE_BACKSTAB, "Small PvE Backstab", 31720, 12, 100100530},
        MoveDefinition{MoveId::SMALL_PVE_RIPOSTE, "Small PvE Riposte", 31730, 12, 100100530},
        MoveDefinition{MoveId::LARGE_PVE_RIPOSTE, "Large PvE Riposte", 31750}
    };

    // Six-stage ordinary/offhand and matching-pair chains. The two 1H
    // movement states invoke PCBehavior instead of AttackBehavior.
    inline constexpr auto clawCatalog = [] {
        std::array<MoveDefinition, daggerCatalog.size() + 2> catalog;
        std::ranges::copy(daggerCatalog, catalog.begin());
        catalog[daggerCatalog.size()] = {MoveId::POWERSTANCE_L1_5, "Powerstance L1 #5", 34040};
        catalog[daggerCatalog.size() + 1] = {MoveId::POWERSTANCE_L1_6, "Powerstance L1 #6", 34050};
        for (auto& definition : catalog) {
            definition.requiredCategory.reset();
            definition.requiredBehaviorParamId.reset();
            switch (definition.id) {
            case MoveId::ONE_HAND_RUNNING_R1: case MoveId::ONE_HAND_ROLLING_R1:
                definition.eventType = 307; break;
            case MoveId::BACKSTAB: case MoveId::RIPOSTE:
            case MoveId::SMALL_PVE_BACKSTAB: case MoveId::SMALL_PVE_RIPOSTE:
            case MoveId::LARGE_PVE_RIPOSTE:
                definition.eventType = 304; break;
            default: break;
            }
        }
        return catalog;
    }();

    // Fists share the six-stage vocabulary, but running R1 uses type 1.
    // Only rolling R1 invokes the audited right-weapon PCBehavior selector.
    inline constexpr auto fistCatalog = [] {
        auto catalog = clawCatalog;
        for (auto& definition : catalog)
            if (definition.id == MoveId::ONE_HAND_RUNNING_R1) definition.eventType = 1;
        return catalog;
    }();

    // Four ordinary/offhand and matching-pair stages, four neutral feints.
    // Both 1H movement R1 states use the audited right-weapon PCBehavior.
    inline constexpr auto beastClawCatalog = [] {
        std::array<MoveDefinition, greatKatanaCatalog.size() + 1> catalog;
        std::ranges::copy(greatKatanaCatalog, catalog.begin());
        catalog.back() = {MoveId::POWERSTANCE_L1_4, "Powerstance L1 #4", 34030};
        for (auto& definition : catalog) {
            definition.requiredCategory.reset();
            definition.requiredBehaviorParamId.reset();
            switch (definition.id) {
            case MoveId::ONE_HAND_RUNNING_R1: case MoveId::ONE_HAND_ROLLING_R1:
                definition.eventType = 307; break;
            case MoveId::BACKSTAB: case MoveId::RIPOSTE:
            case MoveId::SMALL_PVE_BACKSTAB: case MoveId::SMALL_PVE_RIPOSTE:
            case MoveId::LARGE_PVE_RIPOSTE:
                definition.eventType = 304; break;
            default: break;
            }
        }
        return catalog;
    }();

    // Six-stage built-in-pair vocabulary. Both movement R1 states use type 1;
    // the mounted right charge/release uses judges 730/735, like the left.
    inline constexpr auto backhandBladeCatalog = [] {
        auto catalog = clawCatalog;
        for (auto& definition : catalog) {
            if (definition.id == MoveId::ONE_HAND_RUNNING_R1 || definition.id == MoveId::ONE_HAND_ROLLING_R1)
                definition.eventType = 1;
            if (definition.id == MoveId::MOUNTED_R2_CHARGING) definition.eventJudgeId = 730;
            if (definition.id == MoveId::MOUNTED_CHARGED_R2) definition.eventJudgeId = 735;
        }
        return catalog;
    }();

    // Martial arts retain all six stages and standard mounted phases.
    // Both 1H movement R1 states use ordinary AttackBehavior events.
    inline constexpr auto martialArtsCatalog = [] {
        auto catalog = clawCatalog;
        for (auto& definition : catalog)
            if (definition.id == MoveId::ONE_HAND_RUNNING_R1 || definition.id == MoveId::ONE_HAND_ROLLING_R1)
                definition.eventType = 1;
        return catalog;
    }();

    // Native special-110/base-23 vocabulary. The special registration changes
    // four 1H heavy phases and both rolling attacks; other states use base23.
    // Production graph resolution still selects the actual registered clips.
    inline constexpr auto straightSword110Catalog = [] {
        auto catalog = straightSwordCatalog;
        for (auto& definition : catalog) {
            switch (definition.id) {
            case MoveId::ONE_HAND_R2_1: case MoveId::ONE_HAND_CHARGED_R2_1:
            case MoveId::ONE_HAND_R2_2: case MoveId::ONE_HAND_CHARGED_R2_2:
            case MoveId::ONE_HAND_ROLLING_R1: case MoveId::TWO_HAND_ROLLING_R1:
                definition.motionCategory = 110; break;
            default: break;
            }
        }
        return catalog;
    }();

    // Native straight-sword special/base-23 heavy vocabulary for categories
    // 111/112/113/117. Each special registration changes all eight R2 phases;
    // other states use base23. Production graph resolution still selects the
    // actual registered clips. Warhawk's Talon (117) carries a second hit event
    // on most heavies; that data is read from its special clips, not authored here.
    consteval auto straightSwordHeavySpecialCatalog(int special) {
        auto catalog = straightSwordCatalog;
        for (auto& definition : catalog) {
            switch (definition.id) {
            case MoveId::ONE_HAND_R2_1: case MoveId::ONE_HAND_CHARGED_R2_1:
            case MoveId::ONE_HAND_R2_2: case MoveId::ONE_HAND_CHARGED_R2_2:
            case MoveId::TWO_HAND_R2_1: case MoveId::TWO_HAND_CHARGED_R2_1:
            case MoveId::TWO_HAND_R2_2: case MoveId::TWO_HAND_CHARGED_R2_2:
                definition.motionCategory = special; break;
            default: break;
            }
        }
        return catalog;
    }
    inline constexpr auto straightSword111Catalog = straightSwordHeavySpecialCatalog(111);
    inline constexpr auto straightSword112Catalog = straightSwordHeavySpecialCatalog(112);
    inline constexpr auto straightSword113Catalog = straightSwordHeavySpecialCatalog(113);
    inline constexpr auto straightSword117Catalog = straightSwordHeavySpecialCatalog(117);

    // Representative partial override fixture. Production graph resolution
    // chooses registered special clips and falls back only on absent entries.
    inline constexpr auto footworkCatalog = [] {
        auto catalog = martialArtsCatalog;
        using enum MoveId;
        for (auto& definition : catalog) {
            const auto id = definition.id;
            if ((id >= ONE_HAND_R1_1 && id <= ONE_HAND_CHARGED_R2_2) || id == ONE_HAND_R1_6
                || (id >= TWO_HAND_R1_1 && id <= TWO_HAND_CHARGED_R2_2) || id == TWO_HAND_R1_6
                || (id >= OFFHAND_R1_1 && id <= OFFHAND_R1_6))
                definition.motionCategory = 246;
        }
        return catalog;
    }();

    // Audited special/base category pairs, not weapon-ID exceptions. The scoped
    // priority policy is inferred from partial overrides, not recovered native
    // selector code; do not extend it to arbitrary special categories.
    constexpr bool auditedSpecialWeaponRouting(int motionCategory, int specialCategory) {
        return (motionCategory == 55 && specialCategory == 246)
            || (motionCategory == 23 && specialCategory == 110)
            || (motionCategory == 23 && specialCategory == 111)
            || (motionCategory == 23 && specialCategory == 112)
            || (motionCategory == 23 && specialCategory == 113)
            || (motionCategory == 23 && specialCategory == 117);
    }

    class AnimationCache {
    public:
        explicit AnimationCache(std::filesystem::path tempDir)
            : taeRoot_(
                std::move(tempDir) /
                "chr/c0000-anibnd-dcx-wanibnd/"
                "INTERROOT_win64/chr/c0000/tae"
            ) {}

        const std::vector<AttackEvent>& get(
            int motionCategory,
            int animationId,
            bool includePCBehaviors = false,
            bool includeBulletBehaviors = false
        ) {
            std::set<std::pair<int, int>> active;
            return load(motionCategory, animationId, active, includePCBehaviors, includeBulletBehaviors);
        }

    private:
        const std::vector<AttackEvent>& load(
            int motionCategory, int animationId, std::set<std::pair<int, int>>& active,
            bool includePCBehaviors, bool includeBulletBehaviors
        ) {
            const auto key = std::pair{
                motionCategory, animationId
            };

            auto& cache = includePCBehaviors
                ? (includeBulletBehaviors ? pcBulletCache_ : pcCache_)
                : (includeBulletBehaviors ? bulletCache_ : cache_);
            if (auto it = cache.find(key); it != cache.end())
                return it->second;

            if (!active.insert(key).second)
                throw std::runtime_error(std::format("TAE alias cycle at a{} / {:06}", motionCategory, animationId));
            if (active.size() > 64)
                throw std::runtime_error("TAE alias chain exceeds 64 animations");

            const auto path =
                taeRoot_ /
                std::format("a{}-tae", motionCategory) /
                std::format("anim-{:06}.xml", animationId);

            pugi::xml_document tae;
            const auto result = tae.load_file(path.string().c_str());

            if (!result)
                throw std::runtime_error(
                    std::format(
                        "Failed to load {}: {}",
                        path.string(),
                        result.description()
                    )
                );

            std::vector<AttackEvent> events;
            const auto header = tae.child("anim").child("header");
            if (xmlValue(header, "type") == "ImportOtherAnim") {
                const auto target = xmlInt(header, "animId");
                if (!target || *target < 0)
                    throw std::runtime_error(std::format("Invalid TAE alias target in {}", path));
                // ImportOtherAnim replaces the event source with the referenced
                // animation; local rows are inactive, even if they contain attacks.
                // DSAnimStudio f1bff06, DSAProj.INNER_SolveAnimRefChain and
                // NewAnimationContainer.NewGetAnimFromRequest resolve this chain
                // before assigning TaeAnimation. Never merge the local rows.
                // The exported full animation ID encodes category * 1,000,000 + local ID.
                try {
                    events = load(*target / 1000000, *target % 1000000, active, includePCBehaviors, includeBulletBehaviors);
                } catch (const std::runtime_error& error) {
                    throw std::runtime_error(std::format("TAE alias {} -> {}: {}", path, *target, error.what()));
                }
                for (auto& event : events)
                    event.animationId = animationId;
            } else {
                events = extractAttackEvents(tae, animationId, includePCBehaviors, includeBulletBehaviors);
                for (auto& event : events) {
                    event.sourceMotionCategory = motionCategory;
                    event.sourceAnimationId = animationId;
                }
            }

            active.erase(key);

            return cache.emplace(
                key, std::move(events)
            ).first->second;
        }

    private:
        std::filesystem::path taeRoot_;

        std::map<
            std::pair<int, int>,
            std::vector<AttackEvent>
        > cache_;
        // Keep opt-in PCBehavior extraction separate so an earlier ordinary
        // request cannot hide PC events, or expose them to a later default one.
        decltype(cache_) pcCache_;
        decltype(cache_) bulletCache_;
        decltype(cache_) pcBulletCache_;
    };

    struct MoveImportFailure {
        MoveId moveId;
        std::string name;
        int animationId;
        std::vector<std::string> diagnostics;
        std::vector<UnresolvedAttack> unresolved;
    };

    struct MovesetImportReport {
        int motionCategory;
        int behaviorVariation;
        std::size_t requestedMoves;
        std::size_t importedMoves = 0;
        std::vector<MoveImportFailure> failures;
    };

    ResolutionResult resolveSelectedAttackEvents(
        const std::vector<AttackEvent>& events, const MoveDefinition& definition,
        int variation, const BehaviorIndex& behaviors, const ParamTable& attacks,
        std::optional<int> requiredCategory, BehaviorLookup lookup, const ParamTable* bullets)
    {
        std::vector<AttackEventSelection> selections{{definition.eventType.value_or(-1),
            definition.eventJudgeId, definition.requiredCategory, definition.requiredBehaviorParamId}};
        if (definition.additionalEventSelections) for (const auto& selection : *definition.additionalEventSelections) {
            if (selection.eventType != 1 && selection.eventType != 2 && selection.eventType != 304 && selection.eventType != 307)
                throw std::runtime_error("Unsupported additional attack event type filter");
            if (definition.eventJudgeId && !selection.eventJudgeId)
                throw std::runtime_error("Additional events for a phased move require an explicit judge filter");
            selections.push_back(selection);
        }
        std::vector<std::vector<AttackEvent>> groups(selections.size());
        std::vector<std::vector<std::size_t>> positions(selections.size());
        for (std::size_t position = 0; position < events.size(); ++position) {
            const auto& event = events[position];
            std::optional<std::size_t> owner;
            for (std::size_t i = 0; i < selections.size(); ++i) {
                const auto& selection = selections[i];
                const bool typeMatches = selection.eventType == -1
                    ? event.eventType == 1 || event.eventType == 304 : event.eventType == selection.eventType;
                if (!typeMatches || (selection.eventJudgeId && event.behaviorJudgeId != *selection.eventJudgeId)) continue;
                if (owner)
                    throw std::runtime_error(std::format("Overlapping attack event selections: animation {} type {} judge {}",
                        event.animationId, event.eventType, event.behaviorJudgeId));
                owner = i;
            }
            if (owner) {
                groups[*owner].push_back(event);
                positions[*owner].push_back(position);
            }
        }
        ResolutionResult result;
        std::map<std::size_t, ResolvedHit> ordered;
        for (std::size_t i = 0; i < selections.size(); ++i) {
            const auto& selection = selections[i];
            if (groups[i].empty()) {
                result.diagnostics.emplace_back(std::format("No attack events for selection type {} judge {}",
                    selection.eventType == -1 ? "direct" : std::to_string(selection.eventType),
                    selection.eventJudgeId ? std::to_string(*selection.eventJudgeId) : "any"));
                continue;
            }
            auto resolved = resolveAttackEvents(groups[i], variation, behaviors, attacks,
                requiredCategory ? requiredCategory : selection.requiredCategory,
                selection.requiredBehaviorParamId, lookup, bullets);
            if (resolved.diagnostics.empty()) {
                if (resolved.hits.size() != positions[i].size())
                    throw std::runtime_error("Selected attack event count mismatch");
                for (std::size_t j = 0; j < resolved.hits.size(); ++j)
                    ordered.emplace(positions[i][j], std::move(resolved.hits[j]));
            }
            result.diagnostics.insert(result.diagnostics.end(), std::make_move_iterator(resolved.diagnostics.begin()), std::make_move_iterator(resolved.diagnostics.end()));
            result.unresolved.insert(result.unresolved.end(), std::make_move_iterator(resolved.unresolved.begin()), std::make_move_iterator(resolved.unresolved.end()));
        }
        // Cache positions retain equal-time and repeated event order without
        // comparing identities or deduplicating. A failed component publishes
        // no partial composed hit list; its owned diagnostics still survive.
        if (result.diagnostics.empty())
            for (auto& [position, hit] : ordered) result.hits.push_back(std::move(hit));
        return result;
    }

    template <class AddMove>
        requires std::invocable<AddMove&, const MoveDefinition&, const std::vector<ResolvedHit>&>
    MovesetImportReport importMoveset(
        AnimationCache& animationCache,
        int motionCategory,
        int behaviorVariation,
        std::span<const MoveDefinition> catalog,
        const BehaviorIndex& behaviors,
        const ParamTable& attacks,
        AddMove&& addMove,
        std::optional<int> requiredCategory = std::nullopt,
        BehaviorLookup lookup = BehaviorLookup::STRICT_FIELDS,
        const ParamTable* bullets = nullptr
    ) {
        MovesetImportReport report{motionCategory, behaviorVariation, catalog.size()};
        for (const auto& definition : catalog) {
            ResolutionResult result;
            try {
                const auto extraType = [&](int type) {
                    return definition.additionalEventSelections && std::ranges::any_of(*definition.additionalEventSelections,
                        [&](const auto& selection) { return selection.eventType == type; });
                };
                const auto& events = animationCache.get(definition.motionCategory.value_or(motionCategory),
                    definition.animationId, definition.eventType == 307 || extraType(307), definition.eventType == 2 || extraType(2));
                std::vector<AttackEvent> selectedEvents;
                if (definition.eventType && *definition.eventType != 1 && *definition.eventType != 2 && *definition.eventType != 304
                    && *definition.eventType != 307)
                    throw std::runtime_error("Unsupported attack event type filter");
                if (definition.additionalEventSelections && !definition.additionalEventSelections->empty()) {
                    result = resolveSelectedAttackEvents(events, definition, behaviorVariation, behaviors, attacks,
                        requiredCategory, lookup, bullets);
                } else {
                    if (definition.eventJudgeId || definition.eventType) {
                        std::ranges::copy_if(events, std::back_inserter(selectedEvents), [&](const auto& event) {
                            return (!definition.eventJudgeId || event.behaviorJudgeId == *definition.eventJudgeId)
                                && (!definition.eventType || event.eventType == *definition.eventType);
                        });
                        if (selectedEvents.empty())
                            throw std::runtime_error(std::format("No attack events for selected judge {} and event type {}",
                                definition.eventJudgeId ? std::to_string(*definition.eventJudgeId) : "any",
                                definition.eventType ? std::to_string(*definition.eventType) : "any"));
                    }
                    result = resolveAttackEvents((definition.eventJudgeId || definition.eventType) ? selectedEvents : events,
                        behaviorVariation, behaviors, attacks,
                        requiredCategory ? requiredCategory : definition.requiredCategory,
                        definition.requiredBehaviorParamId, lookup, bullets);
                }
            } catch (const std::runtime_error& error) {
                result.diagnostics.emplace_back(error.what());
            }

            if (result.diagnostics.empty() && result.hits.empty())
                result.diagnostics.emplace_back("No attack events");

            if (!result.diagnostics.empty()) {
                for (const auto& diagnostic : result.diagnostics) {
                    std::println(
                        stderr,
                        "{}: {}",
                        definition.name,
                        diagnostic
                    );
                }

                report.failures.push_back({definition.id, std::string(definition.name),
                    definition.animationId, std::move(result.diagnostics), std::move(result.unresolved)});

                // Don't publish a partially resolved move as complete.
                continue;
            }

            // The callback copies the resolved data into your OO model.
            addMove(definition, result.hits);
            ++report.importedMoves;
        }
        return report;
    }

    struct MovesetImportResult {
        std::shared_ptr<Moveset> moveset;
        MovesetImportReport report;
    };

    MovesetImportResult importMoveset(
        AnimationCache& animationCache,
        int motionCategory,
        int behaviorVariation,
        std::span<const MoveDefinition> catalog,
        const BehaviorIndex& behaviors,
        const ParamTable& attacks,
        std::optional<int> requiredCategory = std::nullopt,
        BehaviorLookup lookup = BehaviorLookup::STRICT_FIELDS,
        const ParamTable* bullets = nullptr
    ) {
        auto moveset = std::make_shared<Moveset>(motionCategory, behaviorVariation);
        auto report = importMoveset(animationCache, motionCategory, behaviorVariation,
            catalog, behaviors, attacks,
            [&](const MoveDefinition& definition, const std::vector<ResolvedHit>& hits) {
                moveset->moves.push_back({definition.id, std::string(definition.name), definition.animationId, hits});
            }, requiredCategory, lookup, bullets);
        return {std::move(moveset), std::move(report)};
    }

    std::map<long long, std::shared_ptr<Moveset>> importWeaponMovesets(
        const std::filesystem::path& root,
        const std::map<long long, std::map<std::string, double>>& weapons,
        std::vector<MovesetImportReport>* reports,
        const MovesetDefinitionResolver& resolveDefinitions
    ) {
        struct SupportedCatalog {
            int weaponType;
            int motionCategory;
            int baseBehaviorVariation;
            std::span<const MoveDefinition> moves;
            int enableThrow = 1;
            int paired = 0;
            int specialCategory = 0;
        };
        const std::array catalogs{
            SupportedCatalog{1, 20, 100, daggerCatalog},
            SupportedCatalog{3, 23, 200, straightSwordCatalog},
            SupportedCatalog{17, 30, 1400, axeCatalog},
            SupportedCatalog{21, 33, 1100, hammerCatalog},
            SupportedCatalog{24, 34, 1300, flailCatalog},
            SupportedCatalog{5, 25, 300, greatswordCatalog},
            SupportedCatalog{19, 32, 1500, greataxeCatalog},
            SupportedCatalog{23, 35, 1200, greathammerCatalog},
            SupportedCatalog{13, 29, 900, katanaCatalog},
            SupportedCatalog{11, 40, 800, curvedGreatswordCatalog},
            SupportedCatalog{7, 26, 400, colossalSwordCatalog},
            SupportedCatalog{41, 31, 2300, colossalWeaponCatalog},
            SupportedCatalog{29, 38, 1800, halberdCatalog},
            SupportedCatalog{31, 50, 1900, scytheCatalog},
            SupportedCatalog{25, 36, 1600, spearCatalog},
            SupportedCatalog{28, 37, 1700, greatSpearCatalog},
            SupportedCatalog{39, 43, 2000, whipCatalog, 0},
            SupportedCatalog{15, 27, 500, thrustingSwordCatalog},
            SupportedCatalog{9, 28, 700, curvedSwordCatalog},
            SupportedCatalog{94, 61, 6600, greatKatanaCatalog},
            SupportedCatalog{93, 60, 6700, lightGreatswordCatalog},
            SupportedCatalog{90, 57, 6100, thrustingShieldCatalog},
            SupportedCatalog{87, 21, 2400, torchCatalog, 0},
            SupportedCatalog{37, 22, 2200, clawCatalog, 1, 1},
            SupportedCatalog{35, 42, 2100, fistCatalog, 1, 1},
            SupportedCatalog{95, 62, 6800, beastClawCatalog, 1, 1},
            SupportedCatalog{92, 58, 6400, backhandBladeCatalog, 1, 1},
            SupportedCatalog{88, 55, 6300, martialArtsCatalog, 1, 1},
            SupportedCatalog{3, 23, 200, straightSword110Catalog, 1, 0, 110},
            SupportedCatalog{3, 23, 200, straightSword111Catalog, 1, 0, 111},
            SupportedCatalog{3, 23, 200, straightSword112Catalog, 1, 0, 112},
            SupportedCatalog{3, 23, 200, straightSword113Catalog, 1, 0, 113},
            SupportedCatalog{3, 23, 200, straightSword117Catalog, 1, 0, 117}
        };

        // Share complete validated families only when all routing inputs match.
        // Names/weapon IDs, attack power, scaling and passive status do not select
        // these animations or behavior rows. Special, catalyst and other paired
        // profiles require separate support; never inherit by category alone.
        // The behavior resolver already implements exact -> hundred-family ->
        // common lookup. Different exact variations must never share hit data.
        std::map<std::pair<std::size_t, int>, std::vector<long long>> supportedRows;
        for (const auto& [id, row] : weapons) {
            for (std::size_t i = 0; i < catalogs.size(); ++i) {
                const auto& catalog = catalogs[i];
                const auto matches = [&](const char* field, double expected) {
                    const auto found = row.find(field);
                    return found != row.end() && found->second == expected;
                };
                const auto variationField = row.find("behaviorVariationId");
                if (variationField == row.end() || !std::isfinite(variationField->second)
                    || variationField->second < catalog.baseBehaviorVariation
                    || variationField->second >= catalog.baseBehaviorVariation + 100
                    || std::trunc(variationField->second) != variationField->second) continue;
                const int variation = static_cast<int>(variationField->second);
                if (matches("wepmotionCategory", catalog.motionCategory) &&
                    matches("wepType", catalog.weaponType) &&
                    matches("wepmotionOneHandId", ordinaryWeaponMotionPosition(catalog.motionCategory, false)) &&
                    matches("wepmotionBothHandId", ordinaryWeaponMotionPosition(catalog.motionCategory, true)) &&
                    matches("spAtkcategory", catalog.specialCategory) && matches("isWeaponCatalyst", 0) &&
                    matches("isDualBlade", catalog.paired) && matches("enableThrow", catalog.enableThrow) &&
                    (catalog.paired == 0 || matches("bothHandEquipable", 1)))
                    supportedRows[{i, variation}].push_back(id);
            }
        }
        if (supportedRows.empty())
            return {};

        ParamTable attacks;
        attacks.load((root / AtkParam_PcFile).string().c_str());
        ParamTable behaviors;
        behaviors.load((root / BehaviorParam_PcFile).string().c_str());
        BehaviorIndex index;
        index.build(behaviors);
        AnimationCache cache(root);
        std::map<long long, std::shared_ptr<Moveset>> result;
        std::string failures;
        for (const auto& [key, rows] : supportedRows) {
            const auto [i, variation] = key;
            const auto& catalog = catalogs[i];
            // Keep the original constrained catalogs as regression references.
            // Production behavior selection follows player row addressing; it
            // no longer needs per-move category/behavior-ID tie breakers.
            std::vector<MoveDefinition> definitions(catalog.moves.begin(), catalog.moves.end());
            for (auto& definition : definitions) {
                definition.requiredCategory.reset();
                definition.requiredBehaviorParamId.reset();
            }
            MovesetImportResult imported{nullptr,
                {catalog.motionCategory, variation, definitions.size()}};
            try {
                if (resolveDefinitions) {
                    auto resolved = resolveDefinitions(rows, definitions);
                    if (resolved.size() != definitions.size() ||
                        !std::ranges::equal(resolved, definitions, {}, &MoveDefinition::id, &MoveDefinition::id))
                        throw std::runtime_error("Definition resolver must preserve complete catalog IDs and order");
                    definitions = std::move(resolved);
                }
                imported = importMoveset(cache, catalog.motionCategory, variation,
                    definitions, index, attacks, std::nullopt, BehaviorLookup::PLAYER_WEAPON);
            } catch (const std::runtime_error& error) {
                // A definition failure invalidates this entire supported set.
                // Keep the existing per-move report shape and collect all groups.
                for (const auto& definition : definitions)
                    imported.report.failures.push_back({definition.id, std::string(definition.name),
                        definition.animationId, {std::string("Definition resolution failed: ") + error.what()}, {}});
            }
            if (imported.report.failures.empty()) {
                for (const auto id : rows)
                    result.emplace(id, imported.moveset);
            } else {
                for (const auto& failure : imported.report.failures) {
                    for (const auto& diagnostic : failure.diagnostics)
                        failures += std::format("\nWeapon {}, motion {}, variation {}, {} ({:06}): {}",
                            rows.front(), catalog.motionCategory, variation,
                            failure.name, failure.animationId, diagnostic);
                }
            }
            if (reports)
                reports->push_back(std::move(imported.report));
        }
        // Retain every report before rejecting an incomplete supported import.
        if (!failures.empty())
            throw std::runtime_error("Could not import supported weapon movesets:" + failures);
        return result;
    }


    struct ExpectedHit {
        int judge;
        float physicalMV;
        std::optional<float> statusMV;

        // -1 means we have not established the expected source.
        int source = -1;

        // Optional expected TAE attack-window times.
        std::optional<double> start;
        std::optional<double> end;
    };

    void testAnimation(
        const std::filesystem::path& tempDir,
        const ParamTable& attacks,
        const BehaviorIndex& behaviorIndex,
        int motionCategory,
        int animationId,
        int behaviorVariation,
        const std::vector<ExpectedHit>& expected
    ) {
        const auto path =
            tempDir /
            "chr/c0000-anibnd-dcx-wanibnd/INTERROOT_win64/chr/c0000/tae" /
            std::format("a{}-tae", motionCategory) /
            std::format("anim-{:06}.xml", animationId);

        pugi::xml_document tae;

        if (auto loaded = tae.load_file(path.string().c_str()); !loaded) {
            throw std::runtime_error(
                std::format("Cannot load {}: {}", path.string(),
                            loaded.description()));
        }

        const auto events = extractAttackEvents(tae, animationId);

        const auto result = resolveAttackEvents(
            events, behaviorVariation, behaviorIndex, attacks
        );

        std::println("\nAnimation {:06}: {} events, {} resolved hits",
                    animationId, events.size(), result.hits.size());

        for (const auto& diagnostic : result.diagnostics)
            std::println("  DIAGNOSTIC: {}", diagnostic);

        if (!result.diagnostics.empty())
            throw std::runtime_error("Unexpected resolution diagnostics");

        if (result.hits.size() != expected.size())
            throw std::runtime_error("Unexpected hit count");

        constexpr auto physicalIndex =
            std::to_underlying(AttackPowerType::PHYSICAL);
        constexpr auto statusIndex =
            std::to_underlying(AttackPowerType::POISON);

        // Match expected hits by judge. This avoids assuming an event
        // order we haven't yet confirmed for every animation.
        std::vector<bool> matched(expected.size(), false);

        for (std::size_t i = 0; i < result.hits.size(); ++i) {
            const auto& hit = result.hits[i];

            if (i > 0 &&
                hit.event.startTime < result.hits[i - 1].event.startTime)
                throw std::runtime_error("Events are not in time order");

            const ExpectedHit* check = nullptr;

            for (std::size_t j = 0; j < expected.size(); ++j) {
                if (!matched[j] &&
                    expected[j].judge == hit.event.behaviorJudgeId) {
                    matched[j] = true;
                    check = &expected[j];
                    break;
                }
            }

            if (!check)
                throw std::runtime_error("Unexpected or duplicate judge");

            const auto& mv = hit.motionValues;

            if (!mv.known[physicalIndex] ||
                std::abs(mv.values[physicalIndex] -
                        check->physicalMV) > 0.001f)
                throw std::runtime_error("Physical MV mismatch");

            if (check->statusMV) {
                if (!mv.known[statusIndex] ||
                    std::abs(mv.values[statusIndex] -
                            *check->statusMV) > 0.001f)
                    throw std::runtime_error("Status MV mismatch");
            }

            if (check->source >= 0 &&
                hit.event.source != check->source)
                throw std::runtime_error("Attack source mismatch");

            if (check->start &&
                std::abs(hit.event.startTime - *check->start) > 0.0001)
                throw std::runtime_error("Start time mismatch");

            if (check->end &&
                std::abs(hit.event.endTime - *check->end) > 0.0001)
                throw std::runtime_error("End time mismatch");

            std::println(
                "  hit {}: judge={}, source={}, time={:.3f}-{:.3f}, "
                "physical={}, status={}",
                i + 1,
                hit.event.behaviorJudgeId,
                hit.event.source,
                hit.event.startTime,
                hit.event.endTime,
                mv.values[physicalIndex],
                mv.values[statusIndex]
            );
        }

        std::println("  PASS");
    }



    std::expected<void, std::string> run_witchy(const std::filesystem::path& witchy_exe_path, const std::filesystem::path& arg)
    {
        auto cmd = std::format("\"{} --passive --parallel {}\"", witchy_exe_path, arg);

        std::println("executing command: {}", cmd);
        auto result = std::system(cmd.c_str());
        if (result != 0)
            return std::unexpected(std::format("WitchyBND failed with exit code {} for file {}", result, arg));

        return {};
    }

    std::expected<GameData, std::string> generate_game_data(
        const std::filesystem::path& elden_ring_exe_path,
        const std::filesystem::path& witchy_exe_path,
        bool delete_temp_directory
    ) try
    {
        if (!std::filesystem::is_regular_file(elden_ring_exe_path))
            return std::unexpected(std::format("Not a file: {}", elden_ring_exe_path));
        const auto input_files_directory = elden_ring_exe_path.parent_path();

        if (!std::filesystem::is_regular_file(witchy_exe_path))
            return std::unexpected(std::format("Not a file: {}", witchy_exe_path));
        
        // set up temporary directory structure
        const auto temp_dir = std::filesystem::temp_directory_path() / "elden-ring-damage-optimizer";
        std::filesystem::remove_all(temp_dir);
        std::filesystem::create_directory(temp_dir);
        const auto unpacked_data_directory = temp_dir / "unpacked_data";
        std::filesystem::create_directory(unpacked_data_directory);
        std::println("created temporary directory {}", temp_dir);

        // copy elden ring executable file to temporary directory
        std::filesystem::copy_file(elden_ring_exe_path, unpacked_data_directory / elden_ring_exe_path.filename());
        std::println("copied elden ring executable file {} to {}", elden_ring_exe_path, unpacked_data_directory / elden_ring_exe_path.filename());

        // iterate over needed packed files
        for (auto&& needed_packed_file : needed_packed_files)
        {
            const auto needed_packed_file_path = input_files_directory / needed_packed_file;

            // copy needed packed file
            const auto packed_file_path = unpacked_data_directory / needed_packed_file;
            std::filesystem::create_directories(packed_file_path.parent_path());
            std::filesystem::copy_file(needed_packed_file_path, packed_file_path);
            std::println("copied packed file {} to {}", needed_packed_file_path, packed_file_path);

            // unpack packed file in-place
            const auto witchy_result = run_witchy(witchy_exe_path, packed_file_path);
            if (!witchy_result)
                return std::unexpected(witchy_result.error());
            std::println("unpacked packed file {} in-place", packed_file_path);
        }

        // assert that all needed unpacked files are present
        for (auto&& needed_unpacked_file : needed_unpacked_files)
        {
            const auto needed_unpacked_path = unpacked_data_directory / needed_unpacked_file;
            if (!std::filesystem::is_regular_file(needed_unpacked_path))
                return std::unexpected(std::format("could not find unpacked file: {}", needed_unpacked_path));
        }

        // parse xml files
        auto game_data = load_game_data(unpacked_data_directory);
        std::println("\nsuccessfully unpacked and parsed game data in {}", unpacked_data_directory);

        // potentially remove temporary directory
        if (delete_temp_directory)
        {
            std::filesystem::remove_all(temp_dir);
            std::println("removed temporary directory {}", temp_dir);
        }

        return game_data;
    }
    catch (const std::exception& e)
    {
        return std::unexpected(e.what());
    }
}
