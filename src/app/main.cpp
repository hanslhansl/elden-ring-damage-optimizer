#include <QApplication>
#include <KDChartWidget>
#include <KDChartLineDiagram>
#include <KDChartCartesianAxis>
using namespace KDChart;




int main2(int argc, char **argv)
{
    QApplication app( argc, argv );
    Widget widget;
    // our Widget can be configured
    // as any Qt Widget
    widget.resize( 600, 600 );
    // store the data and assign it
    QVector< double > vec0, vec1, vec2;
    vec0 << 5 << 1 << 3 << 4 << 1;
    vec1 << 3 << 6 << 2 << 4 << 8;
    vec2 << 0 << 7 << 1 << 2 << 1;
    widget.setDataset( 0, vec0, "vec0" );
    widget.setDataset( 1, vec1, "vec1" );
    widget.setDataset( 2, vec2, "vec2" );

    CartesianAxis *xAxis = new CartesianAxis( widget.lineDiagram() );
    CartesianAxis *xAxis2 = new CartesianAxis( widget.lineDiagram() );

    CartesianAxis *yAxis = new CartesianAxis(widget.lineDiagram() );

    xAxis->setPosition( CartesianAxis::Bottom );
    xAxis2->setPosition( CartesianAxis::Bottom ); //Top
    yAxis->setPosition( CartesianAxis::Left );


    xAxis->setTitleText( "Abscissa bottom position" );
    xAxis2->setTitleText( "Abscissa top position" );
    yAxis->setTitleText( "Ordinate left position" );

    widget.lineDiagram()->addAxis( xAxis );
    widget.lineDiagram()->addAxis( xAxis2 );
    widget.lineDiagram()->addAxis( yAxis );

    widget.show();
    return app.exec();
}

import std;
import erdo;
import erdo.ui;
int main(int argc, char *argv[])
{
	return erdo::ui::run_ui(argc, argv);

	// auto xml_data_directory = std::filesystem::current_path() / "xml_data" / "11611000";
    // auto&& weapons = erdo::parser::load_weapons(xml_data_directory);
	
    // erdo::calculator::AttackOptions attack_options{{0, 25, 10}, true};
    // const auto min_stats = erdo::calculator::character_class_stats.at("wretch");
    // const auto min_relevant_stats = min_stats.relevant_stats();
    // const auto free_attribute_points = 11;

    // constexpr auto optimizer = erdo::optimizer::V2<erdo::optimizer::Target::TOTAL_ATTACK_POWER>{};
	// auto attacks = optimizer.run_synchronously(weapons, attack_options, free_attribute_points, min_stats);

	// std::println("{}", attacks.back().weapon.get().full_name);
	// return 0;
}