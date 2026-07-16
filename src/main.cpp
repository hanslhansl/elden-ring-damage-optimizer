
#include <QCoreApplication>
#include <QDebug>
#include <QApplication>
#include <QLabel>

import std;
import erdo;
import erdo.ui;
import BS.thread_pool;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ui::MainWindow window{};
    window.show();

    return app.exec();
}


int main2(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data";

    auto weapons = xml::load_weapons(xml_data_directory);

    auto stat_variations = calculator::get_stat_variations(
        1 + 60,
        calculator::character_class_stats.at("wretch").to_stats()
    );

    calculator::AttackOptions attack_options{ {0, 25, 10}, true };
    BS::thread_pool<> thread_pool{ 1 };
    std::vector<calculator::AttackRating> attack_ratings = optimizer::optimize<optimizer::optimize_weapon>(weapons, stat_variations, attack_options, thread_pool).get();
    auto&& attack_rating = attack_ratings.front();

    std::println("{}", attack_rating.weapon.get().full_name);

    //calculator::AttackOptions attack_options{{0, 25, 10}, true};
    //calculator::Stats stats{ 21, 10, 10, 10, 10 };

    //auto total_attack_powers = weapons | std::views::transform([&](const calculator::Weapon& w){
    //    return w.get_attack_rating(attack_options, stats).total_attack_power.at(2);
    //}) | std::ranges::to<std::vector>();
    //std::println("{}", total_attack_powers);


    // witchy::run_witchy(
    //     ".../Steam/steamapps/common/ELDEN RING/Game",
    //     ".../WitchyBND-v3.0.0.1-win-x64/WitchyBND.exe",
    //     xml_data_directory
    // );

    return 0;
}