module;
#include <QMainWindow>
#include <QPainter>
#include <QActionGroup>
#include <QProgressDialog>
#include <QFuture>
#include <QtConcurrent>
#include <algorithm>
#include <ranges>
#include "ui_main_window.h"
#include "ui_stats_tab.h"
#include "ui_optimize_tab.h"
#include "ui_plot_tab.h"
export module erdo.ui;
export import erdo.ui.weapons_table;

import erdo;
import std;


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

    struct StatsTab : public QWidget, Ui::StatsTab
    {
        explicit StatsTab(QWidget *parent = nullptr) : QWidget(parent)
        {
            this->setupUi(this);
        };
    };

    struct OptimizeTab : public QWidget, Ui::OptimizeTab
    {
        explicit OptimizeTab(QWidget *parent = nullptr) : QWidget(parent)
        {
            this->setupUi(this);
        };
    };

    struct PlotTab : QWidget, Ui::PlotTab
    {
        explicit PlotTab(QWidget *parent = nullptr) : QWidget(parent)
        {
            this->setupUi(this);
        };
    };

    class MainWindow : public QMainWindow
    {
        std::unique_ptr<Ui::MainWindow> ui = std::make_unique<Ui::MainWindow>();
        StatsTab* stats = new StatsTab();
        OptimizeTab* optimize = new OptimizeTab();
        PlotTab* plot = new PlotTab();

        std::vector<QSpinBox*> attribute_spinboxes{};
        QLabel* character_level_label{};
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
            auto full_stats = this->get_character_full_stats();
            auto stats = full_stats.to_stats();

            // get attack options
            calculator::AttackOptions attack_options{
                this->get_upgrade_levels(),
                this->get_two_handing()
            };

            this->character_level_label->setText(QString::number(full_stats.character_level()));

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

                this->stats->base_name_list->clear();
                this->stats->base_name_list->addItems(new_active_weapon_data
                    | std::views::transform(&calculator::Weapon::base_name)
                    | std::ranges::to<std::set>()
                    | std::views::transform(static_cast<QString(*)(const std::string&)>(string_to_display))
                    | std::ranges::to<QList>());
                
                this->active_weapon_data = std::move(new_active_weapon_data);
                this->weapon_table->model->set_rows(std::move(new_rows));

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                std::println("load weapon data: {} seconds", elapsed.count());
            }
            QTimer::singleShot(50, this->weapon_table, &WeaponTable::resize_columns_to_contents);
        }

    public:
        explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
        {
            this->ui->setupUi(this);
            this->ui->tab_widget->addTab(stats, string_to_display("stats"));
            this->ui->tab_widget->addTab(optimize, string_to_display("optimize"));
            this->ui->tab_widget->addTab(plot, string_to_display("plot"));

            this->setWindowTitle(string_to_display(windowTitle()));
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
                this->stats->starting_class_combobox->addItem(QString::fromStdString(class_name));
            this->stats->starting_class_combobox->setCurrentIndex(-1);
            connect(this->stats->starting_class_combobox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
                auto raii = calculate_weapon_stats_counter(this);

                if (text.isEmpty())
                    return;

                auto full_stats = calculator::character_class_stats.at(text.toStdString());
                this->stats->starting_class_combobox->blockSignals(true);
                this->set_character_full_stats(full_stats);
                this->stats->starting_class_combobox->blockSignals(false);
            });
            
            // character stats spinboxes
            for (auto& attribute : enumerators_of<calculator::Attribute>())
            {
                auto attribute_spinbox = this->attribute_spinboxes.emplace_back(new QSpinBox());
                attribute_spinbox->setMinimum(1);
                attribute_spinbox->setMaximum(99);
                this->stats->character_stats_layout->addRow(
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
                        this->stats->starting_class_combobox->setCurrentText(QString::fromStdString(it->first));
                    else
                        this->stats->starting_class_combobox->setCurrentIndex(-1);

                    std::ranges::for_each(attribute_spinboxes, [](QSpinBox* spinbox) { spinbox->blockSignals(false); });
                });
            }
            this->stats->character_stats_layout->addRow(
                string_to_display("character level:"),
                this->character_level_label = new QLabel()
            );

            // upgrade level spinboxes
            connect(this->stats->normal_upgrade_level_spinbox, &QSpinBox::valueChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });
            connect(this->stats->somber_upgrade_level_spinbox, &QSpinBox::valueChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // two-handing checkbox
            connect(this->stats->two_handing_checkbox, &QCheckBox::checkStateChanged, this, [this]() {
                auto raii = calculate_weapon_stats_counter(this);
            });

            // base game / dlc
            for (auto&& [val, str] : std::views::zip(std::array{false, true}, std::array{"base game", "dlc"}))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->stats->base_game_dlc_list);
                item->setData(Qt::UserRole, val);
            }
            connect(
                this->stats->base_game_dlc_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<bool> selected;

                    for (QListWidgetItem *item : this->stats->base_game_dlc_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toBool());

                    this->weapon_table->proxy_model->set_selected_base_game_dlc(std::move(selected));
                }
            );

            // weapon type list widget
            for (auto&& [type, str] : std::views::zip(enumerator_integrals_of<calculator::Weapon::Type>(), enumerator_strings_of<calculator::Weapon::Type>()))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->stats->type_list);
                item->setData(Qt::UserRole, type);
            }
            connect(
                this->stats->type_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<int> selected;

                    for (QListWidgetItem *item : this->stats->type_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toInt());

                    this->weapon_table->proxy_model->set_selected_types(std::move(selected));
                }
            );

            // weapon base name list widget
            connect(
                this->stats->base_name_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<QString> selected;

                    for (QListWidgetItem *item : this->stats->base_name_list->selectedItems())
                        selected.insert(item->text());

                    this->weapon_table->proxy_model->set_selected_base_names(std::move(selected));
                }
            );

            // weapon affinity list widget
            for (auto&& [affinity, str] : std::views::zip(enumerator_integrals_of<calculator::Weapon::Affinity>(), enumerator_strings_of<calculator::Weapon::Affinity>()))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->stats->affinity_list);
                item->setData(Qt::UserRole, affinity);
            }
            connect(
                this->stats->affinity_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<int> selected;

                    for (QListWidgetItem *item : this->stats->affinity_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toInt());

                    this->weapon_table->proxy_model->set_selected_affinities(std::move(selected));
                }
            );

            // weapon table view
            this->weapon_table = new WeaponTable(this);
            this->weapon_table->hide_section<sections::Stats>();
            this->stats->main_layout->addWidget(this->weapon_table, 1);

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
                    QTimer::singleShot(50, action, &QAction::trigger);
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
            upgrade_levels.at(1) = this->stats->normal_upgrade_level_spinbox->value();
            upgrade_levels.at(2) = this->stats->somber_upgrade_level_spinbox->value();
            return upgrade_levels;
        }
        void set_upgrade_levels(const calculator::UpgradeLevels& upgrade_levels) {
            auto raii = calculate_weapon_stats_counter(this);

            this->stats->normal_upgrade_level_spinbox->setValue(upgrade_levels.at(1));
            this->stats->somber_upgrade_level_spinbox->setValue(upgrade_levels.at(2));
        }

        bool get_two_handing() {
            return this->stats->two_handing_checkbox->isChecked();
        }
        void set_two_handing(bool two_handing) {
            auto raii = calculate_weapon_stats_counter(this);

            this->stats->two_handing_checkbox->setChecked(two_handing);
        }
    };

    export int run_ui(int argc, char *argv[]) {
        QApplication app(argc, argv);

        MainWindow window{};
        window.showMaximized();

        return app.exec();
    }
}

// #include "ui.moc"