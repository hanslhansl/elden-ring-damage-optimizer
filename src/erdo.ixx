export module erdo;

export import :meta;
export import :witchy;
export import :calculator;
export import :xml;



extern "C++" int main(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data";

    auto regulation_file = std::filesystem::current_path().parent_path() / "regulation_data_current_game_current_erdo.json";
    auto weap_contain = calculator::WeaponContainer(regulation_file);

    auto new_weap_contain = xml::WeaponContainer(xml_data_directory);


    for (auto&& [w1, w2] : std::views::zip(weap_contain.weapons, new_weap_contain.weapons))
    {
        if (w1 != w2)
            throw std::runtime_error("WeaponContainers are not equal");
    }


        
    // test1();

    // test2();

    // witchy::run_witchy(
    //     "F:/Programme/Steam/steamapps/common/ELDEN RING/Game",
    //     "C:/Users/Paul/Downloads/WitchyBND-v3.0.0.1-win-x64/WitchyBND.exe",
    //     xml_data_directory
    // );

    return 1;
}