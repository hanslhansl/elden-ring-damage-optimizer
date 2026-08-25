module;
#include "KDChartLegend.h"
#include "KDChartPlotter.h"
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
#include <qstandarditemmodel.h>
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

        QTimer *update_plot_timer = new QTimer(this);
        void update_plot_impl();

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

    public:
        explicit PlotTab(QWidget *parent = nullptr);

        void update_plot()
        {
            this->update_plot_timer->start(settings.calculation_delay);
        }

        void add_datasets(const std::vector<std::reference_wrapper<const calculator::FullAttackOptions>>& attacks_options)
        {
            this->weapon_table->model->add_rows(attacks_options
                | std::views::transform([](const calculator::FullAttackOptions& attacks_option) {
                    auto row = Row(calculator::FullAttackOptions(attacks_option));
                    std::get<sections::ColorSection>(row)[0] = get_distinctive_color();
                    return row;
                })
            );

            this->update_plot_impl();
        }

        void remove_datasets(const std::vector<int>& indices)
        {
            this->weapon_table->model->remove_rows(indices);
            for (auto&& i : indices)
                this->model->removeColumns(i * 2, 2);
        }
    };
}

module : private;


void ui::PlotTab::update_plot_impl()
{
    this->update_plot_timer->stop();

    this->model->clear();

    auto variable_index = this->variable_combobox->currentIndex();
    auto variable = static_cast<PlotVariable>(variable_index);
    auto variable_projection = variable_projections.at(variable_index);
    this->x_axis->setTitleText(enum_to_display(variable));
    auto x = visit_enum(
        variable,
        [&](auto integral_constant) {
            if constexpr (is_valid_enum_integral<calculator::RelevantAttribute>(std::to_underlying(integral_constant.value)))
            {
                return std::views::iota(0u, (unsigned int)settings.attribute_level_limit.value)
                    | std::ranges::to<std::vector>();
            }
        }
    );
    this->model->setRowCount(x.size());
    this->model->setColumnCount(this->weapon_table->model->rows.size() * 2);

    auto metric_index = this->metric_combobox->currentIndex();
    auto metric = static_cast<optimizer::Target>(metric_index);
    auto metric_projection = optimizer::projections.at(metric_index);
    this->y_axis->setTitleText(enum_to_display(metric));
    for (auto&& [i, row] : this->weapon_table->model->rows | std::views::enumerate)
    {
        const auto column = i * 2;

        auto&& attack_options = row.attack;
        auto&& weapon = attack_options.weapon.get();
        calculator::Attack attack{ weapon, attack_options.stats, attack_options };

        this->model->setHeaderData(column, Qt::Horizontal, std::get<sections::NameSection>(row)[0][0].toString());
        this->plotter->setPen(i, std::get<sections::ColorSection>(this->weapon_table->model->rows[i])[0].value<QColor>());

        for (auto [j, x_j] : x | std::views::enumerate)
        {
            variable_projection(attack) = x_j;
            attack.calculate_inplace();
            auto y_ij = metric_projection(attack);
            this->model->setData(model->index(j, column), x_j);
            this->model->setData(model->index(j, column + 1), y_ij);
        }
    }
}

ui::PlotTab::PlotTab(QWidget *parent) : QSplitter(Qt::Orientation::Vertical, parent)
{
    // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
    QPrinter printer;
    Q_UNUSED(printer);

    this->update_plot_timer->setSingleShot(true);
    connect(this->update_plot_timer, &QTimer::timeout, this, &PlotTab::update_plot_impl);

    auto upper_widget = new QWidget(this);
    auto upper_layout = new QVBoxLayout(upper_widget);

    auto upper_horizontal_layout = new QHBoxLayout();
    upper_horizontal_layout->addStretch(1);
    upper_layout->addLayout(upper_horizontal_layout);

    auto x_axis_layout = new QFormLayout();
    upper_horizontal_layout->addLayout(x_axis_layout);
    x_axis_layout->addRow("Variable:", this->variable_combobox = new QComboBox(this));
    connect(this->variable_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);
    for (const auto& variable : enumerators_of<PlotVariable>())
        this->variable_combobox->addItem(enum_to_display(variable));

    auto y_axis_layout = new QFormLayout();
    upper_horizontal_layout->addLayout(y_axis_layout);
    y_axis_layout->addRow("Metric:", this->metric_combobox = new QComboBox(this));
    connect(this->metric_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);
    for (const auto& target : enumerators_of<optimizer::Target>())
        this->metric_combobox->addItem(enum_to_display(target));

    upper_horizontal_layout->addStretch(1);

    this->model = new QStandardItemModel(this);
    this->chart = new KDChart::Chart(this);
    upper_layout->addWidget(this->chart);

    this->plotter = new KDChart::Plotter;
    KDChart::LineAttributes attr;
    attr.setDisplayArea(false);
    this->plotter->setLineAttributes(0, attr);
    this->plotter->setModel(model);
    this->chart->coordinatePlane()->replaceDiagram(this->plotter);

    this->x_axis = new KDChart::CartesianAxis(plotter);
    this->y_axis = new KDChart::CartesianAxis(plotter);
    this->x_axis->setPosition(KDChart::CartesianAxis::Bottom);
    this->y_axis->setPosition(KDChart::CartesianAxis::Left);
    plotter->addAxis(this->x_axis);
    plotter->addAxis(this->y_axis);

    this->weapon_table = new WeaponTable<Row>(this);
    connect(this->weapon_table, &WeaponTable<Row>::remove_from_plot, this, &PlotTab::remove_datasets);
    connect(this->weapon_table, &WeaponTable<Row>::row_color_changed, [this](int index, QColor color){
        this->plotter->setPen(index, color);
    });
};