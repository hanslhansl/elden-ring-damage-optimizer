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
export module erdo.ui.plot_tab;

import std;
import erdo;
import erdo.ui.settings;
import erdo.ui.weapons_table;

namespace erdo::ui
{
    export class PlotTab : public QSplitter
    {
        QComboBox* variable_combobox;
        QSpinBox* min_spinbox;
        QSpinBox* max_spinbox;
        QSpinBox* step_spinbox;
        QComboBox* metric_combobox;
        WeaponTable* weapon_table;

        QTimer *update_plot_timer = new QTimer(this);
        void update_plot_impl()
        {
            this->update_plot_timer->stop();
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

            auto *chart = new KDChart::Chart();
            upper_layout->addWidget(chart);

            auto upper_right_layout = new QVBoxLayout();
            upper_layout->addLayout(upper_right_layout);

            auto x_axis_box = new QGroupBox("X-Axis");
            upper_right_layout->addWidget(x_axis_box);
            auto x_axis_layout = new QFormLayout(x_axis_box);
            x_axis_layout->addRow("Variable:", this->variable_combobox = new QComboBox());
            connect(this->variable_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);
            x_axis_layout->addRow("Min:", this->min_spinbox = new QSpinBox());
            connect(this->min_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);
            x_axis_layout->addRow("Max:", this->max_spinbox = new QSpinBox());
            connect(this->max_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);
            x_axis_layout->addRow("Step:", this->step_spinbox = new QSpinBox());
            connect(this->step_spinbox, &QSpinBox::valueChanged, this, &PlotTab::update_plot);

            auto y_axis_box = new QGroupBox("Y-Axis");
            upper_right_layout->addWidget(y_axis_box);
            auto y_axis_layout = new QFormLayout(y_axis_box);
            y_axis_layout->addRow("Metric:", this->metric_combobox = new QComboBox());
            connect(this->metric_combobox, &QComboBox::currentTextChanged, this, &PlotTab::update_plot);

            this->weapon_table = new WeaponTable(this);
        };

        void update_plot()
        {
            this->update_plot_timer->start(500);
        }

        void add_datasets(const std::vector<std::reference_wrapper<const calculator::Attack>>& attacks)
        {

        }
    };

}