module;
#include <QMainWindow>
#include <QPainter>
// #include <QMatrix>
#include "ui_main_window.h"
export module erdo.ui;

import erdo;
import std;

QString string_to_display(const QString& str) {
    QStringList words = str.split(QRegularExpression("[_\\s]+"), Qt::SkipEmptyParts);

    for (QString &word : words) {
        word = word.toLower();
        // if (!word.isEmpty())
        //     word[0] = word[0].toUpper();
    }

    return words.join(' ');
}
QString string_to_display(const char* str) {
    return string_to_display(QString::fromStdString(str));
}
QString string_to_display(std::string_view str) {
    return string_to_display(QString::fromStdString(std::string(str)));
}

std::string format_float(double x) {
    std::string s = std::format("{:.3f}", x);

    // Remove trailing zeros
    while (!s.empty() && s.back() == '0')
        s.pop_back();

    // Remove trailing decimal point
    if (!s.empty() && s.back() == '.')
        s.pop_back();

    return s;
}


export namespace ui
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT
        std::unique_ptr<Ui::MainWindow> ui = std::make_unique<Ui::MainWindow>();
        std::vector<calculator::Weapon> weapons{};

        std::vector<QSpinBox*> attribute_spinboxes{};
        std::vector<std::array<QLabel*, 3>> attack_power_labels{};
        std::vector<std::array<QLabel*, 3>> status_effect_labels{};
        std::vector<QLabel*> attribute_scaling_labels{};
        std::vector<QLabel*> attribute_requirements_labels{};


        long long _calculate_weapon_stats_counter = 0;
        class calculate_weapon_stats_counter {
            MainWindow* self;
        public:
            explicit calculate_weapon_stats_counter(MainWindow* self) : self{self} {
                self->_calculate_weapon_stats_counter++;
            }

            ~calculate_weapon_stats_counter() {
                if (--self->_calculate_weapon_stats_counter == 0)
                    self->calculate_weapon_stats();
            }
        };

        void calculate_weapon_stats() {
            // get weapon
            auto&& weapon = this->get_weapon();

            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            calculator::AttackOptions attack_options{};
            attack_options.two_handing = this->ui->two_handing_checkbox->isChecked();
            auto upgrade_level = attack_options.upgrade_levels.at(weapon.upgrade_level_index) = this->ui->upgrade_level_spinbox->value();

            // set weapon stats
            auto attack_rating = weapon.get_attack_rating(attack_options, stats);

            auto full_name = QString::fromStdString(weapon.full_name);
            if (upgrade_level != 0)
                full_name += " +" + QString::number(upgrade_level);
            this->ui->weapon_full_name_label->setText(full_name);
            this->ui->weapon_type_label->setText(string_to_display(enum_to_string(weapon.type)));
            this->ui->base_game_dlc_label->setText(string_to_display(weapon.dlc ? "dlc" : "base game"));
            this->ui->spell_scaling_label->setText(QString::fromStdString(format_float(attack_rating.spell_scaling)));

            auto formatters = std::array<QString(*)(double, bool), 3>{
                [](double value, bool is_ineffective){
                    return QString::fromStdString(format_float(value));
                },
                [](double value, bool is_ineffective){
                    auto text = format_float(value);
                    if (value >= 0)
                        text.insert(0, "+");
                    if (is_ineffective)
                        text = std::format("<font color='red'>{}</font>", text);
                    return QString::fromStdString(text);
                },
                [](double value, bool is_ineffective){
                    return "= " + QString::fromStdString(format_float(value));
                }
            };

            for (auto&& [formatter, label, value] : std::views::zip(
                formatters,
                std::array { this->ui->total_attack_power_label_0, this->ui->total_attack_power_label_1, this->ui->total_attack_power_label_2 },
                attack_rating.total_attack_power
            ))
                label->setText(formatter(value, false));

            for (auto&& [labels, values, is_ineffective] : std::views::zip(
                std::views::join(std::views::all(std::array<std::span<std::array<QLabel*, 3>>, 3>{ this->attack_power_labels, this->status_effect_labels })),
                std::views::join(std::views::all(std::array<std::span<std::array<double, 3>>, 3>{ attack_rating.attack_power, attack_rating.status_effect })),
                attack_rating.ineffective_attack_power_types
            ))
                for (auto&& [formatter, label, value] : std::views::zip(formatters, labels, values))
                    label->setText(formatter(value, is_ineffective));

            // ineffective_attributes
            for (auto&& [scaling_label, requirement_label, requirement, is_ineffective] : std::views::zip(
                this->attribute_scaling_labels,
                this->attribute_requirements_labels,
                weapon.requirements,
                attack_rating.ineffective_attributes
            ))
            {
                std::string text = "\u2012";
                if (requirement != 0)
                {
                    if (is_ineffective)
                        text = std::format("<font color='red'>\u2265{}</font>", requirement);
                    else
                        text = std::format("\u2265{}", requirement);
                }
                requirement_label->setText(QString::fromStdString(text));
            }

            // static_assert(false, "add attribute scaling letter and number as well as list of ineffective attributes.");
        }

    public:
        explicit MainWindow(QWidget *parent = nullptr) {
            this->ui->setupUi(this);

            setWindowTitle(string_to_display(windowTitle()));
            for (QWidget *w : findChildren<QWidget *>())
            {
                if (auto tab = qobject_cast<QTabWidget *>(w)) {
                    for (int i = 0; i < tab->count(); ++i) {
                        tab->setTabText(i, string_to_display(tab->tabText(i)));
                    }
                }
                else if (auto label = qobject_cast<QLabel *>(w)) {
                    label->setText(string_to_display(label->text()));
                }
                else if (auto button = qobject_cast<QAbstractButton *>(w)) {
                    button->setText(string_to_display(button->text()));
                }
                else if (auto box = qobject_cast<QGroupBox *>(w)) {
                    box->setTitle(string_to_display(box->title()));
                }
                else if (auto menu = qobject_cast<QMenu *>(w)) {
                    menu->setTitle(string_to_display(menu->title()));
                }
            }

            // starting class combobox
            for (const auto& [class_name, _] : calculator::character_class_stats)
                this->ui->starting_class_combobox->addItem(QString::fromStdString(class_name));
            this->ui->starting_class_combobox->setCurrentIndex(-1);
            connect(this->ui->starting_class_combobox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
                auto raii = calculate_weapon_stats_counter(this);

                if (text.isEmpty())
                    return;

                auto full_stats = calculator::character_class_stats.at(text.toStdString());
                this->ui->starting_class_combobox->blockSignals(true);
                this->set_character_full_stats(full_stats);
                this->ui->starting_class_combobox->blockSignals(false);
            });
            
            // character stats spinboxes
            for (auto& attribute : enumerators_of<calculator::Attribute>())
            {
                auto attribute_spinbox = this->attribute_spinboxes.emplace_back(new QSpinBox());
                attribute_spinbox->setMinimum(1);
                attribute_spinbox->setMaximum(99);
                this->ui->character_stats_layout->addRow(
                    string_to_display(enum_to_string(attribute)),
                    attribute_spinbox
                );

                connect(attribute_spinbox, &QSpinBox::valueChanged, this, [this]() {
                    auto raii = calculate_weapon_stats_counter(this);

                    auto full_stats = this->get_character_full_stats();
                    auto it = std::ranges::find_if(calculator::character_class_stats, [&](const auto& pair) {
                        return pair.second == full_stats;
                    });

                    std::ranges::for_each(attribute_spinboxes, [](QSpinBox* spinbox) { spinbox->blockSignals(true); });

                    if (it != calculator::character_class_stats.end()) 
                        this->ui->starting_class_combobox->setCurrentText(QString::fromStdString(it->first));
                    else
                        this->ui->starting_class_combobox->setCurrentIndex(-1);

                    std::ranges::for_each(attribute_spinboxes, [](QSpinBox* spinbox) { spinbox->blockSignals(false); });
                });
            }

            // upgrade level spinbox
            connect(this->ui->upgrade_level_spinbox, &QSpinBox::valueChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // two-handing checkbox
            connect(this->ui->two_handing_checkbox, &QCheckBox::checkStateChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // weapon base name list widget
            connect(this->ui->weapon_base_name_list, &QListWidget::currentItemChanged,
                this, [this](QListWidgetItem *current_item, QListWidgetItem*) {
                    auto raii = calculate_weapon_stats_counter(this);

                    if (!current_item)
                        return;
                    auto base_weapon = current_item->text().toStdString();

                    auto it = std::ranges::find(this->weapons, base_weapon, &calculator::Weapon::base_name);
                    if (it == this->weapons.end())
                        throw std::runtime_error("base weapon not found in weapons list");
                    auto&& weapon = *it;

                    this->ui->upgrade_level_spinbox->setMaximum(weapon.base_attack_power.size() - 1);

                    QString previous_affinity_string{};
                    current_item = this->ui->weapon_affinity_list->currentItem();
                    if (current_item)
                        previous_affinity_string = current_item->text();

                    auto affinities = this->weapons
                        | std::views::filter([&](const calculator::Weapon& w) { return w.base_name == weapon.base_name; })
                        | std::views::transform(&calculator::Weapon::affinity)
                        | std::ranges::to<std::vector>();
                    if (affinities.empty())
                        throw std::runtime_error("no affinities found for base weapon");
                    constexpr auto all_affinities = enumerators_of<calculator::Weapon::Affinity>();
                    std::ranges::sort(affinities, [&](auto a, auto b) {
                        return std::ranges::find(all_affinities, a) < std::ranges::find(all_affinities, b);
                    });
                    this->ui->weapon_affinity_list->clear();
                    this->ui->weapon_affinity_list->addItems(
                        affinities
                        | std::views::transform([](const calculator::Weapon::Affinity& a) { return QString::fromStdString(std::string(enum_to_string(a))); })
                        | std::ranges::to<QList>()
                    );

                    auto matches = this->ui->weapon_affinity_list->findItems(previous_affinity_string, Qt::MatchExactly);
                    if (!matches.isEmpty())
                        this->ui->weapon_affinity_list->setCurrentItem(matches.first());
                    else
                        this->ui->weapon_affinity_list->setCurrentRow(0);
                }
            );

            // affinity list widget
            connect(this->ui->weapon_affinity_list, &QListWidget::currentItemChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // attack power labels
            for (auto&& [row, damage_type] : enumerators_of<calculator::DamageType>() | std::views::enumerate)
            {
                ++row;

                this->ui->attack_power_layout->addWidget(new QLabel(string_to_display(enum_to_string(damage_type))), row, 0);

                for (auto&& [col, attack_power_label] : this->attack_power_labels.emplace_back() | std::views::enumerate)
                    this->ui->attack_power_layout->addWidget(attack_power_label = new QLabel(), row, col + 1);
            }

            // status effect labels
            for (auto&& [row, status_type] : enumerators_of<calculator::StatusType>() | std::views::enumerate)
            {
                this->ui->status_effect_layout->addWidget(new QLabel(string_to_display(enum_to_string(status_type))), row, 0);

                for (auto&& [col, status_effect_label] : this->status_effect_labels.emplace_back() | std::views::enumerate)
                    this->ui->status_effect_layout->addWidget(status_effect_label = new QLabel(), row, col + 1);
            }

            // attribute labels
            for (auto&& [row, attribute] : enumerators_of<calculator::RelevantAttribute>() | std::views::enumerate)
            {
                row += 2;

                this->ui->attribute_layout->addWidget(new QLabel(string_to_display(enum_to_string(attribute))), row, 0);

                this->ui->attribute_layout->addWidget(this->attribute_scaling_labels.emplace_back(new QLabel()), row, 1);
                this->ui->attribute_layout->addWidget(this->attribute_requirements_labels.emplace_back(new QLabel()), row, 2);
            }

            // load weapon data
            auto application_directory = std::filesystem::absolute(QCoreApplication::applicationDirPath().toStdString());
            auto xml_data_directory = application_directory / "xml_data";
            this->set_weapon_data(xml::load_weapons(xml_data_directory));
        }

        void set_weapon_data(std::vector<calculator::Weapon>&& weapons) {
            if (weapons.empty())
                throw std::runtime_error("weapons list is empty");

            this->weapons = std::move(weapons);

            auto weapon_base_names = this->weapons
                | std::views::transform([](const calculator::Weapon& w) { return QString::fromStdString(w.base_name); })
                | std::ranges::to<std::set>()
                | std::ranges::to<QList>();
            this->ui->weapon_base_name_list->clear();
            this->ui->weapon_base_name_list->addItems(weapon_base_names);
            this->ui->weapon_base_name_list->setCurrentRow(0);
        }

        calculator::Stats get_character_stats() {
            calculator::Stats stats{};
            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes | std::views::drop(calculator::irrelevant_attribute_count), stats))
                stat = spinbox->value();
            return stats;
        }
        void set_character_stats(const calculator::Stats& stats) {
            auto raii = calculate_weapon_stats_counter(this);

            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes | std::views::drop(calculator::irrelevant_attribute_count), stats))
                spinbox->setValue(stat);
        }

        calculator::FullStats get_character_full_stats() {
            calculator::FullStats full_stats{};
            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes, full_stats))
                stat = spinbox->value();
            return full_stats;
        }
        void set_character_full_stats(const calculator::FullStats& full_stats) {
            auto raii = calculate_weapon_stats_counter(this);

            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes, full_stats))
                spinbox->setValue(stat);
        }

        std::string get_base_weapon() {
            auto current_item = this->ui->weapon_base_name_list->currentItem();
            if (!current_item)
                throw std::runtime_error("no weapon base name selected");
            return current_item->text().toStdString();
        }
        void set_base_weapon(const std::string& base_weapon) {
            auto raii = calculate_weapon_stats_counter(this);

            auto matches = this->ui->weapon_base_name_list->findItems(QString::fromStdString(base_weapon), Qt::MatchExactly);
            if (matches.isEmpty())
                throw std::runtime_error("weapon base name not found in list widget");
            this->ui->weapon_base_name_list->setCurrentItem(matches.first());
        }

        calculator::Weapon::Affinity get_affinity() {
            auto current_item = this->ui->weapon_affinity_list->currentItem();
            if (!current_item)
                throw std::runtime_error("no weapon affinity selected");
            return string_to_enum<calculator::Weapon::Affinity>(current_item->text().toStdString());
        }
        void set_affinity(calculator::Weapon::Affinity affinity) {
            auto raii = calculate_weapon_stats_counter(this);

            auto affinity_string = QString::fromStdString(std::string(enum_to_string(affinity)));
            auto matches = this->ui->weapon_affinity_list->findItems(affinity_string, Qt::MatchExactly);
            if (matches.isEmpty())
                throw std::runtime_error("weapon affinity not found in list widget");
            this->ui->weapon_affinity_list->setCurrentItem(matches.first());
        }

        calculator::Weapon& get_weapon() {
            auto weapon_base_name = this->get_base_weapon();
            auto weapon_affinity = this->get_affinity();

            // get weapon
            auto it = std::ranges::find_if(this->weapons, [&](const calculator::Weapon& w) {
                return w.base_name == weapon_base_name && w.affinity == weapon_affinity;
            });
            if (it == this->weapons.end())
                throw std::runtime_error("weapon not found");

            return *it;
        }
        void set_weapon(const calculator::Weapon& weapon) {
            auto raii = calculate_weapon_stats_counter(this);

            this->set_base_weapon(weapon.base_name);
            this->set_affinity(weapon.affinity);
        }
    };
}

#include "ui.moc"