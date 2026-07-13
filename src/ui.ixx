module;
#include <QMainWindow>
#include "ui_main_window.h"
export module erdo.ui;

import erdo;
import std;

export namespace ui
{

    class MainWindow : public QMainWindow
    {
        Q_OBJECT
        std::unique_ptr<Ui::MainWindow> ui = std::make_unique<Ui::MainWindow>();
        std::vector<calculator::Weapon> weapons{};

        std::vector<QSpinBox*> attribute_spinboxes{};
        QString most_recently_selected_affinity;


        // void set_weapon_stats(const calculator::Weapon& weapon, const calculator::AttackOptions& attack_options, const calculator::Stats& stats) {
        // }

        bool try_collect_and_set_weapon_stats() {

            auto current_item = this->ui->weapon_base_name_list->currentItem();
            if (!current_item)
                return false;
            auto weapon_base_name = current_item->text().toStdString();

            current_item = this->ui->weapon_affinity_list->currentItem();
            if (!current_item)
                return false;
            auto weapon_affinity = string_to_enum<calculator::Weapon::Affinity>(current_item->text().toStdString());

            // get weapon
            auto it = std::ranges::find_if(this->weapons, [&](const calculator::Weapon& w) {
                return w.base_name == weapon_base_name && w.affinity == weapon_affinity;
            });
            if (it == this->weapons.end())
                return false;
            auto&& weapon = *it;

            // get character stats
            calculator::Stats stats{};
            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes, stats))
                stat = spinbox->value();

            // get attack options
            calculator::AttackOptions attack_options{};
            attack_options.two_handing = this->ui->two_handing_checkbox->isChecked();
            attack_options.upgrade_levels.at(weapon.upgrade_level_index) = this->ui->upgrade_level_spinbox->value();

            // set weapon stats
            this->ui->weapon_full_name_label->setText(weapon.full_name.c_str());
            this->ui->weapon_type_label->setText(std::string(enum_to_string(weapon.type)).c_str());
            this->ui->base_game_dlc_label->setText(weapon.dlc ? "dlc" : "base game");

            // this->set_weapon_stats();
            return true;
        }

    public:
        explicit MainWindow(QWidget *parent = nullptr) {
            this->ui->setupUi(this);


            auto application_directory = std::filesystem::absolute(QCoreApplication::applicationDirPath().toStdString());
            auto xml_data_directory = application_directory / "xml_data";

            this->set_weapon_data(xml::get_weapons(xml_data_directory));
            

            connect(this->ui->weapon_base_name_list, &QListWidget::currentItemChanged,
                this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
                    if (current)
                        this->set_base_weapon(current->text().toStdString());
                }
            );

            connect(this->ui->weapon_affinity_list, &QListWidget::currentItemChanged,
                this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
                    if (current)
                    {
                        this->most_recently_selected_affinity = current->text();
                        this->try_collect_and_set_weapon_stats();
                    }
                }
            );

            for (auto& attribute : enumerators_of<calculator::Attribute>())
            {
                auto attribute_string = std::string(enum_to_string(attribute));

                QLabel *label = new QLabel(attribute_string.c_str());
                auto spinBox = attribute_spinboxes.emplace_back(new QSpinBox());

                this->ui->character_stats_layout->addRow(label, spinBox);
            }
        }

        void set_weapon_data(std::vector<calculator::Weapon>&& weapons) {
            this->weapons = std::move(weapons);

            auto weapon_base_names = this->weapons
                | std::views::transform([](const calculator::Weapon& w) -> QString { return w.base_name.c_str(); })
                | std::ranges::to<std::set>()
                | std::ranges::to<QList>();
            this->ui->weapon_base_name_list->addItems(weapon_base_names);
        }

        void set_base_weapon(const std::string& weapon_base_name) {
            // this->ui->weapon_base_name_list->blockSignals(true);

            this->ui->weapon_base_name_list->setCurrentItem(this->ui->weapon_base_name_list->findItems(weapon_base_name.c_str(), Qt::MatchExactly).first());

            // if (this->ui->weapon_affinity_list->currentItem())
            //     this->most_recently_selected_affinity = this->ui->weapon_affinity_list->currentItem()->text();

            auto affinities = this->weapons
                | std::views::filter([&](const calculator::Weapon& w) { return w.base_name == weapon_base_name; })
                | std::views::transform(&calculator::Weapon::affinity)
                | std::views::transform([](const calculator::Weapon::Affinity& a) -> QString { return std::string(enum_to_string(a)).c_str(); })
                | std::ranges::to<QList>();
            this->ui->weapon_affinity_list->clear();
            this->ui->weapon_affinity_list->addItems(affinities);

            // this->ui->weapon_base_name_list->blockSignals(false);

            auto matches = this->ui->weapon_affinity_list->findItems(this->most_recently_selected_affinity, Qt::MatchExactly);
            if (!matches.isEmpty())
                this->ui->weapon_affinity_list->setCurrentItem(matches.first());
        }

        void set_affinity(calculator::Weapon::Affinity affinity) {
            auto affinity_string = std::string(enum_to_string(affinity));

            auto matches = this->ui->weapon_affinity_list->findItems(affinity_string.c_str(), Qt::MatchExactly);
            if (!matches.isEmpty())
                this->ui->weapon_affinity_list->setCurrentItem(matches.first());
        }

        void set_weapon(const calculator::Weapon& weapon) {
            this->set_base_weapon(weapon.base_name);
            this->set_affinity(weapon.affinity);
        }
    };
}

#include "ui.moc"