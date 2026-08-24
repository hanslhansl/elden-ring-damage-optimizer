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

        // UPGRADE_LEVEL
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
    // std::pair{ui::PlotVariable::UPGRADE_LEVEL, "UPGRADE_LEVEL"}
};

namespace erdo::ui
{
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

            this->chart->resetData();

            auto x = visit_enum(
                static_cast<PlotVariable>(this->variable_combobox->currentIndex()),
                [&](auto integral_constant) {
                    if constexpr (is_valid_enum_integral<PlotVariable>(std::to_underlying(integral_constant.value)))
                    {
                        return std::views::iota(0, settings.attribute_level_limit.value)
                            | std::ranges::to<QVector<qreal>>();
                    }
                }
            );

            for (auto&& [i, row] : this->weapon_table->model->rows | std::views::enumerate)
            {
                auto&& weapon = row.attack.weapon.get();

                auto xy = x
                    | std::views::transform([&](qreal x){ return x*i/*QPair{ x*2, x*2 }*/; })
                    | std::ranges::to<QVector<qreal/*QPair<qreal, qreal>*/>>();
                this->chart->setDataset(i, xy, std::get<sections::NameSection>(row)[0][0].toString());
                std::println("plot {}", weapon.full_name);
            }

            this->chart->setDataset(this->weapon_table->model->rows.size(), x, "x-axis");

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
            this->weapon_table->model->remove_rows(attacks);
            this->update_plot_impl();
        }
    };
}