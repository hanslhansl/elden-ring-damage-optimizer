export module erdo:witchy;
import :xml;

import std;

template <typename CharT>
struct std::formatter<std::filesystem::path, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
    template <typename FormatContext>
    auto format(const std::filesystem::path& p, FormatContext& ctx) const {
        if constexpr (std::same_as<CharT, char>)
        {
            auto s = p.string();
            return std::formatter<std::basic_string_view<char>, char>::format(s, ctx);
        }
        else if constexpr (std::same_as<CharT, wchar_t>)
        {
            auto s = p.wstring();
            return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format(s, ctx);
        }
        else
        {
            static_assert(sizeof(CharT) == 0, "Unsupported character type for filesystem::path formatter");
        }
    }
};

std::string quote_path(const std::filesystem::path& p)
{
    return std::format("\"{}\"", std::filesystem::canonical(p).make_preferred());
}

namespace erdo::witchy
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

    export {
        const std::filesystem::path AttackElementCorrectParamFile = "AttackElementCorrectParam.param";
        const std::filesystem::path CalcCorrectGraphFile = "CalcCorrectGraph.param";
        const std::filesystem::path EquipParamWeaponFile = "EquipParamWeapon.param";
        const std::filesystem::path ReinforceParamWeaponFile = "ReinforceParamWeapon.param";
        const std::filesystem::path SpEffectParamFile = "SpEffectParam.param";
        const std::filesystem::path MenuValueTableParamFile = "MenuValueTableParam.param";
        const std::filesystem::path WeaponNameFile = "WeaponName.fmg";
        const std::filesystem::path WeaponName_dlc01File = "WeaponName_dlc01.fmg";
        const std::filesystem::path GR_MenuTextFile = "GR_MenuText.fmg";
    }

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

    std::string witchy_cmd(const std::filesystem::path& witchy_exe_path, std::ranges::range auto&& copied_uxm_file_paths, std::filesystem::path location = {}) {
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

    export void run_witchy(const std::filesystem::path& unpacked_uxm_files_directory, const std::filesystem::path& witchy_exe_path, const std::filesystem::path& save_to_directory) {

        // create temporary directory
        auto temp_dir = std::filesystem::temp_directory_path() / "elden-ring-damage-optimizer";
        std::filesystem::create_directory(temp_dir);

        // copy needed uxm files to temporary directory
        auto copied_uxm_file_paths = needed_uxm_files
            | std::views::transform([&](const std::filesystem::path &uxm_file) {
                auto uxm_file_path = unpacked_uxm_files_directory / uxm_file;
                auto uxm_file_temp_path = temp_dir / uxm_file.filename();
                std::filesystem::copy_file(uxm_file_path, uxm_file_temp_path, std::filesystem::copy_options::overwrite_existing);
                return uxm_file_temp_path;
            })
            | std::ranges::to<std::vector>();
        std::println("copied needed uxm files to temporary directory {}", quote_path(temp_dir));

        // unpack uxm files inplace (temporary directory)
        auto result = std::system(witchy_cmd(witchy_exe_path, copied_uxm_file_paths).c_str());
        if (result != 0)
            throw std::runtime_error(std::format("WitchyBND failed with exit code {}", result));
        std::println("unpacked uxm files to temporary directory {}", quote_path(temp_dir));
            
        // get version from /regulation-bin/_witchy-bnd4.xml
        auto version = xml::get_element_value<std::string>(
            xml::load_file(temp_dir / "regulation-bin" / "_witchy-bnd4.xml"),
            { "bnd4", "version" }
        );
        std::println("version {}", version);    

        // compute paths of needed unpacked files
        auto needed_unpacked_file_paths = copied_uxm_file_paths
            | std::views::transform([&](const std::filesystem::path& p){
                auto filename = p.filename().string();
                std::ranges::replace(filename, '.', '-');
                return std::filesystem::directory_iterator(temp_dir / filename);
            })
            | std::views::join
            | std::views::transform(&std::filesystem::directory_entry::path)
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

        // convert needed unpacked files to xml
        auto xml_files_directory = temp_dir / "xml_data";
        std::filesystem::create_directory(xml_files_directory);
        result = std::system(witchy_cmd(witchy_exe_path, needed_unpacked_file_paths, xml_files_directory).c_str());
        if (result != 0)
            throw std::runtime_error(std::format("WitchyBND failed with exit code {}", result));
        std::println("converted needed unpacked files to xml in {}", quote_path(xml_files_directory));

        // copy paths of needed xml files
        auto xml_file_paths = needed_unpacked_file_paths
            | std::views::transform([&](const std::filesystem::path& p){ return xml_files_directory / p.filename() += ".xml"; })
            | std::ranges::to<std::vector>();

        // copy xml files to save_to_directory
        auto full_save_to_directory = save_to_directory / version;
        std::filesystem::create_directories(full_save_to_directory);
        std::filesystem::copy(xml_files_directory, full_save_to_directory, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        std::println("copied xml files to {}", quote_path(full_save_to_directory));

        // remove temporary directory
        std::filesystem::remove_all(temp_dir);
        std::println("removed temporary directory {}", quote_path(temp_dir));

        std::println("\nsuccessfully unpacked and converted uxm files to xml in {}", quote_path(full_save_to_directory));
    }
}
