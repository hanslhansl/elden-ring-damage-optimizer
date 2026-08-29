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
constexpr std::array<std::pair<ui::PlotVariable, std::string_view>, 6> enum_string_mapping<ui::PlotVariable> = {
    std::pair{ui::PlotVariable::STRENGTH, "STRENGTH"},
    std::pair{ui::PlotVariable::DEXTERITY, "DEXTERITY"},
    std::pair{ui::PlotVariable::INTELLIGENCE, "INTELLIGENCE"},
    std::pair{ui::PlotVariable::FAITH, "FAITH"},
    std::pair{ui::PlotVariable::ARCAINE, "ARCAINE"},
    std::pair{ui::PlotVariable::UPGRADE_LEVEL, "UPGRADE_LEVEL"}
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
    template<>
    struct VariableProjection<PlotVariable::UPGRADE_LEVEL>
    {
        static unsigned int& operator()(calculator::FullAttackOptions& attack_options)
        {
            return attack_options.upgrade_levels.at(attack_options.weapon.get().upgrade_level_index);
        }
    };
    static constexpr auto variable_projections = [](auto){
        static constexpr auto [...variables] = enumerators_of<PlotVariable>();
        return std::array{ VariableProjection<variables>::operator()... };
    }(1);

    class SplitterHandle : public QSplitterHandle
    {
    public:
        explicit SplitterHandle(Qt::Orientation orientation, QSplitter *parent) : QSplitterHandle(orientation, parent)
        {
            setCursor(Qt::SplitVCursor);
        }

    protected:
        void enterEvent(QEnterEvent *) override
        {
            update();
        }

        void leaveEvent(QEvent *) override
        {
            update();
        }

        void paintEvent(QPaintEvent *) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            // Entire handle = white
            painter.fillRect(this->rect(), Qt::white);

            // Actual visual handle
            const int visualHeight = this->height()-8;
            const int y = (this->height() - visualHeight) / 2;

            const bool hovered = this->underMouse();
            painter.fillRect(
                0, y, this->width(), visualHeight,
                hovered ? QColor("#b0b0b0") : QColor("#d6d6d6")
            );

            // Grip dots
            const int dotSize = 3;
            const int spacing = 5;

            const int totalWidth = 3 * dotSize + 2 * spacing;
            const int startX = (this->width() - totalWidth) / 2;
            const int dotY = y + (visualHeight - dotSize) / 2;

            painter.setBrush(QColor("#777777"));
            painter.setPen(Qt::NoPen);

            for (int i = 0; i < 3; ++i)
                painter.drawEllipse(startX + i * (dotSize + spacing), dotY, dotSize, dotSize);
        }
    };

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
                static const auto res = calculator::max_upgrade_levels
                    | std::views::transform([](unsigned int max_upgrade_level){
                        return std::views::iota(0u, max_upgrade_level + 1)
                            | std::ranges::to<std::vector>();
                    })
                    | std::ranges::to<std::vector>();

                return res.at(weapon.upgrade_level_index);
            }
            throw std::runtime_error(std::format("Invalid variable for dataset x values: {}", std::to_underlying(variable)));
        }
        static const std::vector<unsigned int>& get_universal_x_values(PlotVariable variable)
        {
            if (is_valid_enum_integral<calculator::RelevantAttribute>(std::to_underlying(variable) - std::to_underlying(PlotVariable::STRENGTH)))
            {
                static const auto res = std::views::iota(0u, calculator::attribute_level_limit + 1u)
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

        int weapon_index_to_dataset(int i)
        {
            return i + 1;
        }

        void set_dataset_attributes(int wi)
        {
            auto i = this->weapon_index_to_dataset(wi);
            const auto column = i * 2;
            auto dataset_color = std::get<sections::ColorSection>(this->weapon_table->model->rows.at(wi))[0].value<QColor>();
            
            auto pen = this->plotter->pen(i);
            pen.setColor(dataset_color);
            this->plotter->setPen(i, pen);

            auto index = this->model->index(0, column);
            auto dva = this->plotter->dataValueAttributes(index);
            auto marker = dva.markerAttributes();
            marker.setMarkerColor(dataset_color);
            dva.setMarkerAttributes(marker);
            dva.setVisible(true);
            this->plotter->setDataValueAttributes(index, dva);
        }

        void update_datasets(int index, int count)
        {
            if (index < 0 || index + count > this->weapon_table->model->rows.size())
                throw std::runtime_error(std::format("Invalid range for update_datasets: index: {}, count: {}, rows: {}", index, count, this->weapon_table->model->rows.size()));

            auto variable_index = this->variable_combobox->currentIndex();
            auto variable = static_cast<PlotVariable>(variable_index);
            auto variable_projection = variable_projections.at(variable_index);

            auto&& universal_xs = get_universal_x_values(variable);
            for(auto j = 0; j < universal_xs.size(); ++j)
                this->model->setData(this->model->index(j, 0), universal_xs[j]);

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
                bool already_hit_original_x = false;
                for(auto j = 0; j < xs.size(); ++j)
                {
                    auto x_j = xs[j];
                    variable_projection(attack) = x_j;
                    attack.calculate_inplace();
                    auto y_ij = metric_projection(attack);

                    auto new_j = j;
                    if (x_j == original_x)
                    {
                        already_hit_original_x = true;
                        new_j = 0;
                    }
                    else
                        new_j += !already_hit_original_x;

                    this->model->setData(this->model->index(new_j, column), x_j);
                    this->model->setData(this->model->index(new_j, column + 1), y_ij);
                }
                for(auto j = xs.size(); j < universal_xs.size(); ++j)
                {
                    this->model->setData(this->model->index(j, column), QVariant());
                    this->model->setData(this->model->index(j, column + 1), QVariant());
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

            for (auto wi = indices.back(); wi < this->weapon_table->model->rows.size(); ++wi)
                this->set_dataset_attributes(wi);
        }

        QSplitterHandle *createHandle() override
        {
            return new SplitterHandle(Qt::Vertical, this);
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
                this->set_dataset_attributes(wi);

            this->update_datasets(current_dataset_count, attacks_options.size());
        }

        explicit PlotTab(QWidget *parent = nullptr) : QSplitter(Qt::Orientation::Vertical, parent)
        {
            // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
            QPrinter printer;
            Q_UNUSED(printer);

            // weapon table (model)
            this->weapon_table = new WeaponTable<Row>(true, this);
            connect(this->weapon_table, &WeaponTable<Row>::remove_selection_from_plot, this, &PlotTab::remove_datasets);
            connect(this->weapon_table, &WeaponTable<Row>::row_color_changed, [this](int wi, QColor color){
                auto i = this->weapon_index_to_dataset(wi);
                auto pen = this->plotter->pen(i);
                pen.setColor(color);
                this->plotter->setPen(i, pen);
            });
            connect(this->weapon_table, &WeaponTable<Row>::edit_row, [this](int row_index){
                throw std::runtime_error("Not implemented: add_new_to_plot");
            });
            connect(this->weapon_table, &WeaponTable<Row>::add_new_to_plot, [this](){
                throw std::runtime_error("Not implemented: add_new_to_plot");
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

            auto pen = this->plotter->pen();
            pen.setCosmetic(true);
            pen.setWidth(settings.plot_data_line_width);
            this->plotter->setPen(pen);

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
            this->setHandleWidth(12);
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
                    auto column = i * 2;
                    auto index = this->model->index(0, column);

                    auto dva = this->plotter->dataValueAttributes(index);
                    auto marker = dva.markerAttributes();
                    marker.setMarkerSize(QSizeF(settings.plot_point_diameter, settings.plot_point_diameter));
                    dva.setMarkerAttributes(marker);
                    this->plotter->setDataValueAttributes(index, dva);
                    this->plotter->update(); // dont know why but the new size only applies after a repaint
                }
            };
            connect(&settings.plot_point_diameter, settings.plot_point_diameter.changed_member_pointer, set_point_diameter);

            auto plane = this->chart->coordinatePlane();
            auto grid = plane->globalGridAttributes();
            grid.setSubGridVisible(false);
            pen = grid.gridPen();
            pen.setCosmetic(true);
            pen.setWidth(settings.plot_grid_line_width);
            grid.setGridPen(pen);
            plane->setGlobalGridAttributes(grid);
            auto set_grid_line_width = [this](){
                auto plane = this->chart->coordinatePlane();
                auto grid = plane->globalGridAttributes();
                auto pen = grid.gridPen();
                pen.setWidth(settings.plot_grid_line_width);
                grid.setGridPen(pen);
                plane->setGlobalGridAttributes(grid);
            };
            connect(&settings.plot_grid_line_width, settings.plot_grid_line_width.changed_member_pointer, set_grid_line_width);

            pen = grid.zeroLinePen();
            pen.setCosmetic(true);
            pen.setWidth(settings.plot_axis_line_width);
            pen.setColor(Qt::black);
            grid.setZeroLinePen(pen);
            plane->setGlobalGridAttributes(grid);
            auto set_axis_line_width = [this](){
                auto plane = this->chart->coordinatePlane();
                auto grid = plane->globalGridAttributes();
                auto pen = grid.zeroLinePen();
                pen.setWidth(settings.plot_axis_line_width);
                grid.setZeroLinePen(pen);
                plane->setGlobalGridAttributes(grid);
            };
            connect(&settings.plot_axis_line_width, settings.plot_axis_line_width.changed_member_pointer, set_axis_line_width);

            this->addWidget(this->weapon_table);
        }
    };
}

module : private;
