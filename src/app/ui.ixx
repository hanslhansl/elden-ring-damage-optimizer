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
#include <QDesktopServices>
#include <QUrl>
#include "ui_main_window.h"
#include "ui_stats_tab.h"
#include "ui_optimize_widget.h"
export module erdo.ui;

import std;

import erdo;

import erdo.ui.settings;
import erdo.ui.weapons_table;
import erdo.ui.plot_tab;


export namespace erdo::ui
{
    int run_ui(int argc, char *argv[]);
}

module : private;

namespace erdo::ui
{
    void critical_error(QWidget* parent, const QString& message)
    {
        QMessageBox::critical(parent, "Fatal Error", message);
        QCoreApplication::exit(1);
    }

    template<bool cancelable, typename T>
    auto execute_future_with_blocking_progress_bar(QFuture<T>& future, QWidget* parent, const QString& labelText)
    {
        QFutureWatcher<T> watcher;
        watcher.setFuture(future);

        const QString cancelButtonText = cancelable ? "Cancel" : QString{};

        QProgressDialog progress(
            labelText,
            cancelButtonText,
            0,
            0,
            parent);

        progress.setWindowModality(Qt::ApplicationModal);
        progress.setMinimumDuration(0);

        // if (!cancelable)
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
                        const double averageMsPerTask = double(now) / double(completed);

                        const qint64 remainingMs =
                            qRound64(
                                averageMsPerTask *
                                double(total - completed));

                        cachedEta = formatDuration(remainingMs / 1000);

                        lastEtaUpdate = now;
                    }

                    if (!cachedEta.isEmpty())
                        status += QObject::tr(" • ETA %1").arg(cachedEta);
                }

                text += '\n' + status;
            }

            progress.setLabelText(text);
        };

        // ---------------------------------------------------------------------
        // Connections
        // ---------------------------------------------------------------------

        QObject::connect(&watcher, &QFutureWatcher<T>::progressRangeChanged, &progress, [&](int min, int max) {
            progress.setRange(min, max);

            cachedEta.clear();
            lastEtaUpdate = -1000;

            updateLabel();
        });

        QObject::connect(&watcher, &QFutureWatcher<T>::progressTextChanged, [&](const QString& text) {
            progressText = text;
            updateLabel();
        });

        QObject::connect(&watcher, &QFutureWatcher<T>::progressValueChanged, [&](int value) {
            progress.setValue(value);
            updateLabel();
        });

        QObject::connect(&watcher, &QFutureWatcher<T>::finished, &progress, &QDialog::accept);

        QObject::disconnect(&progress, &QProgressDialog::canceled, nullptr, nullptr);
        QObject::connect(&progress,  &QProgressDialog::canceled, [&]() {
            // future.cancel();
            progress.setCancelButton(nullptr);
            progress.setLabelText("Canceling…");
            progress.setRange(0, 0);
            future.cancel();
        });

        progress.exec();

        future.waitForFinished();

        if constexpr (cancelable)
            return !future.isCanceled();
        else if(!future.isValid())
            critical_error(parent, "Unable to continue.");
    }

    class StatsTabBase : public QWidget, public Ui::StatsTab
    {
        Q_OBJECT

    protected:
        std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data{};
        std::vector<QSpinBox*> attribute_spinboxes{};

        QSet<bool> base_game_dlc_filter{};
        QSet<int> type_filter{};
        QSet<QString> base_name_filter{};
        QSet<int> affinity_filter{};

        void adjust_base_game_dlc_filter()
        {
            auto visible_base_game_dlc = *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::dlc)
                | std::ranges::to<QSet>();

            this->base_game_dlc_filter.clear();
            for (int i = 0; i < this->base_game_dlc_list->count(); ++i)
            {
                auto item = this->base_game_dlc_list->item(i);
                const auto dlc_value = item->data(Qt::UserRole).toBool();
                auto visible = true;
                item->setHidden(!visible);
                if (visible && item->isSelected())
                    this->base_game_dlc_filter.insert(dlc_value);
            }
            this->adjust_type_filter();
        }
        void adjust_type_filter()
        {
            auto visible_types = *this->active_weapon_data
                | std::views::filter([&](const calculator::Weapon& w){
                    return this->base_game_dlc_filter.isEmpty() || this->base_game_dlc_filter.contains(w.dlc);
                })
                | std::views::transform(&calculator::Weapon::type)
                | std::ranges::to<QSet>();

            this->type_filter.clear();
            for (int i = 0; i < this->type_list->count(); ++i)
            {
                auto item = this->type_list->item(i);
                const int type_value = item->data(Qt::UserRole).toInt();
                auto visible = visible_types.isEmpty() || visible_types.contains(static_cast<calculator::Weapon::Type>(type_value));
                item->setHidden(!visible);
                if (visible && item->isSelected())
                    this->type_filter.insert(type_value);
            }

            this->adjust_base_name_filter();
        }
        
        void adjust_base_name_filter()
        {
            auto filterable_base_names = *this->active_weapon_data
                | std::views::filter([&](const calculator::Weapon& w) {
                    return (this->base_game_dlc_filter.isEmpty() ||
                            this->base_game_dlc_filter.contains(w.dlc)) &&
                        (this->type_filter.isEmpty() ||
                            this->type_filter.contains(std::to_underlying(w.type)));
                })
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<QSet>();

            this->base_name_filter.clear();

            const QString query = this->base_name_line_edit->text().trimmed();

            for (int i = 0; i < this->base_name_list->count(); ++i)
            {
                auto item = this->base_name_list->item(i);
                const QString base_name = item->text();

                // Determined by the filters higher in the hierarchy.
                const bool filter_visible =
                    filterable_base_names.isEmpty() ||
                    filterable_base_names.contains(base_name.toStdString());

                // Only affects visual visibility, never the actual filter.
                const bool search_match =
                    query.isEmpty() ||
                    base_name.contains(query, Qt::CaseInsensitive);

                const bool visible =
                    filter_visible && (search_match || item->isSelected());

                item->setHidden(!visible);

                // Search does NOT affect the filter.
                if (filter_visible && item->isSelected())
                    this->base_name_filter.insert(base_name);
            }

            this->adjust_affinity_list_filter();
        }

        void adjust_affinity_list_filter()
        {
            auto visible_affinities = *this->active_weapon_data
                | std::views::filter([&](const calculator::Weapon& w){
                    return  (this->base_game_dlc_filter.isEmpty() || this->base_game_dlc_filter.contains(w.dlc)) &&
                            (this->type_filter.isEmpty() || this->type_filter.contains(std::to_underlying(w.type))) &&
                            (this->base_name_filter.isEmpty() || this->base_name_filter.contains(QString::fromStdString(w.base_name)));
                })
                | std::views::transform(&calculator::Weapon::affinity)
                | std::ranges::to<QSet>();

            this->affinity_filter.clear();
            for (int i = 0; i < this->affinity_list->count(); ++i)
            {
                auto item = this->affinity_list->item(i);
                const int affinity_value = item->data(Qt::UserRole).toInt();
                auto visible = visible_affinities.isEmpty() || visible_affinities.contains(static_cast<calculator::Weapon::Affinity>(affinity_value));
                item->setHidden(!visible);
                if (visible && item->isSelected())
                    this->affinity_filter.insert(affinity_value);
            }

            emit this->filter_changed();
        }

    public:
        using Row = decltype([](auto){
            static constexpr auto [...apts] = enumerators_of<calculator::AttackPowerType>();

            return BasicRow<
                sections::NameSection,
                sections::BaseNameSection,
                sections::AffinitySection,
                sections::TypeSection,
                sections::BaseGameDLCSection,
                sections::UpgradeLevelSection,
                sections::TwoHandingSection,
                sections::CharacterLevelSection,

                sections::AttackPowers,
                sections::StatusEffects,
                sections::SpellScaling,
                sections::Stats,
                sections::Requirements,
                sections::AttributeScalings,
                sections::AttackPowerTypeAttributeScalings<apts>...
            >{};
        }(1));

        WeaponTable<Row>* weapon_table{};

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
                attribute_spinbox->setMinimum(0);
                attribute_spinbox->setMaximum(calculator::attribute_level_limit);

                this->character_stats_layout->insertRow(
                    this->character_stats_layout->rowCount() - 1,
                    enum_to_display(attribute) + ":", attribute_spinbox
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

            // character level label
            this->character_level_label->setText(QString::number(this->get_character_stats().character_level()));
            connect(this, &StatsTabBase::character_stats_changed, [this](const calculator::AttributeLevels& stats){
                this->character_level_label->setText(QString::number(stats.character_level()));
            });

            // filters
            connect(this->base_game_dlc_list, &QListWidget::itemSelectionChanged, this, &StatsTabBase::adjust_base_game_dlc_filter);
            connect(this->type_list, &QListWidget::itemSelectionChanged, this, &StatsTabBase::adjust_type_filter);
            connect(this->base_name_list, &QListWidget::itemSelectionChanged, this, &StatsTabBase::adjust_base_name_filter);
            connect(this->affinity_list, &QListWidget::itemSelectionChanged, this, &StatsTabBase::adjust_affinity_list_filter);

            // search bar for the weapon base name filter
            connect(this->base_name_line_edit, &QLineEdit::textChanged, this, &StatsTabBase::adjust_base_name_filter);

            // weapon table view
            this->weapon_table = new WeaponTable<Row>(false, "No items to display", this);
            this->weapon_table->set_section_hidden<sections::UpgradeLevelSection>(true);
            this->weapon_table->set_section_hidden<sections::TwoHandingSection>(true);

            this->main_layout->addWidget(this->weapon_table, 1);
        };

        calculator::AttributeLevels get_character_stats() const
        {
            calculator::AttributeLevels stats{};
            for (auto&& [spinbox, stat] : std::views::zip(this->attribute_spinboxes, stats))
                stat = spinbox->value();
            return stats;
        }
        calculator::AttackOptions get_attack_options()
        {
            return calculator::AttackOptions{
                {
                    0,
                    (unsigned int)this->normal_upgrade_level_spinbox->value(),
                    (unsigned int)this->somber_upgrade_level_spinbox->value()
                },
                this->two_handing_checkbox->isChecked()
            };
        }

        void set_active_weapon_data(std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data)
        {
            this->active_weapon_data = std::move(active_weapon_data);

            // base game / dlc
            this->base_game_dlc_filter.clear();
            this->base_game_dlc_list->clear();
            for (auto&& dlc : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::dlc)
                | std::ranges::to<std::set>()
            )
            {
                auto item = new QListWidgetItem(dlc ? "DLC" : "Base Game", this->base_game_dlc_list);
                item->setData(Qt::UserRole, dlc);
            }

            // type list widget
            this->type_filter.clear();
            this->type_list->clear();
            for (auto&& type : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::type)
                | std::ranges::to<std::set>()
            )
            {
                auto item = new QListWidgetItem(enum_to_display(type), this->type_list);
                item->setData(Qt::UserRole, std::to_underlying(type));
            }

            // base name list widget
            this->base_name_filter.clear();
            this->base_name_list->clear();
            for (auto&& base_name : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<std::set>()
            )
            {
                auto item = new QListWidgetItem(QString::fromStdString(base_name), this->base_name_list);
            }

            // affinity list widget
            this->affinity_filter.clear();
            this->affinity_list->clear();
            for (auto&& affinity : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::affinity)
                | std::ranges::to<std::set>()
            )
            {
                auto item = new QListWidgetItem(enum_to_display(affinity), this->affinity_list);
                item->setData(Qt::UserRole, std::to_underlying(affinity));
            }
            
            this->adjust_base_game_dlc_filter();
        }
    
    signals:
        void character_stats_changed(const calculator::AttributeLevels& stats);
        void filter_changed();
    };

    class StatsTab : public StatsTabBase
    {
    public:
        explicit StatsTab(QWidget *parent = nullptr) : StatsTabBase(parent)
        {
            // character stats spinboxes
            connect(this, &StatsTabBase::character_stats_changed, this, &StatsTab::calculate_weapon_stats);

            // upgrade level spinboxes
            connect(this->normal_upgrade_level_spinbox, &QSpinBox::valueChanged, this, &StatsTab::calculate_weapon_stats);
            connect(this->somber_upgrade_level_spinbox, &QSpinBox::valueChanged, this, &StatsTab::calculate_weapon_stats);

            // two-handing checkbox
            connect(this->two_handing_checkbox, &QCheckBox::checkStateChanged, this, &StatsTab::calculate_weapon_stats);

            // filters
            connect(this, &StatsTabBase::filter_changed,
                [this]() { this->weapon_table->proxy_model->set_filters(
                    this->base_game_dlc_filter,
                    this->type_filter,
                    this->base_name_filter,
                    this->affinity_filter
                );
            });

            // weapon table view
            this->weapon_table->set_section_hidden<sections::CharacterLevelSection>(true);
            this->weapon_table->set_section_hidden<sections::Stats>(true);
        }

        void set_active_weapon_data(std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data)
        {
            this->StatsTabBase::set_active_weapon_data(std::move(active_weapon_data));

            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            auto attack_options = this->get_attack_options();

            // temporary attack object to avoid copying the weapon data multiple times
            calculator::Attack attack{ calculator::Weapon::dummy, stats, attack_options };

            this->weapon_table->model->set_rows(*this->active_weapon_data
                | std::views::transform([&](const calculator::Weapon& w) {
                    attack.weapon = w;
                    attack.calculate_inplace();
                    return Row(attack);
                })
                | std::ranges::to<std::vector>());
        }
        
        void calculate_weapon_stats()
        {
            // get character stats
            auto stats = this->get_character_stats();

            // get attack options
            auto attack_options = this->get_attack_options();

            // temporary attack object to avoid copying the weapon data multiple times
            calculator::Attack attack{ calculator::Weapon::dummy, stats, attack_options };

            this->weapon_table->model->update_rows(
                *this->active_weapon_data | std::views::transform([&](const calculator::Weapon& w)->calculator::Attack&& {
                    attack.weapon = w;
                    attack.calculate_inplace();
                    return std::move(attack);
                })
            );
        }
    };

    class OptimizeTab : public StatsTabBase, public Ui::OptimizeWidget
    {
        std::vector<std::reference_wrapper<const calculator::Weapon>> filtered_active_weapon_data{};

        QSpinBox* max_character_level_spinbox{};
        QLabel* max_attribute_points_label{};
        QLabel* free_attribute_points_label{};

        void prepare_optimization()
        {
            this->filtered_active_weapon_data.clear();
            this->filtered_active_weapon_data.reserve(this->active_weapon_data->size());
            this->filtered_active_weapon_data.append_range(*this->active_weapon_data
                | std::views::filter([&](const calculator::Weapon& w) {
                    return (this->base_game_dlc_filter.empty() || this->base_game_dlc_filter.contains(w.dlc))
                        && (this->type_filter.empty() || this->type_filter.contains(std::to_underlying(w.type)))
                        && (this->base_name_filter.empty() || this->base_name_filter.contains(QString::fromStdString(w.base_name)))
                        && (this->affinity_filter.empty() || this->affinity_filter.contains(std::to_underlying(w.affinity)));
                })
            );
            this->weapons_label->setText(QString::number(this->filtered_active_weapon_data.size()));

            auto min_stats = this->get_character_stats();
            auto max_attribute_points = calculator::character_level_to_attribute_points(this->max_character_level_spinbox->value());
            auto free_attribute_points = max_attribute_points - min_stats.attribute_points();
            this->max_attribute_points_label->setText(QString::number(max_attribute_points));
            this->free_attribute_points_label->setText(QString::number(free_attribute_points));


            auto stat_variation_count = optimizer::get_stat_variation_count(
                free_attribute_points,
                min_stats.relevant_stats(),
                make_filled_array<calculator::RelevantAttributeLevels>(calculator::attribute_level_limit)
            );
            this->brute_force_variations_label->setText(QString::number(stat_variation_count));
            this->brute_force_iterations_label->setText(QString::number(stat_variation_count * this->filtered_active_weapon_data.size()));
        }

        void optimize(bool use_v2)
        {
            auto attack_options = this->get_attack_options();
            auto min_stats = this->get_character_stats();
            auto free_attribute_points = calculator::character_level_to_attribute_points(this->max_character_level_spinbox->value())
                - min_stats.attribute_points();

            auto target = static_cast<optimizer::Target>(this->target_combobox->currentIndex());

            auto optimizer_visitor = [&](const auto& optimizer) {
                if(optimizer.iteration_count == 0)
                {
                    QMessageBox::warning(
                        this,
                        "No Valid Stat Variations",
                        "There are no valid stat variations for the given min character attributes and max character level."
                    );
                    return;
                }

                auto future = QtConcurrent::mapped(
                    this->filtered_active_weapon_data,
                    [&](const calculator::Weapon& weapon) { return Row(optimizer(weapon)); }
                );

                std::vector<Row> rows{};
                if (execute_future_with_blocking_progress_bar<true>(future, this, "Optimizing..."))
                {
                    rows.reserve(this->filtered_active_weapon_data.size());
                    rows.append_range(
                        future
                        | std::views::as_rvalue
                    );
                }
                this->weapon_table->model->set_rows(std::move(rows));
            };

            if (use_v2)
            {
                visit_enum(target, [&](auto integral_constant) {
                    if constexpr (optimizer::valid_optimizer_target<integral_constant.value>)
                    {
                        auto v2_optimizer = optimizer::V2<integral_constant.value>{
                            filtered_active_weapon_data,
                            attack_options,
                            free_attribute_points,
                            min_stats,
                            calculator::attribute_level_limit
                        };
                        this->v2_variations_label->setText(
                            QString::number(v2_optimizer.iteration_count / this->filtered_active_weapon_data.size())
                        );
                        this->v2_iterations_label->setText(QString::number(v2_optimizer.iteration_count));

                        optimizer_visitor(v2_optimizer);
                    }
                    else
                        throw std::runtime_error("V2 optimizer not implemented for this target.");
                });
            }
            else
            {
                visit_enum(target, [&](auto integral_constant) {
                    if constexpr (optimizer::valid_optimizer_target<integral_constant.value>)
                    {   
                        auto brute_force_optimizer = optimizer::BruteForce<integral_constant.value>{
                            filtered_active_weapon_data,
                            attack_options,
                            free_attribute_points,
                            min_stats,
                            calculator::attribute_level_limit
                        };

                        optimizer_visitor(brute_force_optimizer);
                    }
                    else
                        throw std::runtime_error("V2 optimizer not implemented for this target.");
                });
            }
        }

    public:
        explicit OptimizeTab(QWidget *parent = nullptr) : StatsTabBase(parent)
        {
            this->weapon_table->placeholder_string = "No items to display, start an optimization first.";

            this->character_stats_box->setTitle("Min Character Attributes");

            auto max_character_stats_box = new QGroupBox("Max Character Attributes");
            this->second_vertical_layout->insertWidget(1, max_character_stats_box);

            auto max_character_stats_layout = new QFormLayout();
            max_character_stats_box->setLayout(max_character_stats_layout);

            // max character level label
            max_character_stats_layout->addRow("Max Character Level:", this->max_character_level_spinbox = new QSpinBox());
            this->max_character_level_spinbox->setMinimum(1);
            calculator::AttributeLevels max_stats{};
            max_stats.fill(99);
            this->max_character_level_spinbox->setMaximum(max_stats.character_level());
            connect(this->max_character_level_spinbox, &QSpinBox::valueChanged, this, &OptimizeTab::prepare_optimization);

            // attribute points label
            max_character_stats_layout->addRow("Max Attribute Points:", this->max_attribute_points_label = new QLabel());
            max_character_stats_layout->addRow("Max Free Attribute Points:", this->free_attribute_points_label = new QLabel());

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
            this->Ui::OptimizeWidget::setupUi(opt_group);

            // optimize target combobox
            [&](auto){
                static constexpr auto [...targets] = enumerators_of<optimizer::Target>();
                ([&]{
                    if constexpr (optimizer::valid_optimizer_target<targets>)
                        this->target_combobox->addItem(enum_to_display(targets));
                }(), ...);
            }(1);
            this->target_combobox->setCurrentIndex(std::to_underlying(optimizer::Target::TOTAL_ATTACK_POWER));
            
            // optimize buttons
            connect(this->start_brute_force_button, &QPushButton::clicked, [this](){ this->optimize(false); });
            connect(this->start_v2_button, &QPushButton::clicked, [this](){ this->optimize(true); });
        }
    
        void set_active_weapon_data(std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data)
        {
            this->StatsTabBase::set_active_weapon_data(std::move(active_weapon_data));
            this->prepare_optimization();
            this->weapon_table->model->set_rows({});
        }
    };

    class MainWindow : public QMainWindow, public Ui::MainWindow
    {
        QActionGroup* menu_weapon_data_group = new QActionGroup(this);
        QMenu *menu_choose_weapon_data;
        StatsTab* stats = new StatsTab();
        OptimizeTab* optimize = new OptimizeTab();
        PlotTab* plot = new PlotTab();

        std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data{};

        void set_active_weapon_data(const std::filesystem::path& dir)
        {
            auto future = QtConcurrent::run([&](){ return parser::load_weapons(dir); });
            execute_future_with_blocking_progress_bar<false>(future, this, "Loading Weapon Data...");
            this->active_weapon_data = std::make_shared<const std::vector<calculator::Weapon>>(future.takeResult());

            this->stats->set_active_weapon_data(this->active_weapon_data);
            this->optimize->set_active_weapon_data(this->active_weapon_data);
            this->plot->set_active_weapon_data(this->active_weapon_data);
        }

        QAction* add_weapon_data(std::filesystem::path dir)
        {
            dir = std::filesystem::canonical(dir).make_preferred();
            if (!std::filesystem::is_directory(dir))
            {
                QMessageBox::critical(this, "Invalid Directory", std::format("Not a Directory: {}", dir).c_str());
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

            auto action = this->menu_choose_weapon_data->addAction(
                QString::fromStdString(action_text),
                [this, dir]() { this->set_active_weapon_data(dir); }
            );
            action->setCheckable(true);
            this->menu_weapon_data_group->addAction(action);
            
            return action;
        }

        void load_weapon_data_from_directory()
        {
            QString directory = QFileDialog::getExistingDirectory(
                this,
                "Select Weapon Data Directory",
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
                "Select eldenring.exe",
                QDir::homePath(),
                "Elden Ring Executable (eldenring.exe);;All Executables (*.exe);;All Files (*)"
            );
            if (file_name.isEmpty())
                return;
            auto elden_ring_executable = std::filesystem::path(file_name.toStdString());

            file_name = QFileDialog::getOpenFileName(
                this,
                "Select WitchyBND.exe",
                QDir::homePath(),
                "WitchyBND executable (WitchyBND.exe);;All Executables (*.exe);;All Files (*)"
            );
            if (file_name.isEmpty())
                return;
            auto witchybdn_executable = std::filesystem::path(file_name.toStdString());

            auto xml_data_directory = (std::filesystem::absolute(QCoreApplication::applicationDirPath().toStdString()) / "xml_data").make_preferred();
            QString directory = QFileDialog::getExistingDirectory(
                this,
                "Select a Save Directory",
                xml_data_directory.string().c_str(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );
            if (directory.isEmpty())
                return;
            auto save_directory = std::filesystem::path(directory.toStdString());
            if (!std::filesystem::is_directory(save_directory))
            {
                QMessageBox::critical(this, "Invalid Directory", std::format("Not a Directory: {}", save_directory).c_str());
                return;
            }

            auto future = QtConcurrent::run([&](){
                return witchy::run_witchy(
                    elden_ring_executable.parent_path(),
                    witchybdn_executable,
                    save_directory
                );
            });
            execute_future_with_blocking_progress_bar<false>(future, this, "Generating Weapon Data...");
            auto expected = future.takeResult();
            if (expected)
                QMessageBox::information(this, "Success", QString::fromStdString(expected.value()));
            else
                QMessageBox::critical(this, "Error", QString::fromStdString(expected.error()));
        }

        void closeEvent(QCloseEvent *event) override
        {
            // save geometry and state
            QSettings settings{};
            settings.beginGroup("MainWindow");
            settings.setValue("geometry", this->saveGeometry());
            settings.setValue("state", this->saveState());
            settings.endGroup();

            QMainWindow::closeEvent(event);
        }

    public:
        explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
        {
            // setup
            this->setupUi(this);

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
                critical_error(this, "No weapon data directories found in xml_data directory.");

            // weapon data menu
            this->menu_choose_weapon_data = this->menu_file->addMenu("Choose Weapon Data");
            this->menu_weapon_data_group->setExclusive(true);
            for (auto&& [i, dir] : weapon_data_directories | std::views::enumerate)
            {
                auto action = this->add_weapon_data(dir);
                if (action == nullptr)
                    std::terminate();
                if (i == 0)
                    QTimer::singleShot(0, action, &QAction::trigger);
            }
            this->menu_file->addAction("Load Weapon Data from Directory", this, &MainWindow::load_weapon_data_from_directory);
            this->menu_file->addAction("Generate Weapon Data from Game Data", this, &MainWindow::generate_weapon_data_from_game_data);
            this->menu_file->addSeparator();
            this->menu_file->addAction("Settings", [](){ settings.show(); });

            this->menu_help->addAction("About elden-ring-damage-optimizer", [](){
                QDesktopServices::openUrl(QUrl("https://github.com/hanslhansl/elden-ring-damage-optimizer"));
            });
            this->menu_help->addAction("About Qt", QApplication::aboutQt);


            // add tabs
            this->tab_widget->addTab(this->stats, "Calculate");
            connect(this->stats->weapon_table, &WeaponTable<StatsTabBase::Row>::add_selection_to_plot, this->plot, &PlotTab::add_datasets);

            this->tab_widget->addTab(this->optimize, "Optimize");
            connect(this->optimize->weapon_table, &WeaponTable<StatsTabBase::Row>::add_selection_to_plot, this->plot, &PlotTab::add_datasets);

            this->tab_widget->addTab(this->plot, "Plot");

            // restore geometry and state
            QSettings settings{};
            settings.beginGroup("MainWindow");
            const auto geometry = settings.value("geometry", QByteArray()).toByteArray();
            if (!geometry.isEmpty())
                this->restoreGeometry(geometry);
            const auto state = settings.value("state", QByteArray()).toByteArray();
            if (state.isEmpty())
                this->setWindowState(Qt::WindowMaximized);
            else
                this->restoreState(state);
            settings.endGroup();
        }
    };
}

int erdo::ui::run_ui(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("hanslhansl");
    app.setApplicationName("elden-ring-damage-optimizer");
    settings.initialize();

    MainWindow window{};
    window.show();

    return app.exec();
}

#include "ui.moc"