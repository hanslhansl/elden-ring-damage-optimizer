module;
#include <QMainWindow>
#include <QPainter>
#include <QActionGroup>
#include <QProgressDialog>
#include <QFuture>
#include <QtConcurrent>
#include "ui_main_window.h"
export module erdo.ui;
export import erdo.ui.weapons_table;

import erdo;
import std;

template<typename T>
bool contains_object(const std::vector<T>& vec, const T& obj) {
    return std::any_of(vec.begin(), vec.end(),
        [&](const T& element) {
            return &element == &obj;
        });
}


namespace erdo::ui
{
    template<typename F>
    auto blocking_progress_bar_dialog(QWidget* parent, const QString& label_text, F&& computation)
    {
        using R = decltype(computation());

        QProgressDialog dialog(nullptr/*label_text*/, nullptr, 0, 0, parent);
        // auto title = parent->windowTitle();
        dialog.setWindowTitle(label_text);
        // dialog.setMinimumWidth(QFontMetrics(dialog.font()).horizontalAdvance(title) + 150);
        dialog.setWindowModality(Qt::ApplicationModal);
        dialog.setMinimumDuration(0);
        dialog.setCancelButton(nullptr);
        dialog.show();

        QFuture<R> future = QtConcurrent::run(std::forward<F>(computation));

        QFutureWatcher<R> watcher;
        QEventLoop loop;

        QObject::connect(
            &watcher,
            &QFutureWatcher<R>::finished,
            &loop,
            &QEventLoop::quit
        );

        watcher.setFuture(future);

        // Blocks this function, but keeps Qt responsive
        loop.exec();

        dialog.close();

        if constexpr (!std::is_void_v<R>)
        {
            if (!future.isValid())
                throw std::runtime_error("future is not valid after computation");
            return future.takeResult();
        }
    }

    class MainWindow : public QMainWindow
    {
        std::unique_ptr<Ui::MainWindow> ui = std::make_unique<Ui::MainWindow>();
        std::vector<QSpinBox*> attribute_spinboxes{};
        std::vector<std::array<QLabel*, 3>> attack_power_labels{};
        std::vector<std::array<QLabel*, 3>> status_effect_labels{};
        std::vector<QLabel*> attribute_scaling_labels{};
        std::vector<QLabel*> attribute_requirements_labels{};
        WeaponTable* weapon_table{};

        std::vector<calculator::Weapon> active_weapon_data{};


        long long _calculate_weapon_stats_counter = 0;
        class calculate_weapon_stats_counter {
            MainWindow* self;
            std::filesystem::path new_path;
        public:
            explicit calculate_weapon_stats_counter(MainWindow* self, const std::filesystem::path& new_path = {}) : self{self}, new_path{new_path} {
                self->_calculate_weapon_stats_counter++;
            }

            ~calculate_weapon_stats_counter() {
                if (--self->_calculate_weapon_stats_counter == 0)
                    self->calculate_weapon_stats(new_path);
            }
        };

        void calculate_weapon_stats(const std::filesystem::path& new_weapon_data_directory = {})
        {
            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            calculator::AttackOptions attack_options{
                this->get_upgrade_levels(),
                this->get_two_handing()
            };


            if (new_weapon_data_directory.empty())
            {
                auto start = std::chrono::high_resolution_clock::now();

                std::ranges::for_each(std::views::zip(this->get_active_weapon_data(), this->weapon_table->model->rows), [&](auto&& pair) {
                    auto&& [w, row] = pair;
                    row.update(w.calculate_attack_rating(attack_options, stats));
                });
                // this->weapon_table->model->rows.clear();
                // this->weapon_table->model->rows.append_range(
                //     this->get_active_weapon_data()
                //         | std::views::transform([&](const calculator::Weapon& w) { return Row(w.calculate_attack_rating(attack_options, stats)); })
                // );

                this->weapon_table->model->notifyAllChanged();
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                std::println("calculate weapon data: {} seconds", elapsed.count());
            }
            else
            {
                auto start = std::chrono::high_resolution_clock::now();

                auto&& [new_active_weapon_data, new_rows] = blocking_progress_bar_dialog(
                    this,
                    "loading weapon data",
                    [&](){
                        std::pair<std::vector<calculator::Weapon>, std::vector<Row>> result {
                            parser::load_weapons(new_weapon_data_directory),
                            {}
                        };
                        
                        if (result.first.empty())
                            throw std::runtime_error("weapon_data is empty");

                        result.second.reserve(result.first.size());
                        result.second.append_range(result.first
                            | std::views::transform([&](const calculator::Weapon& w) { return Row(w.calculate_attack_rating(attack_options, stats)); })
                            | std::ranges::to<std::vector>()
                        );


                        return result;
                    }
                );

                this->active_weapon_data = std::move(new_active_weapon_data);
                this->weapon_table->model->set_rows(std::move(new_rows));

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                std::println("load weapon data: {} seconds", elapsed.count());
            }
            this->weapon_table->resize_columns_to_contents();
        }

    public:
        explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
        {
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

            // upgrade level spinboxes
            connect(this->ui->normal_upgrade_level_spinbox, &QSpinBox::valueChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });
            connect(this->ui->somber_upgrade_level_spinbox, &QSpinBox::valueChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // two-handing checkbox
            connect(this->ui->two_handing_checkbox, &QCheckBox::checkStateChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // weapon base name list widget
            /*connect(this->ui->weapon_base_name_list, &QListWidget::currentItemChanged,
                this, [this](QListWidgetItem *current_item, QListWidgetItem*) {
                    auto raii = calculate_weapon_stats_counter(this);

                    if (!current_item)
                        return;
                    auto base_weapon = current_item->text().toStdString();

                    auto&& active_weapon_data = this->get_active_weapon_data();

                    auto it = std::ranges::find(active_weapon_data, base_weapon, &calculator::Weapon::base_name);
                    if (it == active_weapon_data.end())
                        throw std::runtime_error("base weapon not found in weapons list");
                    auto&& weapon = *it;

                    this->ui->upgrade_level_spinbox->setMaximum(weapon.base_attack_power.size() - 1);

                    QString previous_affinity_string{};
                    current_item = this->ui->weapon_affinity_list->currentItem();
                    if (current_item)
                        previous_affinity_string = current_item->text();

                    auto affinities = active_weapon_data
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
            );*/

            // affinity list widget
            /*connect(this->ui->weapon_affinity_list, &QListWidget::currentItemChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });*/

            // attack power labels
            /*for (auto&& [row, damage_type] : enumerators_of<calculator::DamageType>() | std::views::enumerate)
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
            }*/

            // weapon table view
            this->weapon_table = new WeaponTable(this);
            this->ui->weapon_stats_layout->addWidget(this->weapon_table, 1);

            // load weapon data
            auto application_directory = std::filesystem::absolute(QCoreApplication::applicationDirPath().toStdString()).make_preferred();
            auto xml_data_directory = application_directory / "xml_data";
            auto weapon_data_directories = std::filesystem::directory_iterator(xml_data_directory)
                | std::views::transform(&std::filesystem::directory_entry::path)
                | std::ranges::to<std::set<
                    std::filesystem::path,
                    decltype([](const std::filesystem::path& a, const std::filesystem::path& b) {
                        return std::stoll(a.filename().string()) > std::stoll(b.filename().string());
                    })
                >>();
            if (weapon_data_directories.empty())
                throw std::runtime_error("no weapon data directories found in xml_data directory");

            // weapon data menu
            QActionGroup *group = new QActionGroup(this);
            group->setExclusive(true);
            for (auto&& [i, dir] : weapon_data_directories | std::views::enumerate)
            {
                QAction *action = this->ui->menu_weapon_data->addAction(QString::fromStdString(dir.string()));
                action->setCheckable(true);
                group->addAction(action);
                connect(action, &QAction::triggered, this, [this, dir]() { this->set_active_weapon_data(dir); });
                if (i == 0)
                    QTimer::singleShot(0, action, &QAction::trigger);
            }

            this->ui->menu_weapon_data->addSeparator();

            QAction* action = this->ui->menu_weapon_data->addAction("load weapon data from directory");
            connect(action, &QAction::triggered, this, [this]() { std::println("not implemented"); });

            action = this->ui->menu_weapon_data->addAction("generate weapon data from game data");
            connect(action, &QAction::triggered, this, [this]() { std::println("not implemented"); });
        }

        const std::vector<calculator::Weapon>& get_active_weapon_data() const
        {
            return this->active_weapon_data;
        }
        void set_active_weapon_data(const std::filesystem::path& dir)
        {
            auto raii = calculate_weapon_stats_counter(this, dir);
        }

        calculator::Stats get_character_stats()
        {
            calculator::Stats stats{};
            for (auto&& [spinbox, stat] : std::views::zip(attribute_spinboxes | std::views::drop(calculator::irrelevant_attribute_count), stats))
                stat = spinbox->value();
            return stats;
        }
        void set_character_stats(const calculator::Stats& stats)
        {
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

        calculator::UpgradeLevels get_upgrade_levels() {
            calculator::UpgradeLevels upgrade_levels{};
            upgrade_levels.at(1) = this->ui->normal_upgrade_level_spinbox->value();
            upgrade_levels.at(2) = this->ui->somber_upgrade_level_spinbox->value();
            return upgrade_levels;
        }
        void set_upgrade_levels(const calculator::UpgradeLevels& upgrade_levels) {
            auto raii = calculate_weapon_stats_counter(this);

            this->ui->normal_upgrade_level_spinbox->setValue(upgrade_levels.at(1));
            this->ui->somber_upgrade_level_spinbox->setValue(upgrade_levels.at(2));
        }

        bool get_two_handing() {
            return this->ui->two_handing_checkbox->isChecked();
        }
        void set_two_handing(bool two_handing) {
            auto raii = calculate_weapon_stats_counter(this);

            this->ui->two_handing_checkbox->setChecked(two_handing);
        }

        /*std::string get_base_weapon() {
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
        }*/

        /*calculator::Weapon::Affinity get_affinity() {
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
        }*/

        /*const calculator::Weapon& get_weapon() {
            auto weapon_base_name = this->get_base_weapon();
            auto weapon_affinity = this->get_affinity();

            // get weapon
            auto&& active_weapon_data = this->get_active_weapon_data();
            auto it = std::ranges::find_if(active_weapon_data, [&](const calculator::Weapon& w) {
                return w.base_name == weapon_base_name && w.affinity == weapon_affinity;
            });
            if (it == active_weapon_data.end())
                throw std::runtime_error("weapon not found");

            return *it;
        }
        void set_weapon(const calculator::Weapon& weapon) {
            auto raii = calculate_weapon_stats_counter(this);

            this->set_base_weapon(weapon.base_name);
            this->set_affinity(weapon.affinity);
        }*/
    };

    export int run_ui(int argc, char *argv[]) {
        QApplication app(argc, argv);

        MainWindow window{};
        window.show();

        return app.exec();
    }
}

// #include "ui.moc"