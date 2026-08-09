module;
#include <QMainWindow>
#include <QPainter>
#include <QActionGroup>
#include <QProgressDialog>
#include <QFuture>
#include <QtConcurrent>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include "ui_main_window.h"
#include "ui_stats_tab.h"
#include "ui_optimize_widget.h"
#include "ui_plot_tab.h"
export module erdo.ui;

import std;

import erdo;

import erdo.ui.settings;
import erdo.ui.weapons_table;


namespace erdo::ui
{
    template<typename T>
    bool execute_future_with_blocking_progress_bar(QFuture<T>& future, QWidget* parent, const QString& labelText, bool cancelable)
    {
        QFutureWatcher<T> watcher;
        watcher.setFuture(future);

        const QString cancelButtonText =
            cancelable ? QObject::tr("Cancel") : QString{};

        QProgressDialog progress(
            labelText,
            cancelButtonText,
            0,
            0,
            parent);

        progress.setWindowModality(Qt::ApplicationModal);
        progress.setMinimumDuration(0);

        if (!cancelable)
            progress.setWindowFlags(progress.windowFlags() & ~Qt::WindowCloseButtonHint);

        // We provide our own progress text, so hide QProgressBar's "xx%" overlay.
        if (auto* bar = progress.findChild<QProgressBar*>())
            bar->setTextVisible(false);

        // Busy indicator until the future reports its range.
        progress.setRange(0, 0);

        // ---------------------------------------------------------------------
        // ETA state
        // ---------------------------------------------------------------------

        QElapsedTimer timer;
        timer.start();

        QString progressText;

        QString cachedEta;
        qint64 lastEtaUpdate = -1000;

        auto formatDuration = [](qint64 seconds)
        {
            if (seconds < 60)
                return QObject::tr("%1 s").arg(seconds);

            if (seconds < 3600)
                return QTime(0, 0).addSecs(int(seconds)).toString("mm:ss");

            const int hours = int(seconds / 3600);
            const int minutes = int((seconds % 3600) / 60);

            return QObject::tr("%1h %2m")
                .arg(hours)
                .arg(minutes, 2, 10, QLatin1Char('0'));
        };

        auto updateLabel = [&]()
        {
            QString text = labelText;

            if (!progressText.isEmpty())
                text += '\n' + progressText;

            const int min = progress.minimum();
            const int max = progress.maximum();
            const int value = progress.value();

            if (max > min)
            {
                const int completed = value - min;
                const int total = max - min;

                const double fraction =
                    double(completed) / double(total);

                QString status =
                    QString("%1/%2 (%3%)")
                        .arg(completed)
                        .arg(total)
                        .arg(qRound(fraction * 100.0));

                // Don't show ETA before we have enough information.
                // 5% is a reasonable start for hundreds of tasks.
                if (completed > 0 && fraction >= 0.05)
                {
                    const qint64 now = timer.elapsed();

                    if (now - lastEtaUpdate >= 1000)
                    {
                        const double averageMsPerTask =
                            double(now) / double(completed);

                        const qint64 remainingMs =
                            qRound64(
                                averageMsPerTask *
                                double(total - completed));

                        cachedEta =
                            formatDuration(remainingMs / 1000);

                        lastEtaUpdate = now;
                    }

                    if (!cachedEta.isEmpty())
                        status += QObject::tr(" • ETA %1")
                                    .arg(cachedEta);
                }

                text += '\n' + status;
            }

            progress.setLabelText(text);
        };

        // ---------------------------------------------------------------------
        // Connections
        // ---------------------------------------------------------------------

        QObject::connect(
            &watcher,
            &QFutureWatcher<T>::progressRangeChanged,
            &progress,
            [&](int min, int max)
            {
                progress.setRange(min, max);

                cachedEta.clear();
                lastEtaUpdate = -1000;

                updateLabel();
            });

        QObject::connect(
            &watcher,
            &QFutureWatcher<T>::progressTextChanged,
            [&](const QString& text)
            {
                progressText = text;
                updateLabel();
            });

        QObject::connect(
            &watcher,
            &QFutureWatcher<T>::progressValueChanged,
            [&](int value)
            {
                progress.setValue(value);
                updateLabel();
            });

        QObject::connect(
            &watcher,
            &QFutureWatcher<T>::finished,
            &progress,
            &QDialog::accept);

        QObject::connect(
            &progress,
            &QProgressDialog::canceled,
            [&]()
            {
                future.cancel();
            });

        progress.exec();

        future.waitForFinished();

        return !future.isCanceled();
    }

    class StatsTabBase : public QWidget, public Ui::StatsTab
    {
        Q_OBJECT

    protected:
    public:
        std::span<const calculator::Weapon> active_weapon_data{};
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

                auto&& stats = calculator::character_class_stats.at(text.toStdString());
                for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes, stats))
                {
                    QSignalBlocker b { spinbox };
                    spinbox->setValue(stat);
                }

                emit character_stats_changed(stats);
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
                    auto&& stats = this->get_character_stats();

                    QSignalBlocker b { this->starting_class_combobox };
                    auto it = std::ranges::find(
                        calculator::character_class_stats,
                        stats,
                        &decltype(calculator::character_class_stats)::value_type::second
                    );
                    if (it != calculator::character_class_stats.end()) 
                        this->starting_class_combobox->setCurrentText(QString::fromStdString(it->first));
                    else
                        this->starting_class_combobox->setCurrentIndex(-1);

                    emit character_stats_changed(stats);
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
            for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes, stats))
                stat = spinbox->value();
            return stats;
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
    
        calculator::AttackOptions get_attack_options()
        {
            return calculator::AttackOptions{
                this->get_upgrade_levels(),
                this->get_two_handing()
            };
        }

        void set_active_weapon_data(std::span<const calculator::Weapon> active_weapon_data)
        {
            auto new_base_names = active_weapon_data
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<std::set>();

            this->base_name_list->clear();
            for (auto&& base_name : new_base_names)
            {
                auto item = new QListWidgetItem(string_to_display(base_name), this->base_name_list);
                item->setData(Qt::UserRole, QString::fromStdString(base_name));
            }

            this->active_weapon_data = active_weapon_data;
        }
    
    signals:
        void character_stats_changed(const calculator::Stats& stats);
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
                this->character_level_label = new QLabel(QString::number(this->get_character_stats().character_level()))
            );

            // character stats spinboxes
            connect(this, &StatsTabBase::character_stats_changed, [this](const calculator::Stats& stats){
                this->character_level_label->setText(QString::number(stats.character_level()));
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

        void set_active_weapon_data(std::span<const calculator::Weapon> active_weapon_data)
        {
            auto start = std::chrono::high_resolution_clock::now();

            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            auto attack_options = this->get_attack_options();

            // temporary attack rating object to avoid copying the weapon data multiple times
            calculator::AttackRating attack_rating{ calculator::Weapon::dummy, stats, attack_options };

            std::vector<Row> rows{};
            rows.reserve(active_weapon_data.size());
            rows.append_range(active_weapon_data
                | std::views::transform([&](const calculator::Weapon& w) {
                    attack_rating.weapon = w;
                    attack_rating.calculate_inplace();
                    return Row(std::move(attack_rating));
                })
            );
            this->StatsTabBase::set_active_weapon_data(active_weapon_data);
            this->weapon_table->model->set_rows(std::move(rows));

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("StatsTab::set_active_weapon_data: {} seconds", elapsed.count());
        }
        
        void calculate_weapon_stats()
        {
            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            auto attack_options = this->get_attack_options();

            // temporary attack rating object to avoid copying the weapon data multiple times
            calculator::AttackRating attack_rating{ calculator::Weapon::dummy, stats, attack_options };

            auto start = std::chrono::high_resolution_clock::now();

            this->weapon_table->model->update_rows(
                this->active_weapon_data | std::views::transform([&](const calculator::Weapon& w)->calculator::AttackRating&& {
                    attack_rating.weapon = w;
                    attack_rating.calculate_inplace();
                    return std::move(attack_rating);
                })
            );

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("StatsTab::calculate_weapon_stats: {} seconds", elapsed.count());
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
            auto stats = this->get_character_stats();

            auto min_attribute_points = stats.attribute_points();
            auto max_attribute_points = calculator::character_level_to_attribute_points(this->max_character_level_spinbox->value());
            auto max_free_attribute_points = max_attribute_points - min_attribute_points;
            auto stat_variations = calculator::get_stat_variation_count(max_attribute_points, stats);
            
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
                base_name_set.insert(item->data(Qt::UserRole).toString().toStdString());

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
            auto start = std::chrono::high_resolution_clock::now();

            auto stats = this->get_character_stats();
            auto attack_options = this->get_attack_options();
            auto max_attribute_points = calculator::character_level_to_attribute_points(this->max_character_level_spinbox->value());
            auto stat_variations = calculator::get_stat_variations(max_attribute_points, stats);

            static constexpr auto optimizer_callbacks = [](auto){
                static constexpr auto [...enumerators] = enumerators_of<calculator::OptimizationTarget>();
                return std::array{ calculator::optimizers<enumerators>.get_callback... };
            }(1);

            std::vector<Row> rows{};
            if (stat_variations.size() > 0)
            {
                auto target_index = this->optimize.target_combobox->currentIndex();
                auto&& callback = optimizer_callbacks.at(target_index)(stat_variations, attack_options);

                auto future = QtConcurrent::mapped(
                    this->filtered_active_weapon_data,
                    [&](const calculator::Weapon& w){ return Row(callback(w)); }
                );

                auto success = execute_future_with_blocking_progress_bar(future, this, "optimizing...", true);

                if (success)
                {
                    rows.reserve(this->filtered_active_weapon_data.size());
                    rows.append_range(future | std::views::as_rvalue);
                }
            }
            this->weapon_table->model->set_rows(std::move(rows));
            // this->weapon_table->resize_columns_to_contents();

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("OptimizeTab::optimize_brute_force: {} seconds", elapsed.count());
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
            calculator::Stats max_stats{};
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

            // base game / dlc filter
            connect(this->base_game_dlc_list, &QListWidget::itemSelectionChanged, this, &OptimizeTab::prepare_optimization);

            // weapon type filter
            connect(this->type_list, &QListWidget::itemSelectionChanged, this, &OptimizeTab::prepare_optimization);

            // weapon base name filter
            connect(this->base_name_list, &QListWidget::itemSelectionChanged, this, &OptimizeTab::prepare_optimization);

            // weapon affinity filter
            connect(this->affinity_list, &QListWidget::itemSelectionChanged, this, &OptimizeTab::prepare_optimization);

            // optimize widget
            auto temp_layout = new QVBoxLayout();
            this->horizontal_layout->addLayout(temp_layout);
            auto opt_group = new QGroupBox();
            temp_layout->addWidget(opt_group);
            temp_layout->addStretch(1);
            this->optimize.setupUi(opt_group);

            // optimize target combobox
            for (const auto& target : enumerator_strings_of<calculator::OptimizationTarget>())
                this->optimize.target_combobox->addItem(string_to_display(target));
            this->optimize.target_combobox->setCurrentIndex(std::to_underlying(calculator::OptimizationTarget::TOTAL_ATTACK_POWER));
            
            // optimize buttons
            connect(this->optimize.start_brute_force_button, &QPushButton::clicked, this, &OptimizeTab::optimize_brute_force);
            connect(this->optimize.start_v2_button, &QPushButton::clicked, this, &OptimizeTab::optimize_v2);
        }
    
        void set_active_weapon_data(std::span<const calculator::Weapon> active_weapon_data)
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
        QActionGroup* menu_weapon_data_group = new QActionGroup(this);
        QAction* menu_weapon_data_seperator;
        StatsTab* stats = new StatsTab();
        OptimizeTab* optimize = new OptimizeTab();
        PlotTab* plot = new PlotTab();

        std::vector<calculator::Weapon> active_weapon_data{};

        std::span<const calculator::Weapon> get_active_weapon_data() const
        {
            return std::span<const calculator::Weapon>(this->active_weapon_data);
        }
        void set_active_weapon_data(const std::filesystem::path& dir)
        {
            auto start = std::chrono::high_resolution_clock::now();

            auto future = QtConcurrent::run([&](){ return parser::load_weapons(dir); });
            execute_future_with_blocking_progress_bar(future, this, "loading weapon data...", false);
            this->active_weapon_data = future.takeResult();

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::println("MainWindow::set_active_weapon_data: {} seconds", elapsed.count());

            this->stats->set_active_weapon_data(this->active_weapon_data);
            this->optimize->set_active_weapon_data(this->active_weapon_data);
        }

        QAction* add_weapon_data(std::filesystem::path dir)
        {
            dir = std::filesystem::canonical(dir).make_preferred();
            if (!std::filesystem::is_directory(dir))
            {
                QMessageBox::critical(this, "invalid directory", std::format("not a directory: {}", dir).c_str());
                return nullptr;
            }

            auto action_text = dir.string();
            auto version_string = dir.filename().string();

            if (version_string.size() == 8)
            {
                std::size_t version_number;
                auto result = std::from_chars(version_string.data(), version_string.data() + version_string.size(), version_number);
                if (result)
                {
                    auto major = version_string.subview(0, 1);
                    auto minor = version_string.subview(1, 2);
                    auto patch = version_string.subview(3, std::string::npos);
                    while (patch.ends_with('0'))
                        patch.remove_suffix(1);
                    version_string = std::format("{}.{}.{}", major, minor, patch);
                    while(version_string.ends_with('.'))
                        version_string.pop_back();
                    action_text = std::format("{} ({})", action_text, version_string);
                }
            }

            QAction *action = new QAction(QString::fromStdString(action_text), this->ui->menu_file);
            this->ui->menu_file->insertAction(this->menu_weapon_data_seperator, action);
            action->setCheckable(true);

            this->menu_weapon_data_group->addAction(action);
            connect(action, &QAction::triggered, this, [this, dir]() { this->set_active_weapon_data(dir); });
            return action;
        }

        void load_weapon_data_from_directory()
        {
            QString directory = QFileDialog::getExistingDirectory(
                this,
                "select weapon data directory",
                QDir::homePath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );

            if (directory.isEmpty())
                return;
            
            this->add_weapon_data(std::filesystem::path(directory.toStdString()));
        }

        void generate_weapon_data_from_game_data()
        {
            QString file_name = QFileDialog::getOpenFileName(
                this,
                "select eldenring.exe",
                QDir::homePath(),
                "elden ring executable (eldenring.exe)"
            );
            if (file_name.isEmpty())
                return;
            auto elden_ring_executable = std::filesystem::path(file_name.toStdString());
            if (elden_ring_executable.filename() != "eldenring.exe")
            {
                QMessageBox::critical(this, "invalid file", "please select eldenring.exe");
                return;
            }

            file_name = QFileDialog::getOpenFileName(
                this,
                "select WitchyBND.exe",
                QDir::homePath(),
                "WitchyBND executable (WitchyBND.exe)"
            );
            if (file_name.isEmpty())
                return;
            auto witchybdn_executable = std::filesystem::path(file_name.toStdString());
            if (witchybdn_executable.filename() != "WitchyBND.exe")
            {
                QMessageBox::critical(this, "invalid file", "please select WitchyBND.exe");
                return;
            }

            QString directory = QFileDialog::getExistingDirectory(
                this,
                "select a save directory",
                QDir::homePath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );
            if (directory.isEmpty())
                return;
            auto save_directory = std::filesystem::path(directory.toStdString());
            if (!std::filesystem::is_directory(save_directory))
            {
                QMessageBox::critical(this, "invalid directory", std::format("not a directory: {}", save_directory).c_str());
                return;
            }

            auto future = QtConcurrent::run([&](){
                witchy::run_witchy(
                    elden_ring_executable.parent_path(),
                    witchybdn_executable,
                    save_directory
                );
            });
            execute_future_with_blocking_progress_bar(future, this, "generating weapon data...", false);
        }

        void closeEvent(QCloseEvent *event) override
        {
            // save geometry and state
            auto&& s = settings().settings;
            s.beginGroup("MainWindow");
            s.setValue("geometry", this->saveGeometry());
            s.setValue("state", this->saveState());
            s.endGroup();
            
            QMainWindow::closeEvent(event);
        }

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
            this->menu_weapon_data_seperator = this->ui->menu_file->addSeparator();
            this->menu_weapon_data_group->setExclusive(true);
            for (auto&& [i, dir] : weapon_data_directories | std::views::enumerate)
            {
                auto action = this->add_weapon_data(dir);
                if (action == nullptr)
                    std::terminate();
                if (i == 0)
                    QTimer::singleShot(0, action, &QAction::trigger);
            }
            QAction* action = this->ui->menu_file->addAction("load weapon data from directory");
            connect(action, &QAction::triggered, this, &MainWindow::load_weapon_data_from_directory);
            action = this->ui->menu_file->addAction("generate weapon data from game data");
            connect(action, &QAction::triggered, this, &MainWindow::generate_weapon_data_from_game_data);
            action = this->ui->menu_file->addAction("settings");
            connect(action, &QAction::triggered, [](){ settings().dialog->exec(); });

            // add tabs
            this->ui->tab_widget->addTab(stats, string_to_display("stats"));
            this->ui->tab_widget->addTab(optimize, string_to_display("optimize"));
            this->ui->tab_widget->addTab(plot, string_to_display("plot"));

            // restore geometry and state
            auto&& s = settings().settings;
            s.beginGroup("MainWindow");
            const auto geometry = s.value("geometry", QByteArray()).toByteArray();
            if (!geometry.isEmpty())
                this->restoreGeometry(geometry);
            const auto state = s.value("state", QByteArray()).toByteArray();
            if (state.isEmpty())
                this->setWindowState(Qt::WindowMaximized);
            else
                this->restoreState(state);
            s.endGroup();
        }
    };

    export int run_ui(int argc, char *argv[]) {
        QApplication app(argc, argv);
        app.setOrganizationName("hanslhansl");
        app.setApplicationName("elden-ring-damage-optimizer");

        MainWindow window{};
        window.show();

        return app.exec();
    }
}

#include "ui.moc"