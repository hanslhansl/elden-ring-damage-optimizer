module;
#include <QStandarditemmodel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPrinter>
#include <QCompleter>
#include <QKeyEvent>
#include <QModelIndex>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QPointer>

#include <KDChartChart>
#include <KDChartWidget>
#include <KDChartCartesianAxis>
#include <KDChartLineDiagram>
#include <KDChartGridAttributes>
#include <KDChartPlotter>
#include <KDChartDataValueAttributes>
#include <KDChartCartesianCoordinatePlane>
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

    class TrackingChart : public KDChart::Chart
    {
    public:
        explicit TrackingChart(QWidget *parent = nullptr) : KDChart::Chart(parent)
        {
            setMouseTracking(true);
        }

        void setTrackingModel(QAbstractItemModel *model)
        {
            if (m_model == model)
                return;

            if (m_model)
                disconnect(m_model, nullptr, this, nullptr);

            m_model = model;

            if (!m_model) {
                m_datasets.clear();
                clearTracking();
                return;
            }

            /*
            * Column changes alter the dataset layout.
            */
            connect(
                m_model,
                &QAbstractItemModel::columnsInserted,
                this,
                &TrackingChart::scheduleTrackingModelRefresh);

            connect(
                m_model,
                &QAbstractItemModel::columnsRemoved,
                this,
                &TrackingChart::scheduleTrackingModelRefresh);

            /*
            * A model reset can change absolutely everything.
            */
            connect(
                m_model,
                &QAbstractItemModel::modelReset,
                this,
                &TrackingChart::scheduleTrackingModelRefresh);

            /*
            * Row changes are handled separately. Since valid points
            * are contiguous from row 0 and invalid values are trailing,
            * we can update the cached lastValidRow without rebuilding
            * the whole dataset layout.
            */
            connect(
                m_model,
                &QAbstractItemModel::rowsInserted,
                this,
                &TrackingChart::scheduleTrackingModelRefresh);

            connect(
                m_model,
                &QAbstractItemModel::rowsRemoved,
                this,
                &TrackingChart::scheduleTrackingModelRefresh);

            rebuildDatasetInfo();
        }


        /*
        * Call this after you have finished changing/filling the model.
        *
        * This is particularly useful when rows are inserted first and
        * their values are populated afterwards, since the
        * rowsInserted() signal happens before those values necessarily
        * exist.
        */
        void refreshTrackingModel()
        {
            m_refreshScheduled = false;

            rebuildDatasetInfo();

            if (m_tracking)
                updateTracking();
            else
                update();
        }

        void setDatasetTrackable(int dataset, bool trackable)
        {
            if (dataset < 0)
                return;

            m_datasetTrackable[dataset] = trackable;

            if (dataset >= m_datasets.size())
                return;

            if (m_tracking)
                updateTracking();
        }

        void setTrackingAxes(KDChart::CartesianAxis *xAxis, KDChart::CartesianAxis *yAxis)
        {
            m_xAxis = xAxis;
            m_yAxis = yAxis;

            if (m_tracking)
                update();
        }

    protected:
        void scheduleTrackingModelRefresh()
        {
            if (m_refreshScheduled)
                return;

            m_refreshScheduled = true;

            QMetaObject::invokeMethod(
                this,
                [this] {
                    m_refreshScheduled = false;

                    if (!m_model)
                        return;

                    rebuildDatasetInfo();
                    clearTracking();
                },
                Qt::QueuedConnection);
        }

        void mouseMoveEvent(QMouseEvent *event) override
        {
            KDChart::Chart::mouseMoveEvent(event);

            if (!m_model) {
                clearTracking();
                return;
            }

            auto *plane =
                qobject_cast<KDChart::CartesianCoordinatePlane *>(
                    coordinatePlane());

            if (!plane) {
                clearTracking();
                return;
            }

            const QRectF plotArea =
                plane->visibleDiagramArea();

            if (!plotArea.contains(event->position())) {
                clearTracking();
                return;
            }

            m_cursorPos = event->position();

            /*
            * Convert mouse position from pixel coordinates to
            * chart/data coordinates.
            */
            const QPointF dataPos =
                plane->translateBack(m_cursorPos);

            m_cursorDataX = dataPos.x();

            updateTracking();
        }


        void leaveEvent(QEvent *event) override
        {
            KDChart::Chart::leaveEvent(event);

            clearTracking();
        }


        void paintEvent(QPaintEvent *event) override
        {
            /*
            * Let KDChart render the chart first.
            */
            KDChart::Chart::paintEvent(event);

            if (!m_tracking)
                return;

            QPainter painter(this);

            painter.setRenderHint(
                QPainter::Antialiasing);

            drawTracking(painter);
        }


    private:

        struct DatasetInfo
        {
            int xColumn = -1;
            int yColumn = -1;

            /*
            * Valid rows are assumed to be:
            *
            *   0 ... lastValidRow
            *
            * followed by invalid rows.
            */
            int lastValidRow = -1;

            QString name;
        };


        struct TrackingPoint
        {
            int dataset = -1;
            int row = -1;

            double x = 0.0;
            double y = 0.0;

            QPointF pixelPos;
        };


        static bool isValid(const QVariant &value)
        {
            return value.isValid() &&
                !value.isNull();
        }


        void rebuildDatasetInfo()
        {
            m_datasets.clear();

            if (!m_model)
                return;

            const int columnCount =
                m_model->columnCount();

            const int datasetCount =
                columnCount / 2;

            m_datasets.reserve(datasetCount);

            for (int dataset = 0;
                dataset < datasetCount;
                ++dataset) {

                DatasetInfo info;

                info.xColumn =
                    dataset * 2;

                info.yColumn =
                    info.xColumn + 1;

                /*
                * Dataset names live in the X-column header.
                */
                const QVariant header =
                    m_model->headerData(
                        info.xColumn,
                        Qt::Horizontal,
                        Qt::DisplayRole);

                if (header.isValid())
                    info.name = header.toString();

                /*
                * Empty/default header -> useful fallback.
                */
                if (info.name.isEmpty()) {
                    info.name =
                        QStringLiteral("Dataset %1")
                            .arg(dataset + 1);
                }

                info.lastValidRow =
                    calculateLastValidRow(
                        info.xColumn,
                        info.yColumn);

                m_datasets.append(info);
            }
        }


        int calculateLastValidRow( int xColumn, int yColumn) const
        {
            if (!m_model)
                return -1;

            for (int row =
                    m_model->rowCount() - 1;
                row >= 0;
                --row) {

                const QVariant x =
                    m_model->index(
                        row,
                        xColumn)
                        .data();

                const QVariant y =
                    m_model->index(
                        row,
                        yColumn)
                        .data();

                if (isValid(x) &&
                    isValid(y)) {

                    return row;
                }
            }

            return -1;
        }


        /*
        * Update cached ranges after rows are inserted.
        *
        * Since inserted rows are normally initially empty, we don't
        * assume that the new rows are valid. We simply recalculate
        * datasets whose cached end could have been affected.
        *
        * This is still considerably cheaper than rebuilding the
        * dataset metadata, and refreshTrackingModel() can be called
        * after populating the rows.
        */
        void rowsInserted(int first, int last)
        {
            Q_UNUSED(first);

            if (!m_model)
                return;

            /*
            * Insertion after the previous end shifts the cached end.
            */
            const int count =
                last - first + 1;

            for (DatasetInfo &dataset :
                m_datasets) {

                if (dataset.lastValidRow >= first)
                    dataset.lastValidRow += count;
            }

            /*
            * The inserted rows may themselves contain data already,
            * so determine the actual end again.
            */
            for (DatasetInfo &dataset :
                m_datasets) {

                dataset.lastValidRow =
                    calculateLastValidRow(
                        dataset.xColumn,
                        dataset.yColumn);
            }
        }


        void rowsRemoved(int first, int last)
        {
            if (!m_model)
                return;

            const int count =
                last - first + 1;

            for (DatasetInfo &dataset :
                m_datasets) {

                if (dataset.lastValidRow < first)
                    continue;

                if (dataset.lastValidRow <= last) {

                    /*
                    * The cached final point was removed.
                    */
                    dataset.lastValidRow =
                        first - 1;

                } else {

                    /*
                    * The cached final point survived, but moved up.
                    */
                    dataset.lastValidRow -= count;
                }
            }

            /*
            * This handles cases where the removal exposed a different
            * validity boundary.
            */
            for (DatasetInfo &dataset :
                m_datasets) {

                if (dataset.lastValidRow >= 0) {

                    const QVariant x =
                        m_model->index(
                            dataset.lastValidRow,
                            dataset.xColumn)
                            .data();

                    const QVariant y =
                        m_model->index(
                            dataset.lastValidRow,
                            dataset.yColumn)
                            .data();

                    if (!isValid(x) ||
                        !isValid(y)) {

                        dataset.lastValidRow =
                            calculateLastValidRow(
                                dataset.xColumn,
                                dataset.yColumn);
                    }
                }
            }
        }


        int nearestRow( const DatasetInfo &dataset, double targetX) const
        {
            const int lastRow =
                dataset.lastValidRow;

            if (lastRow < 0)
                return -1;

            int lo = 0;
            int hi = lastRow;

            /*
            * Find the first X >= targetX.
            */
            while (lo < hi) {

                const int mid =
                    lo + (hi - lo) / 2;

                const double x =
                    m_model->index(
                        mid,
                        dataset.xColumn)
                        .data()
                        .toDouble();

                if (x < targetX)
                    lo = mid + 1;
                else
                    hi = mid;
            }

            /*
            * targetX is at or before the first point.
            */
            if (lo == 0)
                return 0;

            /*
            * Compare the points on either side.
            */
            const double x1 =
                m_model->index(
                    lo - 1,
                    dataset.xColumn)
                    .data()
                    .toDouble();

            const double x2 =
                m_model->index(
                    lo,
                    dataset.xColumn)
                    .data()
                    .toDouble();

            return std::abs(x1 - targetX) <=
                        std::abs(x2 - targetX)
                    ? lo - 1
                    : lo;
        }


        void updateTracking()
        {
            m_trackingPoints.clear();

            if (!m_model)
                return;

            auto *plane =
                qobject_cast<KDChart::CartesianCoordinatePlane *>(
                    coordinatePlane());

            if (!plane)
                return;

            for (int dataset = 0; dataset < m_datasets.size(); ++dataset)
            {

                if (!m_datasetTrackable.value(dataset, true))
                    continue;

                const DatasetInfo &info =
                    m_datasets[dataset];

                if (info.lastValidRow < 0)
                    continue;

                const double firstX =
                    m_model->index(
                        0,
                        info.xColumn)
                        .data()
                        .toDouble();

                const double lastX =
                    m_model->index(
                        info.lastValidRow,
                        info.xColumn)
                        .data()
                        .toDouble();

                /*
                * The dataset doesn't exist at this X.
                */
                if (m_cursorDataX < firstX ||
                    m_cursorDataX > lastX) {

                    continue;
                }

                const int row =
                    nearestRow(
                        info,
                        m_cursorDataX);

                if (row < 0)
                    continue;

                const double x =
                    m_model->index(
                        row,
                        info.xColumn)
                        .data()
                        .toDouble();

                const double y =
                    m_model->index(
                        row,
                        info.yColumn)
                        .data()
                        .toDouble();

                TrackingPoint point;

                point.dataset = dataset;
                point.row = row;
                point.x = x;
                point.y = y;

                point.pixelPos =
                    plane->translate(
                        QPointF(x, y));

                m_trackingPoints.append(point);
            }

            m_tracking =
                !m_trackingPoints.isEmpty();

            update();
        }


        void clearTracking()
        {
            if (!m_tracking)
                return;

            m_tracking = false;
            m_trackingPoints.clear();

            update();
        }


        void drawTracking(QPainter &painter)
        {
            auto *plane = qobject_cast<KDChart::CartesianCoordinatePlane *>(coordinatePlane());

            if (!plane)
                return;

            const QRectF plotArea = plane->visibleDiagramArea();

            /*
            * ---------------------------------------------------------
            * Vertical crosshair
            * ---------------------------------------------------------
            */

            QPen crosshairPen;
            crosshairPen.setStyle(Qt::DashLine);
            crosshairPen.setWidth(1);

            painter.setPen(crosshairPen);

            painter.drawLine(
                QPointF(
                    m_cursorPos.x(),
                    plotArea.top()),

                QPointF(
                    m_cursorPos.x(),
                    plotArea.bottom()));


            /*
            * ---------------------------------------------------------
            * Selected data points
            * ---------------------------------------------------------
            */

            QPen pointPen;
            pointPen.setWidth(2);

            painter.setPen(pointPen);
            painter.setBrush(Qt::NoBrush);

            for (const TrackingPoint &point :
                m_trackingPoints) {

                painter.drawEllipse(point.pixelPos, settings.plot_point_diameter / 2., settings.plot_point_diameter / 2.);
            }


            /*
            * ---------------------------------------------------------
            * Single tracking box
            * ---------------------------------------------------------
            */

            drawTrackingBox(
                painter,
                plotArea);
        }


        void drawTrackingBox(QPainter &painter, const QRectF &plotArea)
        {
            if (m_trackingPoints.isEmpty())
                return;

            constexpr int padding = 8;
            constexpr int cellPaddingX = 7;
            constexpr int cellPaddingY = 4;
            constexpr int columnSpacing = 0;

            auto *plane =
                qobject_cast<KDChart::CartesianCoordinatePlane *>(
                    coordinatePlane());

            if (!plane)
                return;

            /*
            * Use the actual KDChart axis titles.
            *
            * Fall back to X/Y if no title has been assigned.
            */
            QString xTitle = QStringLiteral("X");
            QString yTitle = QStringLiteral("Y");

            if (m_xAxis)
                xTitle = m_xAxis->titleText();

            if (m_yAxis)
                yTitle = m_yAxis->titleText();

            if (xTitle.isEmpty())
                xTitle = QStringLiteral("X");

            if (yTitle.isEmpty())
                yTitle = QStringLiteral("Y");


            /*
            * -------------------------------------------------------------
            * Build table contents
            * -------------------------------------------------------------
            */

            struct TableRow
            {
                QString dataset;
                QString x;
                QString y;
            };

            QVector<TableRow> rows;

            rows.reserve(m_trackingPoints.size());

            for (const TrackingPoint &point :
                m_trackingPoints) {

                const DatasetInfo &dataset =
                    m_datasets[point.dataset];

                TableRow row;

                row.dataset = dataset.name;

                // row.x = QString::number(
                //     point.x,
                //     'g',
                //     8);
                // row.y = QString::number(
                //     point.y,
                //     'g',
                //     8);
                row.x = format_number(point.x);
                row.y = format_number(point.y);

                rows.append(row); //format_number
            }


            /*
            * -------------------------------------------------------------
            * Fonts
            * -------------------------------------------------------------
            */

            const QFont normalFont =
                painter.font();

            QFont headerFont =
                normalFont;

            headerFont.setBold(true);


            /*
            * -------------------------------------------------------------
            * Measure columns
            * -------------------------------------------------------------
            */

            QFontMetrics normalMetrics(
                normalFont);

            QFontMetrics headerMetrics(
                headerFont);

            const int datasetHeaderWidth =
                headerMetrics.horizontalAdvance(
                    QStringLiteral("Dataset"));

            const int xHeaderWidth =
                headerMetrics.horizontalAdvance(
                    xTitle);

            const int yHeaderWidth =
                headerMetrics.horizontalAdvance(
                    yTitle);

            int datasetWidth =
                datasetHeaderWidth;

            int xWidth =
                xHeaderWidth;

            int yWidth =
                yHeaderWidth;

            for (const TableRow &row : rows) {

                datasetWidth =
                    std::max(
                        datasetWidth,
                        normalMetrics.horizontalAdvance(
                            row.dataset));

                xWidth =
                    std::max(
                        xWidth,
                        normalMetrics.horizontalAdvance(
                            row.x));

                yWidth =
                    std::max(
                        yWidth,
                        normalMetrics.horizontalAdvance(
                            row.y));
            }

            datasetWidth +=
                cellPaddingX * 2;

            xWidth +=
                cellPaddingX * 2;

            yWidth +=
                cellPaddingX * 2;


            /*
            * -------------------------------------------------------------
            * Header + rows
            * -------------------------------------------------------------
            */

            const int headerHeight =
                headerMetrics.height() +
                cellPaddingY * 2;

            const int rowHeight =
                normalMetrics.height() +
                cellPaddingY * 2;

            const int tableWidth =
                datasetWidth +
                xWidth +
                yWidth +
                columnSpacing * 2;

            const int tableHeight =
                headerHeight +
                rowHeight * rows.size();


            /*
            * -------------------------------------------------------------
            * Cursor X header
            * -------------------------------------------------------------
            */

            const QString cursorText =
                QStringLiteral("%1 = %2")
                    .arg(xTitle)
                    .arg(
                        m_cursorDataX,
                        0,
                        'g',
                        8);

            const int cursorHeight =
                normalMetrics.height();


            /*
            * -------------------------------------------------------------
            * Complete box size
            * -------------------------------------------------------------
            */

            const int boxWidth =
                tableWidth + padding * 2;

            const int boxHeight =
                padding * 2 +
                cursorHeight +
                6 +
                tableHeight;


            /*
            * -------------------------------------------------------------
            * Position
            * -------------------------------------------------------------
            */
            qreal trackingBoxGap = settings.plot_point_diameter / 2. + 20.0;

            QRectF box(
                m_cursorPos.x() + trackingBoxGap,
                plotArea.top() + 12,
                boxWidth,
                boxHeight);

            /*
            * Prefer the right side of the cursor.
            */
            if (box.right() > plotArea.right()) {

                box.moveRight(
                    m_cursorPos.x() - trackingBoxGap);
            }

            /*
            * Keep vertically inside the plot area.
            */
            if (box.bottom() > plotArea.bottom()) {

                box.moveBottom(
                    plotArea.bottom() - 5);
            }

            if (box.top() < plotArea.top()) {

                box.moveTop(
                    plotArea.top() + 5);
            }


            /*
            * -------------------------------------------------------------
            * Background
            * -------------------------------------------------------------
            */

            painter.setPen(Qt::NoPen);

            painter.setBrush(
                QColor(255, 255, 255, 235));

            painter.drawRoundedRect(
                box,
                4,
                4);


            /*
            * Border
            */

            QPen borderPen;
            borderPen.setWidth(1);

            painter.setPen(borderPen);
            painter.setBrush(Qt::NoBrush);

            painter.drawRoundedRect(
                box,
                4,
                4);


            /*
            * -------------------------------------------------------------
            * Cursor X
            * -------------------------------------------------------------
        */

            painter.setPen(Qt::black);
            painter.setFont(normalFont);

            QPointF textPos(
                box.left() + padding,
                box.top() +
                    padding +
                    normalMetrics.ascent());

            painter.drawText(
                textPos,
                cursorText);


            /*
            * -------------------------------------------------------------
            * Table geometry
            * -------------------------------------------------------------
            */

            const qreal tableLeft =
                box.left() + padding;

            const qreal tableTop =
                box.top() +
                padding +
                cursorHeight +
                6;

            const qreal xColumnLeft =
                tableLeft + datasetWidth;

            const qreal yColumnLeft =
                xColumnLeft + xWidth;


            /*
            * -------------------------------------------------------------
            * Header background
            * -------------------------------------------------------------
            */

            painter.setPen(Qt::NoPen);

            painter.setBrush(
                QColor(235, 235, 235));

            painter.drawRect(
                QRectF(
                    tableLeft,
                    tableTop,
                    tableWidth,
                    headerHeight));


            /*
            * -------------------------------------------------------------
            * Table grid
            * -------------------------------------------------------------
            */

            QPen gridPen;
            gridPen.setWidth(1);

            painter.setPen(gridPen);
            painter.setBrush(Qt::NoBrush);

            /*
            * Outer rectangle.
            */
            painter.drawRect(
                QRectF(
                    tableLeft,
                    tableTop,
                    tableWidth,
                    tableHeight));


            /*
            * Vertical lines.
            */
            painter.drawLine(
                QPointF(
                    xColumnLeft,
                    tableTop),

                QPointF(
                    xColumnLeft,
                    tableTop +
                        tableHeight));

            painter.drawLine(
                QPointF(
                    yColumnLeft,
                    tableTop),

                QPointF(
                    yColumnLeft,
                    tableTop +
                        tableHeight));


            /*
            * Horizontal line below header.
            */
            painter.drawLine(
                QPointF(
                    tableLeft,
                    tableTop +
                        headerHeight),

                QPointF(
                    tableLeft +
                        tableWidth,
                    tableTop +
                        headerHeight));


            /*
            * Horizontal lines between data rows.
            */
            for (int row = 1;
                row < rows.size();
                ++row) {

                const qreal y =
                    tableTop +
                    headerHeight +
                    rowHeight * row;

                painter.drawLine(
                    QPointF(
                        tableLeft,
                        y),

                    QPointF(
                        tableLeft +
                            tableWidth,
                        y));
            }


            /*
            * -------------------------------------------------------------
            * Header text
            * -------------------------------------------------------------
            */

            painter.setFont(headerFont);
            painter.setPen(Qt::black);

            auto drawCenteredText =
                [&](const QRectF &rect,
                    const QString &text) {

                    painter.drawText(
                        rect,
                        Qt::AlignCenter,
                        text);
                };

            drawCenteredText(
                QRectF(
                    tableLeft,
                    tableTop,
                    datasetWidth,
                    headerHeight),
                QStringLiteral("Dataset"));

            drawCenteredText(
                QRectF(
                    xColumnLeft,
                    tableTop,
                    xWidth,
                    headerHeight),
                xTitle);

            drawCenteredText(
                QRectF(
                    yColumnLeft,
                    tableTop,
                    yWidth,
                    headerHeight),
                yTitle);


            /*
            * -------------------------------------------------------------
            * Data rows
            * -------------------------------------------------------------
            */

            painter.setFont(normalFont);

            for (int row = 0;
                row < rows.size();
                ++row) {

                const TableRow &data =
                    rows[row];

                const qreal top =
                    tableTop +
                    headerHeight +
                    rowHeight * row;

                const QRectF datasetRect(
                    tableLeft,
                    top,
                    datasetWidth,
                    rowHeight);

                const QRectF xRect(
                    xColumnLeft,
                    top,
                    xWidth,
                    rowHeight);

                const QRectF yRect(
                    yColumnLeft,
                    top,
                    yWidth,
                    rowHeight);

                painter.drawText(
                    datasetRect.adjusted(
                        cellPaddingX,
                        0,
                        -cellPaddingX,
                        0),
                    Qt::AlignVCenter |
                        Qt::AlignLeft,
                    data.dataset);

                painter.drawText(
                    xRect.adjusted(
                        cellPaddingX,
                        0,
                        -cellPaddingX,
                        0),
                    Qt::AlignVCenter |
                        Qt::AlignRight,
                    data.x);

                painter.drawText(
                    yRect.adjusted(
                        cellPaddingX,
                        0,
                        -cellPaddingX,
                        0),
                    Qt::AlignVCenter |
                        Qt::AlignRight,
                    data.y);
            }
        }

    private:
        QPointer<QAbstractItemModel> m_model;

        KDChart::CartesianAxis *m_xAxis = nullptr;
        KDChart::CartesianAxis *m_yAxis = nullptr;

        QVector<DatasetInfo> m_datasets;

        bool m_tracking = false;
        bool m_refreshScheduled = false;

        QPointF m_cursorPos;
        double m_cursorDataX = 0.0;

        QVector<TrackingPoint> m_trackingPoints;
        QHash<int, bool> m_datasetTrackable;
    };

    class SearchableComboBox : public QComboBox
    {
        Q_OBJECT

    public:
        explicit SearchableComboBox(QWidget* parent = nullptr)
            : QComboBox(parent),
            m_filterModel(new QSortFilterProxyModel(this)),
            m_completer(new QCompleter(m_filterModel, this)),
            m_lastValidIndex(-1),
            m_updatePending(false),
            m_internalUpdate(false)
        {
            setFocusPolicy(Qt::ClickFocus);
            setEditable(true);
            setInsertPolicy(QComboBox::NoInsert);

            // ------------------------------------------------------------------
            // Filter model
            // ------------------------------------------------------------------

            m_filterModel->setSourceModel(model());
            m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
            m_filterModel->setFilterRole(Qt::DisplayRole);
            m_filterModel->setFilterKeyColumn(modelColumn());

            // ------------------------------------------------------------------
            // Completer
            // ------------------------------------------------------------------

            m_completer->setCompletionMode(
                QCompleter::UnfilteredPopupCompletion
            );
            m_completer->setCaseSensitivity(Qt::CaseInsensitive);
            m_completer->setCompletionRole(Qt::DisplayRole);
            m_completer->setCompletionColumn(modelColumn());

            setCompleter(m_completer);

            // ------------------------------------------------------------------
            // Signals
            // ------------------------------------------------------------------

            connect(
                lineEdit(),
                &QLineEdit::textEdited,
                this,
                &SearchableComboBox::onTextEdited
            );

            connect(
                m_completer,
                qOverload<const QModelIndex&>(&QCompleter::activated),
                this,
                &SearchableComboBox::onCompleterActivated
            );

            connect(
                this,
                &QComboBox::currentIndexChanged,
                this,
                &SearchableComboBox::onCurrentIndexChanged
            );

            if (currentIndex() >= 0)
                m_lastValidIndex = currentIndex();
        }

        // ----------------------------------------------------------------------
        // Model handling
        // ----------------------------------------------------------------------

        void setModel(QAbstractItemModel* model) override
        {
            QComboBox::setModel(model);

            m_filterModel->setSourceModel(model);
            updateModelColumn();

            m_completer->setModel(m_filterModel);

            if (currentIndex() >= 0)
                m_lastValidIndex = currentIndex();
            else
                m_lastValidIndex = -1;
        }

        /*
        * QComboBox::setModelColumn() is not virtual, hence no "override".
        */
        void setModelColumn(int column)
        {
            QComboBox::setModelColumn(column);
            updateModelColumn();
        }

    protected:
        void keyPressEvent(QKeyEvent* event) override
        {
            // --------------------------------------------------------------
            // Enter / Return
            //
            // Complete the current partial search with the first match.
            // --------------------------------------------------------------

            if (event->key() == Qt::Key_Return ||
                event->key() == Qt::Key_Enter)
            {
                acceptFirstMatch();
                event->accept();
                return;
            }

            // --------------------------------------------------------------
            // Escape
            // --------------------------------------------------------------

            if (event->key() == Qt::Key_Escape)
            {
                m_completer->popup()->hide();
                restoreLastValidSelection();

                event->accept();
                return;
            }

            QComboBox::keyPressEvent(event);
        }

        void focusOutEvent(QFocusEvent* event) override
        {
            m_completer->popup()->hide();
            restoreLastValidSelection();

            QComboBox::focusOutEvent(event);
        }

    private slots:

        void onTextEdited(const QString& text)
        {
            /*
            * Do not immediately modify currentIndex().
            *
            * QComboBox has its own internal handling of edits to an editable
            * combo. In particular, when the text becomes empty, that handling
            * can change the current index and/or line-edit contents.
            *
            * Queue our processing so QComboBox has finished processing the
            * user's edit first.
            */
            m_pendingSearchText = text;

            if (m_updatePending)
                return;

            m_updatePending = true;

            QTimer::singleShot(
                0,
                this,
                &SearchableComboBox::processPendingSearch
            );
        }

        void processPendingSearch()
        {
            m_updatePending = false;

            if (m_internalUpdate)
                return;

            const QString searchText = m_pendingSearchText;

            // Remember the cursor position from the actual current edit.
            const int cursorPosition = lineEdit()->cursorPosition();

            // --------------------------------------------------------------
            // Filter
            // --------------------------------------------------------------

            m_filterModel->setFilterRegularExpression(
                QRegularExpression::escape(searchText)
            );

            // --------------------------------------------------------------
            // No matches
            // --------------------------------------------------------------

            if (m_filterModel->rowCount() == 0)
            {
                m_completer->popup()->hide();

                /*
                * The current selection remains untouched.
                *
                * Restore the search text after QComboBox's own processing.
                */
                restoreSearchText(searchText, cursorPosition);
                return;
            }

            // --------------------------------------------------------------
            // First match
            // --------------------------------------------------------------

            const QModelIndex proxyIndex =
                m_filterModel->index(0, modelColumn());

            if (!proxyIndex.isValid())
                return;

            const QModelIndex sourceIndex =
                m_filterModel->mapToSource(proxyIndex);

            if (!sourceIndex.isValid())
                return;

            const int row = sourceIndex.row();

            /*
            * Changing the current index causes an editable QComboBox to update
            * its line edit. That is exactly what we do NOT want while searching.
            */
            m_internalUpdate = true;

            setCurrentIndex(row);
            m_lastValidIndex = row;

            m_internalUpdate = false;

            // Put the user's search text back.
            restoreSearchText(searchText, cursorPosition);

            // --------------------------------------------------------------
            // Popup
            // --------------------------------------------------------------

            if (searchText.isEmpty())
            {
                m_completer->popup()->hide();
            }
            else
            {
                m_completer->complete();
            }
        }

        void onCompleterActivated(const QModelIndex& index)
        {
            if (!index.isValid())
                return;

            if (index.model() != m_filterModel)
                return;

            const QModelIndex sourceIndex =
                m_filterModel->mapToSource(index);

            if (!sourceIndex.isValid())
                return;

            commitRow(sourceIndex.row());
        }

        void onCurrentIndexChanged(int index)
        {
            /*
            * Only remember real selections.
            *
            * During user editing QComboBox may temporarily move to -1.
            * That must never replace our last valid selection.
            */
            if (index >= 0)
                m_lastValidIndex = index;
        }

    private:

        void updateModelColumn()
        {
            const int column = modelColumn();

            m_filterModel->setFilterKeyColumn(column);
            m_completer->setCompletionColumn(column);
        }

        void restoreSearchText(const QString& text, int cursorPosition)
        {
            /*
            * Block QLineEdit signals so restoring the search text does not
            * trigger another search cycle.
            */
            const QSignalBlocker blocker(lineEdit());

            lineEdit()->setText(text);

            lineEdit()->setCursorPosition(
                qMin(cursorPosition, text.size())
            );
        }

        void commitRow(int row)
        {
            if (row < 0 || row >= count())
                return;

            m_internalUpdate = true;

            setCurrentIndex(row);
            m_lastValidIndex = row;

            // A real committed selection displays its complete item text.
            setEditText(itemText(row));

            m_internalUpdate = false;

            m_completer->popup()->hide();
        }

        void acceptFirstMatch()
        {
            if (m_filterModel->rowCount() == 0)
            {
                restoreLastValidSelection();
                return;
            }

            const QModelIndex proxyIndex =
                m_filterModel->index(0, modelColumn());

            if (!proxyIndex.isValid())
                return;

            const QModelIndex sourceIndex =
                m_filterModel->mapToSource(proxyIndex);

            if (!sourceIndex.isValid())
                return;

            commitRow(sourceIndex.row());
        }

        void restoreLastValidSelection()
        {
            if (m_lastValidIndex < 0 ||
                m_lastValidIndex >= count())
            {
                return;
            }

            const int row = m_lastValidIndex;

            m_internalUpdate = true;

            setCurrentIndex(row);
            setEditText(itemText(row));

            m_internalUpdate = false;
        }

    private:
        QSortFilterProxyModel* m_filterModel;
        QCompleter* m_completer;

        int m_lastValidIndex;

        // Search processing is queued to the event loop.
        bool m_updatePending;

        // True while we deliberately modify the combo/edit ourselves.
        bool m_internalUpdate;

        QString m_pendingSearchText;
    };

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
            return BasicRow<
                sections::Color,
                sections::Name,
                sections::BaseName,
                sections::Affinity,
                sections::Type,
                sections::BaseGameDLC,
                sections::UpgradeLevel,
                sections::TwoHanding,
                sections::CharacterLevel,

                sections::Stats,
                sections::Requirements,
                sections::AttributeScalings
            >{};
        }(0));
        static_assert(Row::sparse);

        std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data{};

        QStandardItemModel *model;
        TrackingChart* chart;
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

        void update_datasets(int wi, int w_count)
        {
            if (wi < 0 || wi + w_count > this->weapon_table->model->rows.size())
                throw std::runtime_error(std::format("Invalid range for update_datasets: index: {}, count: {}, rows: {}", wi, w_count, this->weapon_table->model->rows.size()));

            auto variable_index = this->variable_combobox->currentIndex();
            auto variable = static_cast<PlotVariable>(variable_index);
            auto variable_projection = variable_projections.at(variable_index);

            auto&& universal_xs = get_universal_x_values(variable);
            for(auto j = 0; j < universal_xs.size(); ++j)
            {
                this->model->setData(this->model->index(j, 0), universal_xs[j]);
                this->model->setData(this->model->index(j, 1), double(j == 1));
            }

            auto metric_index = this->metric_combobox->currentIndex();
            auto metric = static_cast<optimizer::Target>(metric_index);
            auto metric_projection = optimizer::projections.at(metric_index);
            auto attributes_model = this->plotter->attributesModel();
            for (auto&& [wi, row] : this->weapon_table->model->rows | std::views::enumerate | std::views::drop(wi) | std::views::take(w_count))
            {
                auto i = this->weapon_index_to_dataset(wi);
                const auto column = i * 2;

                auto&& attack_options = row.attack;
                auto original_x = variable_projection(attack_options);
                auto&& weapon = attack_options.weapon.get();
                calculator::Attack attack{ weapon, attack_options.stats, attack_options };

                auto dataset_color = std::get<sections::Color>(this->weapon_table->model->rows.at(wi))[0].value<QColor>();
                auto pen = this->plotter->pen(i);
                pen.setColor(dataset_color);
                this->plotter->setPen(i, pen);

                this->model->setHeaderData(
                    column,
                    Qt::Horizontal,
                    this->weapon_table->model->rows.at(wi).attack.weapon.get().full_name.data()
                );
                
                auto&& xs = get_dataset_x_values(variable, weapon);
                for(auto j = 0; j < xs.size(); ++j)
                {
                    auto x_j = xs[j];
                    variable_projection(attack) = x_j;
                    attack.calculate_inplace();
                    auto y_ij = metric_projection(attack);

                    auto x_index = this->model->index(j, column);
                    auto y_index = this->model->index(j, column + 1);

                    if (x_j == original_x)
                    {
                        auto dva = this->plotter->dataValueAttributes(x_index);
                        auto marker = dva.markerAttributes();
                        marker.setMarkerColor(dataset_color);
                        marker.setMarkerSize(QSizeF(settings.plot_point_diameter, settings.plot_point_diameter));
                        dva.setMarkerAttributes(marker);
                        dva.setVisible(true);
                        this->plotter->setDataValueAttributes(x_index, dva);
                    }
                    else
                    {
                        attributes_model->resetData(x_index, KDChart::DataValueLabelAttributesRole);
                    }

                    this->model->setData(x_index, x_j);
                    this->model->setData(y_index, y_ij);
                }
                for(auto j = xs.size(); j < universal_xs.size(); ++j)
                {
                    this->model->setData(this->model->index(j, column), QVariant());
                    this->model->setData(this->model->index(j, column + 1), QVariant());
                    attributes_model->resetData(this->model->index(j, column), KDChart::DataValueLabelAttributesRole);
                }
            }

            this->plotter->update();
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
                this->update_datasets(wi, 1);
        }

        QSplitterHandle *createHandle() override
        {
            return new SplitterHandle(Qt::Vertical, this);
        }

        bool edit_dataset_dialog_impl(calculator::FullAttackOptions& attack_options, QString title)
        {
            QDialog dialog(this);
            dialog.setWindowTitle(title);

            // create widgets
            auto base_name_combobox = new SearchableComboBox(&dialog);
            auto affinity_combobox = new QComboBox(&dialog);
            auto upgrade_level_spinbox = new QSpinBox(&dialog);
            upgrade_level_spinbox->setValue(attack_options.upgrade_level());
            upgrade_level_spinbox->setMinimum(0);
            auto two_handing_checkbox = new QCheckBox(&dialog);
            two_handing_checkbox->setChecked(attack_options.two_handing);
            auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

            std::set<
                std::reference_wrapper<const calculator::Weapon>,
                decltype([](const calculator::Weapon& a, const calculator::Weapon& b){
                    return a.affinity < b.affinity;
                })
            > possible_weapons{};

            auto set_weapon = [&](){
                auto weapon_it = std::ranges::find(
                    possible_weapons,
                    static_cast<calculator::Weapon::Affinity>(affinity_combobox->currentData().toInt()),
                    &calculator::Weapon::affinity
                );
                if (weapon_it == possible_weapons.end())
                    throw std::runtime_error("No weapon found for selected affinity");

                auto&& weapon = weapon_it->get();
                attack_options.weapon = weapon;
                upgrade_level_spinbox->setMaximum(weapon.max_upgrade_level());
            };

            // connect to weapon base name
            connect(base_name_combobox, &QComboBox::currentIndexChanged, &dialog, [&](int index){
                if (index < 0)
                    throw std::runtime_error("Invalid base name index");

                const QString text = base_name_combobox->itemText(index);

                auto first_invocation = possible_weapons.empty();
                possible_weapons.clear();
                possible_weapons.insert_range(
                    *this->active_weapon_data | std::views::filter([&](const calculator::Weapon& w) { return w.base_name.data() == text; })
                );
                if (possible_weapons.empty())
                    throw std::runtime_error(std::format("No weapons found for base name: {}", text.toStdString()));

                auto blocker = QSignalBlocker(affinity_combobox);
                affinity_combobox->clear();
                for (auto&& affinity : possible_weapons
                    | std::views::transform(&calculator::Weapon::affinity)
                )
                {
                    affinity_combobox->addItem(enum_to_display(affinity), std::to_underlying(affinity));
                    if(first_invocation && affinity == attack_options.weapon.get().affinity)
                        affinity_combobox->setCurrentIndex(affinity_combobox->count() - 1);
                }

                set_weapon();
            });

            // connect to weapon affinity
            connect(affinity_combobox, &QComboBox::currentIndexChanged, &dialog, set_weapon);

            // connect to upgrade level
            connect(upgrade_level_spinbox, &QSpinBox::valueChanged, &dialog, [&attack_options](int value){
                attack_options.upgrade_levels.at(attack_options.weapon.get().upgrade_level_index) = value;
            });

            // connect to two-handing
            connect(two_handing_checkbox, &QCheckBox::toggled, &dialog, [&attack_options](bool checked){
                attack_options.two_handing = checked;
            });

            connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            // create layout and add widgets
            QFormLayout *form = new QFormLayout(&dialog);
            form->addRow("Weapon Name:", base_name_combobox);
            form->addRow("Weapon Affinity:", affinity_combobox);
            form->addRow("Upgrade Level:", upgrade_level_spinbox);
            form->addRow("Two-Handing:", two_handing_checkbox);

            // character attributes spinboxes
            for (auto [i, attribute] : enumerators_of<calculator::Attribute>() | std::views::enumerate)
            {
                auto attribute_spinbox = new QSpinBox();
                attribute_spinbox->setValue(attack_options.stats.at(i));
                attribute_spinbox->setMinimum(0);
                attribute_spinbox->setMaximum(calculator::attribute_level_limit);

                form->addRow(enum_to_display(attribute) + ":", attribute_spinbox);

                connect(attribute_spinbox, &QSpinBox::valueChanged, &dialog, [&, i](int value) {
                    attack_options.stats.at(i) = value;
                });
            }

            form->addRow(buttons);

            // populate weapon base name
            auto blocker = QSignalBlocker(base_name_combobox);
            for (auto&& base_name : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<std::set>()
            )
            {
                if (base_name == attack_options.weapon.get().base_name)
                    blocker.unblock();
                base_name_combobox->addItem(QString::fromStdString(base_name));
                if (base_name == attack_options.weapon.get().base_name)
                    base_name_combobox->setCurrentIndex(base_name_combobox->count() - 1);
                
            }

            return dialog.exec() == QDialog::Accepted;
        }
        void edit_dataset_dialog(int wi)
        {
            auto attack_options = this->weapon_table->model->rows.at(wi).attack;

            auto dialog_was_accepted = this->edit_dataset_dialog_impl(attack_options, "Edit Dataset");

            if (dialog_was_accepted)
            {
                this->weapon_table->model->update_row(wi, std::move(attack_options));
                this->update_datasets(wi, 1);
            }
        }
        void add_new_dataset_dialog()
        {
            calculator::FullAttackOptions attack_options{ this->active_weapon_data->front(), {}, {} };

            auto dialog_was_accepted = this->edit_dataset_dialog_impl(attack_options, "Add New Dataset to Plot");

            if (dialog_was_accepted)
            {
                this->add_datasets({ attack_options });
            }
        }

    public:
        void add_datasets(const std::vector<std::reference_wrapper<const calculator::FullAttackOptions>>& attacks_options)
        {
            auto current_dataset_count = this->weapon_table->model->rows.size();

            this->weapon_table->model->add_rows(attacks_options
                | std::views::transform([](const calculator::FullAttackOptions& attacks_option) {
                    auto row = Row(calculator::FullAttackOptions(attacks_option));
                    std::get<sections::Color>(row)[0] = get_distinctive_color();
                    return row;
                })
            );

            auto current_column_count = this->model->columnCount();
            this->model->insertColumns(current_column_count, attacks_options.size() * 2);

            this->update_datasets(current_dataset_count, attacks_options.size());
        }

        void set_active_weapon_data(std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data)
        {
            this->active_weapon_data = std::move(active_weapon_data);
        }

        explicit PlotTab(QWidget *parent = nullptr) : QSplitter(Qt::Orientation::Vertical, parent)
        {
            // add this dependency so Qt6PrintSupport.dll is pulled in for KDChart, no idea why that's necessary
            QPrinter printer;
            Q_UNUSED(printer);

            // weapon table (model)
            this->weapon_table = new WeaponTable<Row>(true, "No datasets to display, add with right-click or from other tabs.", this);
            connect(this->weapon_table, &WeaponTable<Row>::remove_selection_from_plot, this, &PlotTab::remove_datasets);
            connect(this->weapon_table, &WeaponTable<Row>::row_color_changed, this, [this](int wi, QColor color){
                this->update_datasets(wi, 1);
            });
            connect(this->weapon_table, &WeaponTable<Row>::edit_row, this, &PlotTab::edit_dataset_dialog);
            connect(this->weapon_table, &WeaponTable<Row>::add_new_to_plot, this, &PlotTab::add_new_dataset_dialog);

            // plotting backend
            this->model = new QStandardItemModel(this);
            // this->model->setRowCount(1);
            this->model->setColumnCount(2); // dummy dataset for KDChart::Plotter to not crash
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

            // set dummy dataset invisible (KDChart::Plotter is buggy...)
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
            // this->chart = new KDChart::Chart(this);
            this->chart = new TrackingChart(this);
            upper_layout->addWidget(this->chart);
            this->chart->coordinatePlane()->replaceDiagram(this->plotter);
            this->chart->coordinatePlane()->globalGridAttributes().setSubGridVisible(false);
            this->chart->setTrackingModel(this->model);
            this->chart->setDatasetTrackable(0, false);
            this->chart->setTrackingModel(this->model);
            this->chart->setTrackingAxes(this->x_axis, this->y_axis);

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
            connect(&settings.plot_data_line_width, settings.plot_data_line_width.changed_member_pointer, this, set_data_line_width);

            auto dva = this->plotter->dataValueAttributes();
            auto marker = dva.markerAttributes();
            marker.setVisible(true);
            marker.setMarkerStyle(KDChart::MarkerAttributes::MarkerCircle);
            dva.setMarkerAttributes(marker);
            auto text = dva.textAttributes();
            text.setVisible(false);
            dva.setTextAttributes(text);
            this->plotter->setDataValueAttributes(dva);
            connect(&settings.plot_point_diameter, settings.plot_point_diameter.changed_member_pointer, this, &PlotTab::update_all_datasets);

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
            connect(&settings.plot_grid_line_width, settings.plot_grid_line_width.changed_member_pointer, this, set_grid_line_width);

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
            connect(&settings.plot_axis_line_width, settings.plot_axis_line_width.changed_member_pointer, this, set_axis_line_width);

            this->addWidget(this->weapon_table);
            this->setSizes({600, 400});
        }
    };
}

#include "plot_tab.moc"