
import std;
import erdo;


int main(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data";

    auto regulation_file = std::filesystem::current_path().parent_path() / "regulation_data_current_game_current_erdo.json";
    auto weapon_container = calculator::WeaponContainer(regulation_file);
    std::ranges::sort(weapon_container.weapons, {}, &calculator::Weapon::full_name);

    auto new_weapons = xml::get_weapons(xml_data_directory);

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };
    auto total_attack_powers = new_weapons | std::views::transform([&](const calculator::Weapon& w){
        calculator::AttackRating::total attack_rating{};
        w.get_attack_rating(attack_options, stats, attack_rating);
        return attack_rating.total_attack_power;
    }) | std::ranges::to<std::vector>();
    std::println("{}", total_attack_powers);

    for (auto&& [w1, w2] : std::views::zip(weapon_container.weapons, new_weapons))
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