module;
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

        KDChart::Widget* chart;
        QStandardItemModel *model;
        QComboBox* variable_combobox;
        QSpinBox* min_spinbox;
        QSpinBox* max_spinbox;
        QSpinBox* step_spinbox;
        QComboBox* metric_combobox;
        WeaponTable<Row>* weapon_table;

        QTimer *update_plot_timer = new QTimer(this);
        void update_plot_impl()
        {
            this->update_plot_timer->stop();

            // this->chart->resetData();
            this->model->removeRows(0, this->model->rowCount());

            auto variable_index = this->variable_combobox->currentIndex();
            auto variable = static_cast<PlotVariable>(variable_index);
            auto variable_projection = variable_projections.at(variable_index);
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
            this->model->setColumnCount(this->weapon_table->model->rows.size());

            auto metric_projection = optimizer::projections.at(this->metric_combobox->currentIndex());
            for (auto&& [i, row] : this->weapon_table->model->rows | std::views::enumerate)
            {
                auto&& attack_options = row.attack;
                auto&& weapon = attack_options.weapon.get();
                calculator::Attack attack{ weapon, attack_options.stats, attack_options };

                this->model->setHeaderData(i, Qt::Horizontal, std::get<sections::NameSection>(row)[0][0].toString());

                for (auto [j, x_j] : x | std::views::enumerate)
                {
                    variable_projection(attack) = x_j;
                    attack.calculate_inplace();
                    auto y_ij = metric_projection(attack)/*x*i*//*QPair{ x*2, x*2 }*/;
                    this->model->setData(model->index(j, i), y_ij);
                }

                // auto xy = x
                //     | std::views::transform([&](unsigned int x){
                //         variable_projection(attack) = x;
                //         attack.calculate_inplace();
                //         return metric_projection(attack)/*x*i*//*QPair{ x*2, x*2 }*/;
                //     })
                //     | std::ranges::to<QVector<qreal/*QPair<qreal, qreal>*/>>();
                // this->chart->setDataset(i, xy, std::get<sections::NameSection>(row)[0][0].toString());
            }
        }

    public:
        explicit PlotTab(QWidget *parent = nullptr) : QSplitter(Qt::Orientation::Vertical, parent)
        {
            // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
            QPrinter printer;
            Q_UNUSED(printer);

            this->update_plot_timer->setSingleShot(true);
            connect(this->update_plot_timer, &QTimer::timeout, this, &PlotTab::update_plot_impl);

            auto upper_widget = new QWidget(this);
            auto upper_layout = new QHBoxLayout(upper_widget);

            this->model = new QStandardItemModel(this);
            this->chart = new KDChart::Widget(this);
            KDChart::CartesianAxis *xAxis = new KDChart::CartesianAxis( this->chart->lineDiagram() );
            KDChart::CartesianAxis *yAxis = new KDChart::CartesianAxis(this->chart->lineDiagram() );
            xAxis->setPosition( KDChart::CartesianAxis::Bottom );
            yAxis->setPosition( KDChart::CartesianAxis::Left );
            xAxis->setTitleText( "x" );
            yAxis->setTitleText( "y" );
            this->chart->lineDiagram()->addAxis( xAxis );
            this->chart->lineDiagram()->addAxis( yAxis );
            upper_layout->addWidget(this->chart);

            auto upper_right_layout = new QVBoxLayout();
            upper_layout->addLayout(upper_right_layout);

            auto x_axis_box = new QGroupBox("X-Axis");
            upper_right_layout->addWidget(x_axis_box);
            auto x_axis_layout = new QFormLayout(x_axis_box);

            x_axis_layout->addRow("Variable:", this->variable_combobox = new QComboBox(this));
            connect(this->variable_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);
            for (const auto& variable : enumerators_of<PlotVariable>())
                this->variable_combobox->addItem(enum_to_display(variable));

            x_axis_layout->addRow("Min:", this->min_spinbox = new QSpinBox(this));
            connect(this->min_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);
            x_axis_layout->addRow("Max:", this->max_spinbox = new QSpinBox(this));
            connect(this->max_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);
            x_axis_layout->addRow("Step:", this->step_spinbox = new QSpinBox(this));
            connect(this->step_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);

            auto y_axis_box = new QGroupBox("Y-Axis");
            upper_right_layout->addWidget(y_axis_box);
            auto y_axis_layout = new QFormLayout(y_axis_box);

            y_axis_layout->addRow("Metric:", this->metric_combobox = new QComboBox(this));
            connect(this->metric_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);
            for (const auto& target : enumerators_of<optimizer::Target>())
                this->metric_combobox->addItem(enum_to_display(target));

            this->weapon_table = new WeaponTable<Row>(this);
            connect(this->weapon_table, &WeaponTable<Row>::remove_from_plot, this, &PlotTab::remove_datasets);
        };

        void update_plot()
        {
            this->update_plot_timer->start(settings.calculation_delay);
        }

        void add_datasets(const std::vector<std::reference_wrapper<const calculator::FullAttackOptions>>& attacks_options)
        {
            this->weapon_table->model->add_rows(attacks_options
                | std::views::transform([](const calculator::FullAttackOptions& attacks_option) {
                    return Row(calculator::FullAttackOptions(attacks_option));
                })
            );

            this->update_plot_impl();
        }

        void remove_datasets(const std::vector<int>& attacks)
        {
            // this->weapon_table->model->remove_rows(attacks);
            for (auto&& attack_index : attacks)
                this->chart->diagram()->setHidden(attack_index, true);
            // this->update_plot_impl();
        }
    };
}