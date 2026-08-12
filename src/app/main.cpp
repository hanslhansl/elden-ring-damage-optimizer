import std;

import erdo;
import erdo.ui;
int main(int argc, char *argv[])
{
	return erdo::ui::run_ui(argc, argv);

	// auto xml_data_directory = std::filesystem::current_path() / "xml_data" / "11611000";
    // auto&& weapons = erdo::parser::load_weapons(xml_data_directory);
	
    // erdo::calculator::AttackOptions attack_options{{0, 25, 10}, true};
    // const auto min_stats = erdo::calculator::character_class_stats.at("wretch");
    // const auto min_relevant_stats = min_stats.relevant_stats();
    // const auto free_attribute_points = 11;

    // constexpr auto optimizer = erdo::optimizer::V2<erdo::optimizer::Target::TOTAL_ATTACK_POWER>{};
	// auto attacks = optimizer.run_synchronously(weapons, attack_options, free_attribute_points, min_stats);

	// std::println("{}", attacks.back().weapon.get().full_name);
	// return 0;
}
