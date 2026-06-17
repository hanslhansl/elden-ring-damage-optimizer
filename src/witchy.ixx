module;

export module witchy;

import std;

std::string quote_path(const std::filesystem::path& p)
{
    return std::format("\"{}\"", p);
}

namespace witchy
{
    const std::set<std::filesystem::path> needed_uxm_files = {
        "regulation.bin",
        "msg/engus/menu.msgbnd.dcx",
        "msg/engus/menu_dlc01.msgbnd.dcx",
        "msg/engus/menu_dlc02.msgbnd.dcx",
        "msg/engus/item.msgbnd.dcx",
        "msg/engus/item_dlc01.msgbnd.dcx",
        "msg/engus/item_dlc02.msgbnd.dcx"
    };

    const std::filesystem::path attackElementCorrectFile = "AttackElementCorrectParam.param";
    const std::filesystem::path calcCorrectGraphFile = "CalcCorrectGraph.param";
    const std::filesystem::path equipParamWeaponFile = "EquipParamWeapon.param";
    const std::filesystem::path reinforceParamWeaponFile = "ReinforceParamWeapon.param";
    const std::filesystem::path spEffectFile = "SpEffectParam.param";
    const std::filesystem::path menuValueTableFile = "MenuValueTableParam.param";
    const std::filesystem::path weaponNameFmgFile = "WeaponName.fmg";
    const std::filesystem::path dlcWeaponNameFmgFile = "WeaponName_dlc01.fmg";
    const std::filesystem::path menuTextFmgFile = "GR_MenuText.fmg";

    const std::set<std::filesystem::path> needed_unpacked_files = {
        attackElementCorrectFile,
        calcCorrectGraphFile,
        equipParamWeaponFile,
        reinforceParamWeaponFile,
        spEffectFile,
        menuValueTableFile,
        weaponNameFmgFile,
        dlcWeaponNameFmgFile,
        menuTextFmgFile
    };

    std::string witchy_cmd(const std::filesystem::path& witchy_exe_path, std::ranges::range auto&& copied_uxm_file_paths, std::filesystem::path location = {})
    {
        auto s = std::format("\"{} --passive --parallel", quote_path(witchy_exe_path));
        if (!location.empty())
            s += std::format(" --location {}", quote_path(location));

        return s + std::format(" {}\"",
            copied_uxm_file_paths
                | std::views::transform(quote_path)
                | std::views::join_with(std::string{" "})
                | std::ranges::to<std::string>()
        );
    }

    export void run_witchy(const std::filesystem::path& unpacked_uxm_files_directory, const std::filesystem::path& witchy_exe_path)
    {

        auto temp_dir = std::filesystem::temp_directory_path() / "elden-ring-damage-optimizer";
        std::filesystem::create_directory(temp_dir);

        auto copied_uxm_file_paths = needed_uxm_files
            | std::views::transform([&](const std::filesystem::path &uxm_file) {
                auto uxm_file_path = unpacked_uxm_files_directory / uxm_file;
                auto uxm_file_temp_path = temp_dir / uxm_file.filename();
                std::filesystem::copy_file(uxm_file_path, uxm_file_temp_path, std::filesystem::copy_options::overwrite_existing);
                return uxm_file_temp_path;
            })
            | std::ranges::to<std::vector>();
        std::println("copied needed uxm files to temporary directory {}", quote_path(temp_dir));

        // unpack uxm files
        auto cmd1 = witchy_cmd(witchy_exe_path, copied_uxm_file_paths);
        auto result1 = std::system(cmd1.c_str());
        if (result1 != 0)
            throw std::runtime_error(std::format("WitchyBND failed with exit code {}", result1));
        std::println("unpacked uxm files to temporary directory {}", quote_path(temp_dir));
            
        auto needed_unpacked_file_paths = copied_uxm_file_paths
            | std::views::transform([&](const std::filesystem::path& p){
                auto filename = p.filename().string();
                std::ranges::replace(filename,'.', '-');
                return std::filesystem::directory_iterator(temp_dir / filename);
            })
            | std::views::join
            | std::views::transform(std::filesystem::directory_entry::path)
            | std::views::filter([](const std::filesystem::path& p){ return needed_unpacked_files.contains(p.filename()); })
            | std::ranges::to<std::set>([](const std::filesystem::path& l, const std::filesystem::path& r){ return std::less{}(l.filename(), r.filename()); });
        if (needed_unpacked_file_paths.size() != needed_unpacked_files.size())
            throw std::runtime_error(
                std::format(
                    "only {} out of {} needed unpacked files were found in the temporary directory {}",
                    needed_unpacked_file_paths.size(),
                    needed_unpacked_files.size(),
                    quote_path(temp_dir)
                )
            );
        std::println("found {} needed unpacked files in the temporary directory {}", needed_unpacked_file_paths.size(), quote_path(temp_dir));

        // convert to xml
        auto xml_files_directory = temp_dir / "xml_files";
        std::filesystem::create_directory(xml_files_directory);
        auto cmd2 = witchy_cmd(witchy_exe_path, needed_unpacked_file_paths, xml_files_directory);
        auto result2 = std::system(cmd2.c_str());
        if (result2 != 0)
            throw std::runtime_error(std::format("WitchyBND failed with exit code {}", result2));
        std::println("converted needed unpacked files to xml in {}", quote_path(xml_files_directory));

        auto xml_file_paths = needed_unpacked_file_paths
            | std::views::transform([&](const std::filesystem::path& p){ return xml_files_directory / p.filename() + ".xml"; })
            | std::ranges::to<std::vector>();

        // this->attackElementCorrectParams = read_param_xml(xml_directory / attackElementCorrectFile);
        // this->calcCorrectGraphs = read_param_xml(xml_directory / calcCorrectGraphFile);
        // this->equipParamWeapons = read_param_xml(xml_directory / equipParamWeaponFile);
        // this->reinforceParamWeapons = read_param_xml(xml_directory / reinforceParamWeaponFile);
        // this->spEffectParams = read_param_xml(xml_directory / spEffectFile);
        // this->menuValueTableParams = read_param_xml(xml_directory / menuValueTableFile);
        // this->menuText = read_fmg_xml(xml_directory / menuTextFmgFile);
        // this->weaponNames = read_fmg_xml(xml_directory / weaponNameFmgFile);
        // this->dlcWeaponNames = read_fmg_xml(xml_directory / dlcWeaponNameFmgFile);

        // std::filesystem::remove_all(temp_dir);
    }
}



// extern "C++" int main() {
//     std::vector<std::filesystem::path> v = {"apple/foo", "banana", "cherry"};

//     auto v2 = v
//         | std::views::transform([](std::filesystem::path& p) { return "\"" + p.string() + "\""; })
//         | std::views::join_with(std::string{" "})
//         | std::ranges::to<std::string>();

//     std::cout << v2 << '\n';
// }