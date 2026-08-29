module;
#include "KDChartDataValueAttributes.h"
#include <QStandarditemmodel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPrinter>

#include <KDChartChart>
#include <KDChartWidget>
#include <KDChartCartesianAxis>
#include <KDChartLineDiagram>
#include <KDChartGridAttributes>
#include <KDChartPlotter>
export module erdo.ui.plot_tab;

import std;
import erdo;
import erdo.ui.settings;
import erdo.ui.weapons_table;

namespace erdo::ui
{
    enum class PlotVariable
    {
        STRENGTH,
        DEXTERITY,
        INTELLIGENCE,
        FAITH,
        ARCAINE,

        UPGRADE_LEVEL
    };
}

using namespace erdo;
template<>
constexpr std::array<std::pair<ui::PlotVariable, std::string_view>, 5> enum_string_mapping<ui::PlotVariable> = {
    std::pair{ui::PlotVariable::STRENGTH, "STRENGTH"},
    std::pair{ui::PlotVariable::DEXTERITY, "DEXTERITY"},
    std::pair{ui::PlotVariable::INTELLIGENCE, "INTELLIGENCE"},
    std::pair{ui::PlotVariable::FAITH, "FAITH"},
    std::pair{ui::PlotVariable::ARCAINE, "ARCAINE"},
    // std::pair{ui::PlotVariable::UPGRADE_LEVEL, "UPGRADE_LEVEL"}
};

namespace erdo::ui
{
    export template<PlotVariable variable>
    struct VariableProjection;
    template<PlotVariable variable>
        requires (is_valid_enum_integral<calculator::RelevantAttribute>(std::to_underlying(variable) - std::to_underlying(PlotVariable::STRENGTH)))
    struct VariableProjection<variable>
    {
        static constexpr auto attribute_integral = std::to_underlying(variable) - std::to_underlying(PlotVariable::STRENGTH);
        static constexpr auto attribute = integral_to_enum<calculator::RelevantAttribute>(attribute_integral);

        static unsigned int& operator()(calculator::FullAttackOptions& attack_options)
        {
            return attack_options.stats[attribute_integral + calculator::irrelevant_attribute_count];
        }
    };
    // template<>
    // struct VariableProjection<PlotVariable::UPGRADE_LEVEL>
    // {
    //     static unsigned int& operator()(calculator::FullAttackOptions& attack_options)
    //     {
    //         return attack_options.upgrade_level;
    //     }
    // };
    static constexpr auto variable_projections = [](auto){
        static constexpr auto [...variables] = enumerators_of<PlotVariable>();
        return std::array{ VariableProjection<variables>::operator()... };
    }(1);


    export class PlotTab : public QSplitter
    {
        using Row = decltype([](auto){
            static constexpr auto [...apts] = enumerators_of<calculator::AttackPowerType>();

            return BasicRow<true,
                sections::ColorSection,
                sections::NameSection,
                sections::BaseNameSection,
                sections::AffinitySection,
                sections::TypeSection,
                sections::Stats,
                sections::Requirements,
                sections::CharacterLevelSection,
                sections::BaseGameDLCSection
            >{};
        }(0));

        QStandardItemModel *model;
        KDChart::Chart* chart;
        KDChart::Plotter* plotter;
        KDChart::CartesianAxis *x_axis;
        KDChart::CartesianAxis *y_axis;

        QComboBox* variable_combobox;
        QComboBox* metric_combobox;
        WeaponTable<Row>* weapon_table;

        static QColor get_distinctive_color()
        {
            static const QVector<QColor> colors = {
                QColor("#0072B2"),
                QColor("#E69F00"),
                QColor("#009E73"),
                QColor("#D55E00"),
                QColor("#CC79A7"),
                QColor("#56B4E9"),
                QColor("#F0E442"),
                QColor("#000000"),
            };
            static auto index = 0;

            return colors[index++ % colors.size()];
        }

        static const std::vector<unsigned int>& get_dataset_x_values(PlotVariable variable, const calculator::Weapon& weapon)
        {
            if (is_valid_enum_integral<calculator::RelevantAttribute>(std::to_underlying(variable) - std::to_underlying(PlotVariable::STRENGTH)))
            {
                return get_universal_x_values(variable);
            }
            else if (variable == PlotVariable::UPGRADE_LEVEL)
            {
                static const auto res = std::views::iota(0u, calculator::max_upgrade_levels.at(weapon.upgrade_level_index) + 1)
                    | std::ranges::to<std::vector>();
                return res;
            }
            throw std::runtime_error(std::format("Invalid variable for dataset x values: {}", std::to_underlying(variable)));
        }
        static const std::vector<unsigned int>& get_universal_x_values(PlotVariable variable)
        {
            if (is_valid_enum_integral<calculator::RelevantAttribute>(std::to_underlying(variable) - std::to_underlying(PlotVariable::STRENGTH)))
            {
                static const auto res = std::views::iota(0u, settings.attribute_level_limit.value + 1u)
                    | std::ranges::to<std::vector>();
                    return res;
            }
            else if (variable == PlotVariable::UPGRADE_LEVEL)
            {
                static const auto res = std::views::iota(0u, std::ranges::max(calculator::max_upgrade_levels) + 1)
                    | std::ranges::to<std::vector>();
                return res;
            }
            throw std::runtime_error(std::format("Invalid variable for dataset x values: {}", std::to_underlying(variable)));
        }

        // static_assert(false, "next: implement upgrade level plotting, highlight dataset origin in plot");

        int weapon_index_to_dataset(int i)
        {
            return i + 1;
        }

        void update_datasets(int index, int count)
        {
            if (index < 0 || index + count > this->weapon_table->model->rows.size())
                throw std::runtime_error(std::format("Invalid range for update_datasets: index: {}, count: {}, rows: {}", index, count, this->weapon_table->model->rows.size()));

            auto variable_index = this->variable_combobox->currentIndex();
            auto variable = static_cast<PlotVariable>(variable_index);
            auto variable_projection = variable_projections.at(variable_index);

            auto&& universal_x = get_universal_x_values(variable);
            for (auto&& x : universal_x)
                this->model->setData(this->model->index(x, 0), x);

            auto metric_index = this->metric_combobox->currentIndex();
            auto metric = static_cast<optimizer::Target>(metric_index);
            auto metric_projection = optimizer::projections.at(metric_index);
            for (auto&& [wi, row] : this->weapon_table->model->rows | std::views::enumerate | std::views::drop(index) | std::views::take(count))
            {
                auto i = this->weapon_index_to_dataset(wi);
                const auto column = i * 2;

                auto&& attack_options = row.attack;
                auto original_x = variable_projection(attack_options);
                auto&& weapon = attack_options.weapon.get();
                calculator::Attack attack{ weapon, attack_options.stats, attack_options };

                auto&& xs = get_dataset_x_values(variable, weapon);
                auto attributes_model = this->plotter->attributesModel();
                bool already_hit_original_x = false;
                for(auto j = 0; j < universal_x.size(); ++j)
                {
                    auto x_index = this->model->index(j, column);

                    if (j < xs.size())
                    {
                        auto x_j = xs[j];
                        variable_projection(attack) = x_j;
                        attack.calculate_inplace();
                        auto y_ij = metric_projection(attack);

                        this->model->setData(x_index, x_j);
                        this->model->setData(this->model->index(j, column + 1), y_ij);

                        if (x_j == original_x)
                        {
                            already_hit_original_x = true;
                            auto dva = this->plotter->dataValueAttributes(x_index);
                            dva.setVisible(true);
                            this->plotter->setDataValueAttributes(x_index, dva);
                        }
                        else
                        {
                            attributes_model->resetData(
                                x_index,
                                KDChart::DisplayRoles::DataValueLabelAttributesRole
                            );
                        }
                    }
                    else
                    {
                        this->model->setData(x_index, QVariant());
                        this->model->setData(this->model->index(j, column + 1), QVariant());

                        attributes_model->resetData(
                            x_index,
                            KDChart::DisplayRoles::DataValueLabelAttributesRole
                        );
                    }
                }
            }
        }

        void update_all_datasets()
        {
            this->update_datasets(0, this->weapon_table->model->rows.size());
        }

        void change_variable(int variable_index)
        {
            auto variable = static_cast<PlotVariable>(variable_index);
            this->x_axis->setTitleText(enum_to_display(variable));
            auto new_dataset_length = get_universal_x_values(variable).size();
            auto old_dataset_length = this->model->rowCount();
            if (new_dataset_length > old_dataset_length)
                this->model->insertRows(old_dataset_length, new_dataset_length - old_dataset_length);
            else if (new_dataset_length < old_dataset_length)
                this->model->removeRows(new_dataset_length, old_dataset_length - new_dataset_length);

            this->update_all_datasets();
        }
        void change_metric(int metric_index)
        {
            auto metric = static_cast<optimizer::Target>(metric_index);
            this->y_axis->setTitleText(enum_to_display(metric));
            this->update_all_datasets();
        }

        void remove_datasets(std::vector<int> indices)
        {
            this->weapon_table->model->remove_rows(indices);
            std::ranges::sort(indices, std::greater{});
            for (auto&& wi : indices)
            {
                auto i = this->weapon_index_to_dataset(wi);
                const auto column = i * 2;
                this->model->removeColumns(column, 2);
            }
        }

    public:
        void add_datasets(const std::vector<std::reference_wrapper<const calculator::FullAttackOptions>>& attacks_options)
        {
            auto current_dataset_count = this->weapon_table->model->rows.size();

            this->weapon_table->model->add_rows(attacks_options
                | std::views::transform([](const calculator::FullAttackOptions& attacks_option) {
                    auto row = Row(calculator::FullAttackOptions(attacks_option));
                    std::get<sections::ColorSection>(row)[0] = get_distinctive_color();
                    return row;
                })
            );

            auto current_column_count = this->model->columnCount();
            this->model->insertColumns(current_column_count, attacks_options.size() * 2);

            for (auto&& [wi, row] : this->weapon_table->model->rows
                | std::views::enumerate
                | std::views::drop(current_dataset_count)
                | std::views::take(attacks_options.size())
            )
            {
                auto i = this->weapon_index_to_dataset(wi);
                const auto column = i * 2;
                auto dataset_color = std::get<sections::ColorSection>(row)[0].value<QColor>();

                this->model->setHeaderData(column, Qt::Horizontal, std::get<sections::NameSection>(row)[0][0].toString());
                this->model->setHeaderData(column + 1, Qt::Horizontal, std::get<sections::NameSection>(row)[0][0].toString());
                auto pen = this->plotter->pen(i);
                pen.setCosmetic(true);
                pen.setColor(dataset_color);
                pen.setWidth(settings.plot_data_line_width);
                this->plotter->setPen(i, pen);

                auto dva = this->plotter->dataValueAttributes(i);
                auto marker = dva.markerAttributes();
                // marker.setVisible(true);
                // marker.setMarkerStyle(KDChart::MarkerAttributes::MarkerCircle);
                // marker.setMarkerSize(QSizeF(settings.plot_point_diameter, settings.plot_point_diameter));
                marker.setMarkerColor(dataset_color);
                dva.setMarkerAttributes(marker);

                // auto text = dva.textAttributes();
                // text.setVisible(false);
                // dva.setTextAttributes(text);

                this->plotter->setDataValueAttributes(i, dva);
            }

            this->update_datasets(current_dataset_count, attacks_options.size());
        }

        explicit PlotTab(QWidget *parent = nullptr) : QSplitter(Qt::Orientation::Vertical, parent)
        {
            // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
            QPrinter printer;
            Q_UNUSED(printer);

            // weapon table (model)
            this->weapon_table = new WeaponTable<Row>();
            connect(this->weapon_table, &WeaponTable<Row>::remove_from_plot, this, &PlotTab::remove_datasets);
            connect(this->weapon_table, &WeaponTable<Row>::row_color_changed, [this](int wi, QColor color){
                auto i = this->weapon_index_to_dataset(wi);
                auto pen = this->plotter->pen(i);
                pen.setColor(color);
                this->plotter->setPen(i, pen);
            });

            // plotting backend
            this->model = new QStandardItemModel(this);
            // this->model->setRowCount(1);
            this->model->setColumnCount(2);
            this->plotter = new KDChart::Plotter();
            this->plotter->setModel(this->model);
            this->x_axis = new KDChart::CartesianAxis(plotter);
            this->x_axis->setPosition(KDChart::CartesianAxis::Bottom);
            plotter->addAxis(this->x_axis);
            this->y_axis = new KDChart::CartesianAxis(plotter);
            this->y_axis->setPosition(KDChart::CartesianAxis::Left);
            plotter->addAxis(this->y_axis);

            // variable
            this->variable_combobox = new QComboBox(this);
            for (const auto& variable : enumerators_of<PlotVariable>())
                this->variable_combobox->addItem(enum_to_display(variable));
            connect(this->variable_combobox, &QComboBox::currentIndexChanged, this, &PlotTab::change_variable);

            // metric
            this->metric_combobox = new QComboBox(this);
            for (const auto& target : enumerators_of<optimizer::Target>())
                this->metric_combobox->addItem(enum_to_display(target));
            connect(this->metric_combobox, &QComboBox::currentIndexChanged, this, &PlotTab::change_metric);

            this->change_variable(this->variable_combobox->currentIndex());
            this->change_metric(this->metric_combobox->currentIndex());

            // Initialize the model with dummy data (KDChart::Plotter is buggy...)
            this->model->setData(this->model->index(0, 0), 0.);
            this->model->setData(this->model->index(0, 0 + 1), 0.);
            this->model->setData(this->model->index(1, 0), 1.);
            this->model->setData(this->model->index(1, 0 + 1), 1.);
            this->plotter->setPen(0, Qt::NoPen);

            // layout
            auto upper_widget = new QWidget(this);
            auto upper_layout = new QVBoxLayout(upper_widget);

            auto upper_horizontal_layout = new QHBoxLayout();
            upper_horizontal_layout->addStretch(1);
            upper_layout->addLayout(upper_horizontal_layout);
            
            // variable combobox
            auto x_axis_layout = new QFormLayout();
            upper_horizontal_layout->addLayout(x_axis_layout);
            x_axis_layout->addRow("Variable:", this->variable_combobox);

            // metric combobox
            auto y_axis_layout = new QFormLayout();
            upper_horizontal_layout->addLayout(y_axis_layout);
            y_axis_layout->addRow("Metric:", this->metric_combobox);
            
            upper_horizontal_layout->addStretch(1);

            // chart widget
            this->chart = new KDChart::Chart(this);
            upper_layout->addWidget(this->chart);
            this->chart->coordinatePlane()->replaceDiagram(this->plotter);
            this->chart->coordinatePlane()->globalGridAttributes().setSubGridVisible(false);

            auto set_data_line_width = [this](){
                for (auto&& [wi, row] : this->weapon_table->model->rows | std::views::enumerate)
                {
                    auto i = this->weapon_index_to_dataset(wi);

                    auto pen = this->plotter->pen(i);
                    pen.setWidth(settings.plot_data_line_width);
                    this->plotter->setPen(i, pen);
                }
            };
            set_data_line_width();
            connect(&settings.plot_data_line_width, settings.plot_data_line_width.changed_member_pointer, set_data_line_width);

            auto dva = this->plotter->dataValueAttributes();
            auto marker = dva.markerAttributes();
            marker.setVisible(true);
            marker.setMarkerStyle(KDChart::MarkerAttributes::MarkerCircle);
            marker.setMarkerSize(QSizeF(settings.plot_point_diameter, settings.plot_point_diameter));
            dva.setMarkerAttributes(marker);
            auto text = dva.textAttributes();
            text.setVisible(false);
            dva.setTextAttributes(text);
            this->plotter->setDataValueAttributes(dva);

            auto set_point_diameter = [this](){
                for (auto&& [wi, row] : this->weapon_table->model->rows | std::views::enumerate)
                {
                    auto i = this->weapon_index_to_dataset(wi);

                    auto dva = this->plotter->dataValueAttributes(i);
                    auto marker = dva.markerAttributes();
                    marker.setMarkerSize(QSizeF(settings.plot_point_diameter, settings.plot_point_diameter));
                    dva.setMarkerAttributes(marker);
                    this->plotter->setDataValueAttributes(dva);
                }
            };
            set_point_diameter();
            connect(&settings.plot_point_diameter, settings.plot_point_diameter.changed_member_pointer, set_point_diameter);

            auto set_grid_line_width = [this](){
                auto plane = this->chart->coordinatePlane();
                auto grid = plane->globalGridAttributes();
                grid.setSubGridVisible(false);
                auto pen = grid.gridPen();
                pen.setCosmetic(true);
                pen.setWidth(settings.plot_grid_line_width);
                grid.setGridPen(pen);
                plane->setGlobalGridAttributes(grid);
            };
            set_grid_line_width();
            connect(&settings.plot_grid_line_width, settings.plot_grid_line_width.changed_member_pointer, set_grid_line_width);

            auto set_axis_line_width = [this](){
                auto plane = this->chart->coordinatePlane();
                auto grid = plane->globalGridAttributes();
                auto pen = grid.zeroLinePen();
                pen.setCosmetic(true);
                pen.setWidth(settings.plot_axis_line_width);
                pen.setColor(Qt::black);
                grid.setZeroLinePen(pen);
                plane->setGlobalGridAttributes(grid);
            };
            set_axis_line_width();
            connect(&settings.plot_axis_line_width, settings.plot_axis_line_width.changed_member_pointer, set_axis_line_width);

            this->addWidget(this->weapon_table);
        }
    };
}

module : private;
