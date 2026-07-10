
import std;
import erdo;


int main(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data";

    auto weapons = xml::get_weapons(xml_data_directory);

    calculator::AttackOptions attack_options{{0, 25, 10}, true};
    calculator::Stats stats{ 21, 10, 10, 10, 10 };

    auto total_attack_powers = weapons | std::views::transform([&](const calculator::Weapon& w){
        return w.get_attack_rating(attack_options, stats).total_attack_power.at(2);
    }) | std::ranges::to<std::vector>();
    // std::println("{}", total_attack_powers);


    // witchy::run_witchy(
    //     "F:/Programme/Steam/steamapps/common/ELDEN RING/Game",
    //     "C:/Users/Paul/Downloads/WitchyBND-v3.0.0.1-win-x64/WitchyBND.exe",
    //     xml_data_directory
    // );

    return 1;
}