#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>

import std;
import erdo;
import erdo.ui;
import erdo.ui.settings;
import erdo.ui.weapons_table;

using namespace erdo;
using namespace erdo::calculator;

namespace {
struct GuiEnvironment {
    int argc = 1;
    char name[16] = "erdo-ui-tests";
    char* argv[2] = {name, nullptr};
    std::unique_ptr<QApplication> application;
    QTemporaryDir settings_directory;

    GuiEnvironment()
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        application = std::make_unique<QApplication>(argc, argv);
        QCoreApplication::setOrganizationName("erdo-tests");
        QCoreApplication::setApplicationName("moves");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_directory.path());
        ui::settings.initialize();
    }
    ~GuiEnvironment()
    {
        // Destroy settings widgets while QApplication is still alive.
        std::destroy_at(&ui::settings);
        std::construct_at(&ui::settings, 1);
    }
};

void initialize_gui()
{
    static GuiEnvironment environment;
}

template<class T> T* widget(QWidget& parent, const char* name)
{
    auto* result = parent.findChild<T*>(QString::fromLatin1(name));
    REQUIRE(result);
    return result;
}

std::shared_ptr<GameData> game_data(bool with_moves = true)
{
    auto curves = std::make_shared<ScalingCurve>();
    curves->fill(1.);
    ScalingCurves scaling_curves;
    scaling_curves.fill(curves);
    Weapon weapon{
        .full_name = "First", .base_name = "First", .dlc = 1,
        .type = Weapon::Type::STRAIGHT_SWORD,
        .attribute_scalings_at_upgrade_levels = {{0, 0, 0, 0, 0}},
        .base_attack_powers_at_upgrade_levels = {{100}},
        .attack_power_types_attribute_scalings = std::make_shared<AttackElementCorrects>(),
        .attack_power_scaling_curves = scaling_curves,
        .scaling_tiers = std::make_shared<ScalingTiers>()
    };
    ResolvedHit hit;
    hit.event = {30000, 1, 0, 0, 0, .1, .2};
    hit.motionValues.known.fill(true);
    hit.motionValues.values[0] = 150;
    auto secondary = hit;
    secondary.event.source = 2;
    if (with_moves) {
        weapon.moveset = std::make_shared<Moveset>(23, 200);
        weapon.moveset->moves = {
            {MoveId::ONE_HAND_R1_1, "Light attack", 30000, {hit, hit}},
            {MoveId::POWERSTANCE_L1_1, "Powerstance", 34000, {secondary, hit}}
        };
    }
    auto second = weapon;
    second.full_name = second.base_name = "Second";
    second.dlc = 2;
    if (with_moves) {
        second.moveset = std::make_shared<Moveset>(*weapon.moveset);
        second.moveset->moves[0].hits.pop_back();
    }
    return std::make_shared<GameData>("test", 11711000, std::vector{weapon, second});
}

void select_move(QComboBox* combo, MoveId id)
{
    const auto index = combo->findData(std::to_underlying(id));
    REQUIRE(index > 0);
    combo->setCurrentIndex(index);
}
}

TEST_CASE("UI moves - stats hits and game data replacement", "[moveset][ui]")
{
    initialize_gui();
    ui::StatsTab tab;
    tab.set_active_game_data(game_data());
    auto* moves = widget<QComboBox>(tab, "moveSelection");
    auto* hits = widget<QSpinBox>(tab, "moveHitSelection");
    auto* pair = widget<QCheckBox>(tab, "matchingPowerstance");
    auto* results = widget<QTableWidget>(tab, "moveResults");
    // Select by source row, independent of the view's current sort.
    tab.weapon_table->setCurrentIndex(tab.weapon_table->proxy_model->mapFromSource(
        tab.weapon_table->model->index(0, 0)));
    select_move(moves, MoveId::ONE_HAND_R1_1);
    REQUIRE(results->rowCount() == 3);
    CHECK(results->item(0, 2)->text().toDouble() == 300);
    hits->setValue(2);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Hit 2");
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    select_move(moves, MoveId::POWERSTANCE_L1_1);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Unsupported");
    hits->setValue(2); // The selected right-hand record needs no second weapon.
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    hits->setValue(0);
    pair->setChecked(true);
    REQUIRE(results->rowCount() == 3);
    CHECK(results->item(0, 2)->text().toDouble() == 300);

    auto* dlcs = widget<QListWidget>(tab, "base_game_dlc_list");
    REQUIRE(dlcs->count() == 2);
    dlcs->item(1)->setSelected(true);
    CHECK(tab.weapon_table->proxy_model->rowCount() == 1);
    CHECK(dlcs->item(1)->data(Qt::UserRole).toInt() == 2);
    tab.set_active_game_data(game_data(false));
    CHECK_FALSE(moves->isEnabled());
    CHECK(results->rowCount() == 0);
}

TEST_CASE("UI moves - selected direct hit survives an unsupported Bullet sibling", "[moveset][ui]")
{
    initialize_gui();
    auto data = game_data();
    auto& bullet = data->weapons[0].moveset->moves[0].hits[1];
    bullet.bulletParamId = 200031311;
    bullet.bulletParameters.emplace();
    const auto restored = std::make_shared<GameData>(json::read<GameData>(json::write(*data)));
    ui::StatsTab stats;
    stats.set_active_game_data(restored);
    stats.weapon_table->setCurrentIndex(stats.weapon_table->proxy_model->mapFromSource(
        stats.weapon_table->model->index(0, 0)));
    auto* moves = widget<QComboBox>(stats, "moveSelection");
    auto* hits = widget<QSpinBox>(stats, "moveHitSelection");
    auto* results = widget<QTableWidget>(stats, "moveResults");
    select_move(moves, MoveId::ONE_HAND_R1_1);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Unsupported");
    CHECK(results->item(0, 2)->text().contains("Bullet 200031311"));
    hits->setValue(1);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Hit 1");
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    CHECK(results->item(0, 3)->text().toDouble() == 150);
    hits->setValue(2);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Hit 2");
    CHECK(results->item(0, 2)->text().contains("Bullet 200031311"));
    CHECK(results->item(0, 3) == nullptr);
    hits->setValue(0);
    CHECK(results->item(0, 1)->text() == "Unsupported");

    ui::OptimizeTab optimize;
    optimize.set_active_game_data(restored);
    select_move(widget<QComboBox>(optimize, "moveSelection"), MoveId::ONE_HAND_R1_1);
    auto* count = widget<QLabel>(optimize, "weapons_label");
    CHECK(count->text() == "1"); // Only the other weapon has a complete move.
    auto* optimizeHits = widget<QSpinBox>(optimize, "moveHitSelection");
    optimizeHits->setValue(1);
    CHECK(count->text() == "2");
    optimizeHits->setValue(2);
    CHECK(count->text() == "0");
}

TEST_CASE("UI moves - unknown activation condition", "[moveset][ui]")
{
    initialize_gui();
    auto data = game_data();
    for (auto& weapon : data->weapons)
        weapon.moveset->moves[0].hits[0].event.stateInfo = 188;
    const auto restored = std::make_shared<GameData>(json::read<GameData>(json::write(*data)));
    ui::StatsTab stats;
    stats.set_active_game_data(restored);
    stats.weapon_table->setCurrentIndex(stats.weapon_table->proxy_model->mapFromSource(
        stats.weapon_table->model->index(0, 0)));
    select_move(widget<QComboBox>(stats, "moveSelection"), MoveId::ONE_HAND_R1_1);
    auto* results = widget<QTableWidget>(stats, "moveResults");
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Unsupported");
    CHECK(results->item(0, 2)->text().contains("StateInfo condition 188"));
    auto* hits = widget<QSpinBox>(stats, "moveHitSelection");
    hits->setValue(2);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 1)->text() == "Hit 2");
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    hits->setValue(1);
    CHECK(results->item(0, 2)->text().contains("StateInfo condition 188"));
    ui::OptimizeTab optimize;
    optimize.set_active_game_data(restored);
    select_move(widget<QComboBox>(optimize, "moveSelection"), MoveId::ONE_HAND_R1_1);
    CHECK(widget<QLabel>(optimize, "weapons_label")->text() == "0");
    widget<QSpinBox>(optimize, "moveHitSelection")->setValue(2);
    CHECK(widget<QLabel>(optimize, "weapons_label")->text() == "1");
}

TEST_CASE("UI moves - optimizer eligibility and scores", "[moveset][ui]")
{
    initialize_gui();
    ui::OptimizeTab tab;
    tab.set_active_game_data(game_data());
    auto* moves = widget<QComboBox>(tab, "moveSelection");
    auto* hits = widget<QSpinBox>(tab, "moveHitSelection");
    auto* pair = widget<QCheckBox>(tab, "matchingPowerstance");
    auto* count = widget<QLabel>(tab, "weapons_label");
    auto* target = widget<QComboBox>(tab, "target_combobox");
    auto* stance = widget<QCheckBox>(tab, "two_handing_checkbox");
    auto* targets = qobject_cast<QStandardItemModel*>(target->model());
    REQUIRE(targets);
    target->setCurrentIndex(std::to_underlying(optimizer::Target::SPELL_SCALING));
    select_move(moves, MoveId::ONE_HAND_R1_1);
    CHECK(target->currentIndex() == std::to_underlying(optimizer::Target::TOTAL_ATTACK_POWER));
    CHECK_FALSE(targets->item(std::to_underlying(optimizer::Target::SPELL_SCALING))->isEnabled());
    CHECK_FALSE(stance->isEnabled());
    CHECK(count->text() == "2");
    hits->setValue(2);
    CHECK(count->text() == "1");
    select_move(moves, MoveId::POWERSTANCE_L1_1);
    CHECK(count->text() == "0");
    pair->setChecked(true);
    CHECK(count->text() == "2");

    // Both buttons use the same objective and publish its score separately from AR.
    for (const auto* button : {"start_v2_button", "start_brute_force_button"}) {
        widget<QPushButton>(tab, button)->click();
        REQUIRE(tab.weapon_table->model->rows.size() == 2);
        for (const auto& row : tab.weapon_table->model->rows) {
            CHECK(std::get<ui::sections::MoveScore>(row).data(0, Qt::UserRole).toDouble() == 300);
            CHECK(row.attack.total_attack_power[1] == 100);
        }
    }
    tab.set_active_game_data(game_data(false));
    CHECK_FALSE(moves->isEnabled());
    CHECK(stance->isEnabled());
    CHECK(targets->item(std::to_underlying(optimizer::Target::SPELL_SCALING))->isEnabled());
    CHECK(tab.weapon_table->model->rows.empty());
    CHECK(count->text() == "2");
}

TEST_CASE("UI moves - combat environment changes powers and optimizer scores", "[moveset][ui]")
{
    initialize_gui();
    auto data = game_data();
    for (auto& weapon : data->weapons)
        weapon.moveset->moves[0].hits[0].event.stateInfo = 187;
    const auto restored = std::make_shared<GameData>(json::read<GameData>(json::write(*data)));
    ui::StatsTab stats;
    stats.set_active_game_data(restored);
    stats.weapon_table->setCurrentIndex(stats.weapon_table->proxy_model->mapFromSource(
        stats.weapon_table->model->index(0, 0)));
    select_move(widget<QComboBox>(stats, "moveSelection"), MoveId::ONE_HAND_R1_1);
    auto* environment = widget<QComboBox>(stats, "moveEnvironment");
    auto* results = widget<QTableWidget>(stats, "moveResults");
    REQUIRE(results->rowCount() == 3);
    CHECK(environment->currentIndex() == 0);
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    CHECK(results->item(1, 1)->text() == "Hit 1 (inactive)");
    CHECK(results->item(1, 2)->text().toDouble() == 0);
    environment->setCurrentIndex(1);
    CHECK(results->item(0, 2)->text().toDouble() == 300);
    CHECK(results->item(1, 1)->text() == "Hit 1");
    CHECK(results->item(1, 2)->text().toDouble() == 150);
    widget<QSpinBox>(stats, "moveHitSelection")->setValue(1);
    environment->setCurrentIndex(0);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 2)->text().toDouble() == 0);
    environment->setCurrentIndex(1);
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    // Each tab owns its context. Both search paths use the selected one.
    ui::OptimizeTab optimize;
    optimize.set_active_game_data(restored);
    select_move(widget<QComboBox>(optimize, "moveSelection"), MoveId::ONE_HAND_R1_1);
    widget<QSpinBox>(optimize, "moveHitSelection")->setValue(1);
    auto* optimize_environment = widget<QComboBox>(optimize, "moveEnvironment");
    for (const int choice : {0, 1}) {
        optimize_environment->setCurrentIndex(choice);
        CHECK(widget<QLabel>(optimize, "weapons_label")->text() == "2");
        for (const auto* button : {"start_v2_button", "start_brute_force_button"}) {
            widget<QPushButton>(optimize, button)->click();
            REQUIRE(optimize.weapon_table->model->rows.size() == 2);
            for (const auto& row : optimize.weapon_table->model->rows)
                CHECK(std::get<ui::sections::MoveScore>(row).data(0, Qt::UserRole).toDouble() == (choice ? 150 : 0));
        }
    }
}

#ifdef ERDO_MOVESET_XML_ROOT
TEST_CASE("UI moves - XML through JSON and application", "[moveset][ui][game_xml]")
{
    initialize_gui();
    const auto imported = witchy::load_game_data(ERDO_MOVESET_XML_ROOT);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto file = std::filesystem::path(directory.path().toStdString()) / "11711000.json";
    json::save(file, imported);

    ui::MainWindow window;
    QApplication::processEvents(); // Complete the normal startup game-data selection.
    auto* action = window.add_game_data_from_path(file);
    REQUIRE(action);
    action->trigger(); // Exercise the same asynchronous JSON load and tab updates as the menu.
    CHECK(action->isChecked());
    auto* tabs = widget<QTabWidget>(window, "tab_widget");
    auto& stats = *static_cast<ui::StatsTab*>(tabs->widget(0));
    auto& optimize = *static_cast<ui::OptimizeTab*>(tabs->widget(1));
    REQUIRE(stats.weapon_table->model->rows.size() == imported.weapons.size());
    widget<QComboBox>(stats, "starting_class_combobox")->setCurrentText("Wretch");
    widget<QSpinBox>(stats, "normal_upgrade_level_spinbox")->setValue(25);
    widget<QSpinBox>(stats, "somber_upgrade_level_spinbox")->setValue(10);
    widget<QCheckBox>(stats, "matchingPowerstance")->setChecked(true);
    auto* moves = widget<QComboBox>(stats, "moveSelection");
    auto* hits = widget<QSpinBox>(stats, "moveHitSelection");
    auto* results = widget<QTableWidget>(stats, "moveResults");
    std::map<std::pair<int, int>, std::shared_ptr<Moveset>> shared_sets;
    std::vector<std::reference_wrapper<const Weapon>> supported;

    for (std::size_t index = 0; index < imported.weapons.size(); ++index) {
        const auto& original = imported.weapons[index];
        const auto& loaded = stats.weapon_table->model->rows[index].attack.weapon.get();
        REQUIRE(loaded.full_name == original.full_name);
        REQUIRE(bool(loaded.moveset) == bool(original.moveset));
        CHECK(loaded.dlc == original.dlc);
        if (!original.moveset)
            continue;
        supported.push_back(std::cref(original));
        CAPTURE(original.full_name);
        auto& shared = shared_sets[{loaded.moveset->motionCategory, loaded.moveset->behaviorVariation}];
        if (shared)
            CHECK(shared == loaded.moveset);
        shared = loaded.moveset;

        stats.weapon_table->setCurrentIndex(stats.weapon_table->proxy_model->mapFromSource(
            stats.weapon_table->model->index(static_cast<int>(index), 0)));
        moves->setCurrentIndex(0); // All moves and all hits, including powerstance and criticals.
        hits->setValue(0);
        const MoveEvaluator evaluator(original, stats.get_character_stats(), stats.get_attack_options().upgrade_levels, &original,
            move_context(MoveEnvironment::ORDINARY_COMBAT));
        int row = 0;
        auto check_powers = [&](const MoveAttackPowers& expected) {
            REQUIRE(row < results->rowCount());
            REQUIRE(results->item(row, 2));
            CHECK_THAT(results->item(row, 2)->text().toDouble(),
                Catch::Matchers::WithinAbs(expected.total_attack_power[1], 1e-5));
            for (std::size_t type = 0; type < expected.attack_powers.size(); ++type) {
                REQUIRE(results->item(row, static_cast<int>(type) + 3));
                CHECK_THAT(results->item(row, static_cast<int>(type) + 3)->text().toDouble(),
                    Catch::Matchers::WithinAbs(expected.attack_powers[type][1], 1e-5));
            }
            ++row;
        };
        for (const auto& move : evaluator.evaluate_all()) {
            CAPTURE(std::to_underlying(move.id));
            REQUIRE(move.result.has_value());
            check_powers(move.result->total);
            for (const auto& hit : move.result->hits)
                check_powers(hit.powers);
        }
        CHECK(row == results->rowCount());

        select_move(moves, MoveId::ONE_HAND_R1_1);
        hits->setValue(1);
        REQUIRE(results->rowCount() == 1);
        row = 0;
        check_powers(evaluator.evaluate(MoveId::ONE_HAND_R1_1)->hits[0].powers);
    }
    REQUIRE(supported.size() == 1260);
    REQUIRE(shared_sets.size() == 87);
    CHECK(shared_sets.at({20, 100}) != shared_sets.at({23, 200}));

    widget<QComboBox>(optimize, "starting_class_combobox")->setCurrentText("Wretch");
    widget<QSpinBox>(optimize, "max_character_level_spinbox")->setValue(12);
    widget<QSpinBox>(optimize, "normal_upgrade_level_spinbox")->setValue(25);
    widget<QSpinBox>(optimize, "somber_upgrade_level_spinbox")->setValue(10);
    widget<QCheckBox>(optimize, "optimize_starting_class_checkbox")->setChecked(false);
    auto* selected_move = widget<QComboBox>(optimize, "moveSelection");
    auto* selected_hit = widget<QSpinBox>(optimize, "moveHitSelection");
    auto* pair = widget<QCheckBox>(optimize, "matchingPowerstance");
    auto* target = widget<QComboBox>(optimize, "target_combobox");

    // Verify both search paths and both total-power/status projections against
    // direct optimization using pre-serialization weapon data.
    for (const auto id : {MoveId::ONE_HAND_R1_1, MoveId::TWO_HAND_R1_1, MoveId::POWERSTANCE_L1_1}) {
        std::vector<std::reference_wrapper<const Weapon>> eligible;
        for (const auto& weapon : supported)
            if (find_move(weapon.get(), id)) eligible.push_back(weapon);
        REQUIRE(eligible.size() == (id == MoveId::POWERSTANCE_L1_1 ? 1217 : 1260));
        select_move(selected_move, id);
        pair->setChecked(id == MoveId::POWERSTANCE_L1_1);
        for (const int hit_number : {0, 1}) {
            selected_hit->setValue(hit_number);
            const optimizer::MoveObjective objective{
                {id, hit_number ? std::optional<std::size_t>{0} : std::nullopt}, pair->isChecked(),
                move_context(MoveEnvironment::ORDINARY_COMBAT)};
            for (const auto selected_target : {optimizer::Target::TOTAL_ATTACK_POWER, optimizer::Target::BLEED_STATUS_EFFECT}) {
                target->setCurrentIndex(std::to_underlying(selected_target));
                REQUIRE(widget<QLabel>(optimize, "weapons_label")->text() == QString::number(eligible.size()));
                for (const auto* button : {"start_v2_button", "start_brute_force_button"}) {
                    CAPTURE(std::to_underlying(id), hit_number, std::to_underlying(selected_target), button);
                    widget<QPushButton>(optimize, button)->click();
                    REQUIRE(optimize.weapon_table->model->rows.size() == eligible.size());
                    visit_enum(selected_target, [&](auto selected) {
                        if constexpr (selected.value == optimizer::Target::TOTAL_ATTACK_POWER
                            || selected.value == optimizer::Target::BLEED_STATUS_EFFECT) {
                            const optimizer::BruteForce<selected.value> direct(eligible, optimize.get_attack_options(),
                                character_level_to_attribute_points(12), optimize.get_character_stats(),
                                attribute_level_limit, false, objective);
                            for (const auto& row : optimize.weapon_table->model->rows) {
                                const auto& loaded = row.attack.weapon.get();
                                const auto original = std::ranges::find_if(eligible,
                                    [&](const auto& weapon) { return weapon.get().full_name == loaded.full_name; });
                                REQUIRE(original != eligible.end());
                                const auto best = direct(original->get());
                                const auto score = std::get<ui::sections::MoveScore>(row).data(0, Qt::UserRole).toDouble();
                                CHECK_THAT(score, Catch::Matchers::WithinAbs(direct.projection(best), 1e-8));
                                const MoveEvaluator evaluator(original->get(), row.attack.stats, row.attack.upgrade_levels,
                                    objective.matching_powerstance ? &original->get() : nullptr, objective.context);
                                const auto result = evaluator.evaluate(id);
                                REQUIRE(result.has_value());
                                const auto& powers = result->selected_powers(objective.selection.hit_index);
                                const double expected = selected.value == optimizer::Target::TOTAL_ATTACK_POWER
                                    ? powers.total_attack_power[1]
                                    : powers.attack_powers[std::to_underlying(AttackPowerType::BLEED)][1];
                                CHECK_THAT(score, Catch::Matchers::WithinAbs(expected, 1e-8));
                            }
                        }
                    });
                }
            }
        }
    }
}
#endif

TEST_CASE("UI moves - explicit offhand source needs no matching pair", "[moveset][ui]")
{
    initialize_gui();
    auto data = game_data();
    for (auto& weapon : data->weapons) {
        auto& move = weapon.moveset->moves[0];
        move.id = MoveId::OFFHAND_R1_1;
        move.name = "Offhand";
        for (auto& hit : move.hits) hit.event.source = 2;
    }
    const auto restored = std::make_shared<GameData>(json::read<GameData>(json::write(*data)));
    ui::StatsTab stats;
    stats.set_active_game_data(restored);
    stats.weapon_table->setCurrentIndex(stats.weapon_table->proxy_model->mapFromSource(
        stats.weapon_table->model->index(0, 0)));
    widget<QCheckBox>(stats, "matchingPowerstance")->setChecked(false);
    select_move(widget<QComboBox>(stats, "moveSelection"), MoveId::OFFHAND_R1_1);
    auto* results = widget<QTableWidget>(stats, "moveResults");
    REQUIRE(results->rowCount() == 3);
    CHECK(results->item(0, 2)->text().toDouble() == 300);
    widget<QSpinBox>(stats, "moveHitSelection")->setValue(2);
    REQUIRE(results->rowCount() == 1);
    CHECK(results->item(0, 2)->text().toDouble() == 150);
    ui::OptimizeTab optimize;
    optimize.set_active_game_data(restored);
    widget<QCheckBox>(optimize, "matchingPowerstance")->setChecked(false);
    select_move(widget<QComboBox>(optimize, "moveSelection"), MoveId::OFFHAND_R1_1);
    CHECK(widget<QLabel>(optimize, "weapons_label")->text() == "2");
    widget<QSpinBox>(optimize, "moveHitSelection")->setValue(2);
    CHECK(widget<QLabel>(optimize, "weapons_label")->text() == "1");
}
