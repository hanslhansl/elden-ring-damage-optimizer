module;
#include <QPrinter>
#include "ui_plot_tab.h"
export module erdo.ui.plot_tab;

import std;
import erdo;
import erdo.ui.settings;
import erdo.ui.weapons_table;

namespace erdo::ui
{
    export struct PlotTab : QWidget, Ui::PlotTab
    {
        explicit PlotTab(QWidget *parent = nullptr) : QWidget(parent)
        {
            // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
            QPrinter printer;
            Q_UNUSED(printer);

            this->setupUi(this);
        };
    };

}