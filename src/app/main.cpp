import std;

//import erdo.ui;
//int main(int argc, char *argv[])
//{
//    return erdo::ui::run_ui(argc, argv);
//}

#include <tracy/Tracy.hpp>
import erdo;

int main(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data" / "11611000";

    auto w = erdo::parser::load_weapons(xml_data_directory);
    std::vector<std::reference_wrapper<const erdo::calculator::Weapon>> weapons{ std::from_range, w };

    // while(running)
    {
        FrameMark;
        auto stat_variations = erdo::calculator::get_stat_variations(
            120,
            erdo::calculator::character_class_stats.at("wretch")
        );
        erdo::calculator::AttackOptions attack_options{ {0, 25, 10}, true };

        constexpr auto optimizer = erdo::calculator::optimizers<erdo::calculator::OptimizationTarget::TOTAL_ATTACK_POWER>;

        volatile auto attack_ratings = optimizer.run_synchronously(weapons, stat_variations, attack_options);
    }

    
    

    return 0;
}