module;
#include <QMainWindow>
#include <QPainter>
#include <QActionGroup>
#include <QProgressDialog>
#include <QFuture>
#include <QtConcurrent>
#include <functional>
#include "ui_main_window.h"
#include "ui_stats_tab.h"
#include "ui_optimize_widget.h"
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

    class StatsTabBase : public QWidget, public Ui::StatsTab
    {
        Q_OBJECT

    protected:
        std::span<calculator::Weapon> active_weapon_data{};
        std::vector<QSpinBox*> attribute_spinboxes{};
        WeaponTable* weapon_table{};

    public:
        explicit StatsTabBase(QWidget *parent = nullptr) : QWidget(parent)
        {
            this->setupUi(this);

            // starting class combobox
            for (const auto& [class_name, _] : calculator::character_class_stats)
                this->starting_class_combobox->addItem(QString::fromStdString(class_name));
            this->starting_class_combobox->setCurrentIndex(-1);
            connect(this->starting_class_combobox, &QComboBox::currentTextChanged, [this](const QString& text) {
                if (text.isEmpty())
                    return;

                auto&& full_stats = calculator::character_class_stats.at(text.toStdString());
                for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes, full_stats))
                {
                    QSignalBlocker b { spinbox };
                    spinbox->setValue(stat);
                }

                emit character_stats_changed(full_stats);
            });

            // character stats spinboxes
            for (auto& attribute : enumerators_of<calculator::Attribute>())
            {
                auto attribute_spinbox = this->attribute_spinboxes.emplace_back(new QSpinBox());
                attribute_spinbox->setMinimum(1);
                attribute_spinbox->setMaximum(99);
                this->character_stats_layout->addRow(
                    string_to_display(enum_to_string(attribute)),
                    attribute_spinbox
                );

                connect(attribute_spinbox, &QSpinBox::valueChanged, [this]() {
                    auto&& full_stats = this->get_character_full_stats();

                    QSignalBlocker b { this->starting_class_combobox };
                    auto it = std::ranges::find(
                        calculator::character_class_stats,
                        full_stats,
                        &decltype(calculator::character_class_stats)::value_type::second
                    );
                    if (it != calculator::character_class_stats.end()) 
                        this->starting_class_combobox->setCurrentText(QString::fromStdString(it->first));
                    else
                        this->starting_class_combobox->setCurrentIndex(-1);

                    emit character_stats_changed(full_stats);
                });
            }

            // base game / dlc
            for (auto&& [val, str] : std::views::zip(std::array{false, true}, std::array{"base game", "dlc"}))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->base_game_dlc_list);
                item->setData(Qt::UserRole, val);
            }

            // weapon type list widget
            for (auto&& [type, str] : std::views::zip(enumerator_integrals_of<calculator::Weapon::Type>(), enumerator_strings_of<calculator::Weapon::Type>()))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->type_list);
                item->setData(Qt::UserRole, type);
            }

            // weapon affinity list widget
            for (auto&& [affinity, str] : std::views::zip(enumerator_integrals_of<calculator::Weapon::Affinity>(), enumerator_strings_of<calculator::Weapon::Affinity>()))
            {
                auto item = new QListWidgetItem(string_to_display(str), this->affinity_list);
                item->setData(Qt::UserRole, affinity);
            }
            
            // weapon table view
            this->weapon_table = new WeaponTable(this);
            this->main_layout->addWidget(this->weapon_table, 1);
        };

        calculator::Stats get_character_stats() const
        {
            calculator::Stats stats{};
            for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes | std::views::drop(calculator::irrelevant_attribute_count), stats))
                stat = spinbox->value();
            return stats;
        }

        calculator::FullStats get_character_full_stats() const
        {
            calculator::FullStats full_stats{};
            for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes, full_stats))
                stat = spinbox->value();
            return full_stats;
        }
        
        calculator::UpgradeLevels get_upgrade_levels()
        {
            calculator::UpgradeLevels upgrade_levels{};
            upgrade_levels.at(1) = this->normal_upgrade_level_spinbox->value();
            upgrade_levels.at(2) = this->somber_upgrade_level_spinbox->value();
            return upgrade_levels;
        }
        
        bool get_two_handing()
        {
            return this->two_handing_checkbox->isChecked();
        }
    
        void set_active_weapon_data(std::span<calculator::Weapon> active_weapon_data)
        {
            auto new_base_names = active_weapon_data
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<std::set>()
                | std::views::transform(static_cast<QString(*)(const std::string&)>(string_to_display))
                | std::ranges::to<QList>();
            this->base_name_list->clear();
            this->base_name_list->addItems(new_base_names);

            this->active_weapon_data = active_weapon_data;
        }
    
    signals:
        void character_stats_changed(const calculator::FullStats& full_stats);
    };

    class StatsTab : public StatsTabBase
    {
    public:
        QLabel* character_level_label{};

        explicit StatsTab(QWidget *parent = nullptr) : StatsTabBase(parent)
        {
            // character level label
            this->character_stats_layout->addRow(
                string_to_display("character level:"),
                this->character_level_label = new QLabel(QString::number(this->get_character_full_stats().character_level()))
            );

            // character stats spinboxes
            connect(this, &StatsTabBase::character_stats_changed, [this](const calculator::FullStats& full_stats){
                this->character_level_label->setText(QString::number(full_stats.character_level()));
                this->calculate_weapon_stats();
            });

            // upgrade level spinboxes
            connect(this->normal_upgrade_level_spinbox, &QSpinBox::valueChanged, this, &StatsTab::calculate_weapon_stats);
            connect(this->somber_upgrade_level_spinbox, &QSpinBox::valueChanged, this, &StatsTab::calculate_weapon_stats);

            // two-handing checkbox
            connect(this->two_handing_checkbox, &QCheckBox::checkStateChanged, this, &StatsTab::calculate_weapon_stats);

            // base game / dlc filter
            connect(
                this->base_game_dlc_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<bool> selected;

                    for (QListWidgetItem *item : this->base_game_dlc_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toBool());

                    this->weapon_table->proxy_model->set_selected_base_game_dlc(std::move(selected));
                }
            );

            // weapon type filter
            connect(
                this->type_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<int> selected;

                    for (QListWidgetItem *item : this->type_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toInt());

                    this->weapon_table->proxy_model->set_selected_types(std::move(selected));
                }
            );

            // weapon base name filter
            connect(
                this->base_name_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<QString> selected;

                    for (QListWidgetItem *item : this->base_name_list->selectedItems())
                        selected.insert(item->text());

                    this->weapon_table->proxy_model->set_selected_base_names(std::move(selected));
                }
            );

            // weapon affinity filter
            connect(
                this->affinity_list,
                &QListWidget::itemSelectionChanged,
                this,
                [this]() {
                    QSet<int> selected;

                    for (QListWidgetItem *item : this->affinity_list->selectedItems())
                        selected.insert(item->data(Qt::UserRole).toInt());

                    this->weapon_table->proxy_model->set_selected_affinities(std::move(selected));
                }
            );

            // weapon table view
            this->weapon_table->hide_section<sections::Stats>();
            this->weapon_table->hide_section<sections::CharacterLevelSection>();
        }

        void set_upgrade_levels(const calculator::UpgradeLevels& upgrade_levels)
        {
            QSignalBlocker b1 { this->normal_upgrade_level_spinbox };
            QSignalBlocker b2 { this->somber_upgrade_level_spinbox };

            this->normal_upgrade_level_spinbox->setValue(upgrade_levels.at(1));
            this->somber_upgrade_level_spinbox->setValue(upgrade_levels.at(2));

            this->calculate_weapon_stats();
        }
        
        void set_two_handing(bool two_handing)
        {
            QSignalBlocker b { this->two_handing_checkbox };

            this->two_handing_checkbox->setChecked(two_handing);

            this->calculate_weapon_stats();
        }

        void set_active_weapon_data(std::span<calculator::Weapon> active_weapon_data)
        {
            auto start = std::chrono::high_resolution_clock::now();

            this->StatsTabBase::set_active_weapon_data(active_weapon_data);

            // get character stats
            auto full_stats = this->get_character_full_stats();

            // get attack options
            calculator::AttackOptions attack_options{
                this->get_upgrade_levels(),
                this->get_two_handing()
            };

            this->weapon_table->model->set_rows(
                blocking_progress_bar_dialog(
                    this,
                    "calculating weapon data",
                    [&](){
                        std::vector<Row> result{};
                        
                        result.reserve(this->active_weapon_data.size());
                        result.append_range(this->active_weapon_data
                            | std::views::transform([&](const calculator::Weapon& w) { return Row(w.calculate_attack_rating(attack_options, full_stats)); })
                        );

                        return result;
                    }
                )
            );

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("StatsTab::set_active_weapon_data: {} seconds", elapsed.count());
        
            QTimer::singleShot(0, this->weapon_table, &WeaponTable::resize_columns_to_contents);
        }
        
        void calculate_weapon_stats()
        {
            // get character stats
            auto full_stats = this->get_character_full_stats();

            // get attack options
            calculator::AttackOptions attack_options{
                this->get_upgrade_levels(),
                this->get_two_handing()
            };

            auto start = std::chrono::high_resolution_clock::now();

            std::ranges::for_each(std::views::zip(this->active_weapon_data, this->weapon_table->model->rows), [&](auto&& pair) {
                auto&& [w, row] = pair;
                row.update(w.calculate_attack_rating(attack_options, full_stats));
            });
            // this->weapon_table->model->rows.clear();
            // this->weapon_table->model->rows.append_range(
            //     this->active_weapon_data
            //         | std::views::transform([&](const calculator::Weapon& w) { return Row(w.calculate_attack_rating(attack_options, full_stats)); })
            // );

            this->weapon_table->model->notifyAllChanged();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("StatsTab::calculate_weapon_stats: {} seconds", elapsed.count());

            QTimer::singleShot(0, this->weapon_table, &WeaponTable::resize_columns_to_contents);
        }
    };

    class OptimizeTab : public StatsTabBase
    {
        std::vector<std::reference_wrapper<const calculator::Weapon>> filtered_active_weapon_data{};

        QSpinBox* max_character_level_spinbox{};
        QLabel* max_attribute_points_label{};
        QLabel* max_free_attribute_points_label{};
        QLabel* stat_variations_label{};
        Ui::OptimizeWidget optimize;

        void prepare_optimization()
        {
            auto full_stats = this->get_character_full_stats();

            auto min_attribute_points = full_stats.attribute_points();
            auto max_attribute_points = calculator::character_level_to_attribute_points(this->max_character_level_spinbox->value());
            auto max_free_attribute_points = max_attribute_points - min_attribute_points;
            auto stat_variations = calculator::get_stat_variation_count(max_attribute_points, full_stats);
            
            this->max_attribute_points_label->setText(QString::number(max_attribute_points));
            this->max_free_attribute_points_label->setText(QString::number(max_free_attribute_points));
            this->stat_variations_label->setText(QString::number(stat_variations));

            // base game / dlc filter
            auto selected_base_game_dlc = this->base_game_dlc_list->selectedItems();
            std::unordered_set<bool> base_game_dlc_set;
            base_game_dlc_set.reserve(selected_base_game_dlc.size());
            for (QListWidgetItem *item : selected_base_game_dlc)
                base_game_dlc_set.insert(item->data(Qt::UserRole).toBool());

            // weapon type filter
            auto selected_types = this->type_list->selectedItems();
            std::unordered_set<int> type_set;
            type_set.reserve(selected_types.size());
            for (QListWidgetItem *item : selected_types)
                type_set.insert(item->data(Qt::UserRole).toInt());

            // weapon base name filter
            auto selected_base_names = this->base_name_list->selectedItems();
            std::unordered_set<std::string> base_name_set;
            base_name_set.reserve(selected_base_names.size());
            for (QListWidgetItem *item : selected_base_names)
                base_name_set.insert(item->text().toStdString());

            // weapon affinity filter
            auto selected_affinity = this->affinity_list->selectedItems();
            std::unordered_set<int> affinity_set;
            affinity_set.reserve(selected_affinity.size());
            for (QListWidgetItem *item : selected_affinity)
                affinity_set.insert(item->data(Qt::UserRole).toInt());

            this->filtered_active_weapon_data.clear();
            this->filtered_active_weapon_data.reserve(this->active_weapon_data.size());
            this->filtered_active_weapon_data.append_range(this->active_weapon_data
                | std::views::filter([&](const calculator::Weapon& w) {
                    if (!base_game_dlc_set.empty() && !base_game_dlc_set.contains(w.dlc))
                        return false;
                    if (!type_set.empty() && !type_set.contains(std::to_underlying(w.type)))
                        return false;
                    if (!base_name_set.empty() && !base_name_set.contains(w.base_name))
                        return false;
                    if (!affinity_set.empty() && !affinity_set.contains(std::to_underlying(w.affinity)))
                        return false;
                    return true;
                })
            );

            this->optimize.weapons_label->setText(QString::number(this->filtered_active_weapon_data.size()));
            this->optimize.variations_label->setText(QString::number(this->filtered_active_weapon_data.size() * stat_variations));
        }

        void optimize_brute_force()
        {

        }
        void optimize_v2()
        {
            
        }

    public:
        explicit OptimizeTab(QWidget *parent = nullptr) : StatsTabBase(parent)
        {
            // max character level label
            this->character_stats_layout->addRow(string_to_display("max character level:"), this->max_character_level_spinbox = new QSpinBox());
            this->max_character_level_spinbox->setMinimum(1);
            calculator::FullStats max_stats{};
            max_stats.fill(99);
            this->max_character_level_spinbox->setMaximum(max_stats.character_level());
            connect(this->max_character_level_spinbox, &QSpinBox::valueChanged, this, &OptimizeTab::prepare_optimization);

            // attribute points label
            this->character_stats_layout->addRow(string_to_display("max attribute points:"), this->max_attribute_points_label = new QLabel());
            this->character_stats_layout->addRow(string_to_display("max free attribute points:"), this->max_free_attribute_points_label = new QLabel());

            // stat variations label
            this->character_stats_layout->addRow(string_to_display("stat variations:"), this->stat_variations_label = new QLabel());

            // character stats spinboxes
            connect(this, &StatsTabBase::character_stats_changed, this, &OptimizeTab::prepare_optimization);

            // optimize widget
            auto temp_layout = new QVBoxLayout();
            this->horizontal_layout->addLayout(temp_layout);
            auto opt_group = new QGroupBox();
            temp_layout->addWidget(opt_group);
            temp_layout->addStretch(1);
            this->optimize.setupUi(opt_group);

            // optimize buttons
            connect(this->optimize.start_brute_force_button, &QPushButton::clicked, this, &OptimizeTab::optimize_brute_force);
            connect(this->optimize.start_v2_button, &QPushButton::clicked, this, &OptimizeTab::optimize_v2);
        }
    
        void set_active_weapon_data(std::span<calculator::Weapon> active_weapon_data)
        {
            auto start = std::chrono::high_resolution_clock::now();

            this->StatsTabBase::set_active_weapon_data(active_weapon_data);
            this->prepare_optimization();

            this->weapon_table->model->set_rows({});

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("OptimizeTab::set_active_weapon_data: {} seconds", elapsed.count());
        }
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

        std::vector<calculator::Weapon> active_weapon_data{};

    public:
        explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
        {
            // setup
            this->ui->setupUi(this);
            this->setWindowTitle(string_to_display(this->windowTitle()));
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

            // add tabs
            this->ui->tab_widget->addTab(stats, string_to_display("stats"));
            this->ui->tab_widget->addTab(optimize, string_to_display("optimize"));
            this->ui->tab_widget->addTab(plot, string_to_display("plot"));
        }

        std::span<const calculator::Weapon> get_active_weapon_data() const
        {
            return std::span<const calculator::Weapon>(this->active_weapon_data);
        }
        void set_active_weapon_data(const std::filesystem::path& dir)
        {
            auto start = std::chrono::high_resolution_clock::now();

            this->active_weapon_data = blocking_progress_bar_dialog(
                this,
                "loading weapon data",
                [&](){ return parser::load_weapons(dir); }
            );

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("MainWindow::set_active_weapon_data: {} seconds", elapsed.count());

            this->stats->set_active_weapon_data(this->active_weapon_data);
            this->optimize->set_active_weapon_data(this->active_weapon_data);
        }
    };

    export int run_ui(int argc, char *argv[]) {
        QApplication app(argc, argv);

        MainWindow window{};
        window.showMaximized();

        return app.exec();
    }
}

#include "ui.moc"