module;
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPainter.h>
#include <qobject.h>
#include <qtableview.h>
export module erdo.ui.weapons_table;

import std;
import erdo;

template<typename F, std::size_t I, std::size_t...Is, typename...Tuples>
decltype(auto) visit_tuple_impl(F&& f, std::size_t index, std::index_sequence<I, Is...>, Tuples&&...tuples) {
    if (index == I)
        return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuples>(tuples))...);
    
    if constexpr (sizeof...(Is) > 0)
        return visit_tuple_impl(std::forward<F>(f), index, std::index_sequence<Is...>{}, std::forward<Tuples>(tuples)...);
    else
        throw std::out_of_range("Index out of range in visit_tuple");
}
template<typename F, typename...Ts>
    requires ((std::tuple_size_v<std::remove_cvref_t<Ts...[0]>> == std::tuple_size_v<std::remove_cvref_t<Ts>>) && ...)
decltype(auto) visit_tuple(F&& f, std::size_t index, Ts&&...ts) {
    return visit_tuple_impl(
        std::forward<F>(f),
        index,
        std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Ts...[0]>>>{},
        std::forward<Ts>(ts)...
    );
}

template <typename...Args>
QVariant tuple_to_variant(const std::tuple<Args...>& tuple, std::size_t index) {
    return visit_tuple(
        [](const auto& arg){ return QVariant::fromValue(arg); },
        index,
        tuple
    );
}


namespace erdo::ui
{
    export QString format_float(double x)
    {
        auto s = QString::number(x, 'f', 3);

        // Remove trailing zeros
        while (s.endsWith('0'))
            s.chop(1);

        // Remove trailing decimal point
        if (s.endsWith('.'))
            s.chop(1);

        return s;
    }

    export QString string_to_display(const QString& str) {
        QStringList words = str.split(QRegularExpression("[_\\s]+"), Qt::SkipEmptyParts);

        for (QString &word : words) {
            word = word.toLower();
            // if (!word.isEmpty())
            //     word[0] = word[0].toUpper();
        }

        return words.join(' ');
    }
    export QString string_to_display(const char* str) {
        return string_to_display(QString(str));
    }
    export QString string_to_display(const std::string& str) {
        return string_to_display(QString::fromStdString(str));
    }
    export QString string_to_display(std::string_view str) {
        return string_to_display(QString::fromStdString(std::string(str)));
    }


    export using Row = std::tuple<
        std::array<QVariant, 4>,    // name, affinity, type, base game/dlc
        std::array<std::array<QVariant, 3>, 2 + enumerators_of<calculator::AttackPowerType>().size() + enumerators_of<calculator::RelevantAttribute>().size() * 3> // spell scaling, total, ...
    >;
    QString column_name(int column)
    {
        static constexpr auto apt_size = enumerators_of<calculator::AttackPowerType>().size();
        static constexpr auto attr_size = enumerators_of<calculator::RelevantAttribute>().size();

        if (column == 0)
            return string_to_display("name");
        if (column == 1)
            return string_to_display("affinity");
        if (column == 2)
            return string_to_display("type");
        if (column == 3)
            return string_to_display("base game/dlc");

        if (column == 4)
            return string_to_display("spell scaling");

        if (column == 5)
            return string_to_display("total");
        if (6 <= column && column < 6 + apt_size)
            return string_to_display(enum_to_string(enumerators_of<calculator::AttackPowerType>().at(column - 6)));
        
        if (6 + apt_size <= column && column < 6 + apt_size + attr_size)
            return string_to_display(enum_to_string(enumerators_of<calculator::RelevantAttribute>().at(column - 6 - apt_size)));

        if (6 + apt_size + attr_size <= column && column < 6 + apt_size + attr_size * 2)
            return string_to_display(enum_to_string(enumerators_of<calculator::RelevantAttribute>().at(column - 6 - apt_size - attr_size)));
        
        if (6 + apt_size + attr_size * 2 <= column && column < 6 + apt_size + attr_size * 3)
            return string_to_display(enum_to_string(enumerators_of<calculator::RelevantAttribute>().at(column - 6 - apt_size - attr_size * 2)));

        throw std::out_of_range(std::format("Invalid column index: {}", column));
    }
    export void update_row_impl(Row& row, const calculator::AttackRating& attack_rating)
    {
        static auto format_number = []<typename T>(T x){
            if (x == 0)
                return QString("\u2012");
            if constexpr (std::integral<T>)
                return QString::number(x);
            return format_float(x);
        };

        static auto foreground_color = [](bool is_ineffective) {
            return is_ineffective ? QColor(Qt::red) : QColor(Qt::black);
        };

        auto&& weapon = attack_rating.weapon.get();

        // spell scaling
        std::get<1>(row)[0][0] = format_number(attack_rating.spell_scaling * 100);
        std::get<1>(row)[0][1] = attack_rating.spell_scaling * 100;
        // std::get<1>(row)[0][2] = QColor(Qt::red);

        // total attack power
        std::get<1>(row)[1][0] = format_number(attack_rating.total_attack_power[2]);
        std::get<1>(row)[1][1] = attack_rating.total_attack_power[2];
        std::get<1>(row)[1][2] = foreground_color(true);

        // attack powers
        for (auto&& [ap, is_ineffective, arr] : std::views::zip(
            attack_rating.attack_powers,
            attack_rating.ineffective_attack_power_types,
            std::get<1>(row) | std::views::drop(2)))
        {
            arr[0] = format_number(ap[2]);
            arr[1] = ap[2];
            arr[2] = foreground_color(is_ineffective);
        }

        // attribute scalings
        for (auto&& [attribute_scaling, arr] : std::views::zip(
            attack_rating.attribute_scalings,
            std::get<1>(row) | std::views::drop(2 + enumerators_of<calculator::AttackPowerType>().size())))
        {
            if (attribute_scaling == 0)
                arr[0] = format_number(attribute_scaling * 100);
            else
                arr[0] = format_number(attribute_scaling * 100) + " (" + QString::fromStdString(weapon.scaling_tier(attribute_scaling)) + ")";
            arr[1] = attribute_scaling * 100;
            arr[2] = foreground_color(false);
        }

        // attribute requirements
        for (auto&& [requirement, is_ineffective, arr] : std::views::zip(
            weapon.requirements,
            attack_rating.ineffective_attributes,
            std::get<1>(row) | std::views::drop(2 + enumerators_of<calculator::AttackPowerType>().size() + enumerators_of<calculator::RelevantAttribute>().size())))
        {
            arr[0] = format_number(requirement);
            arr[1] = requirement;
            arr[2] = foreground_color(is_ineffective);
        }

        // stats
        for (auto&& [stat, is_ineffective, arr] : std::views::zip(
            attack_rating.stats,
            attack_rating.ineffective_attributes,
            std::get<1>(row) | std::views::drop(2 + enumerators_of<calculator::AttackPowerType>().size() + enumerators_of<calculator::RelevantAttribute>().size() * 2)))
        {
            arr[0] = stat;
            arr[1] = stat;
            arr[2] = foreground_color(is_ineffective);
        }

        // static_assert(false, "add attribute scaling letter and number as well as list of ineffective attributes.");
    }
    export void update_row(Row& row, const calculator::AttackRating& attack_rating)
    {
        auto&& weapon = attack_rating.weapon.get();
        auto&& attack_options = attack_rating.attack_options;
        auto&& stats = attack_rating.stats;

        std::get<0>(row)[0] = string_to_display(weapon.qualified_name(attack_options.upgrade_levels.at(weapon.upgrade_level_index)));

        update_row_impl(row, attack_rating);
    }
    export Row build_row(const calculator::AttackRating& attack_rating)
    {
        auto&& weapon = attack_rating.weapon.get();
        auto&& attack_options = attack_rating.attack_options;
        auto&& stats = attack_rating.stats;

        Row row{
            {
                string_to_display(weapon.qualified_name(attack_options.upgrade_levels.at(weapon.upgrade_level_index))),
                string_to_display(enum_to_string(weapon.affinity)),
                string_to_display(enum_to_string(weapon.type)),
                string_to_display(weapon.dlc ? "dlc" : "base game")
            },
            {
                
            }
        };

        update_row_impl(row, attack_rating);

        return row;
    }
    QVariant row_data(const Row& row, int column, int role)
    {
        static constexpr auto size_0 = std::tuple_size_v<std::tuple_element_t<0, Row>>;
        static constexpr auto size_1 = std::tuple_size_v<std::tuple_element_t<1, Row>>;

        if (0 <= column && column < size_0)
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                return std::get<0>(row)[column];
            }
            return {};
        }
        else if (size_0 <= column && column < size_0 + size_1)
        {
            auto i = column - size_0;

            if (role == Qt::DisplayRole)
            {
                return std::get<1>(row)[i][0];
            }
            if (role == Qt::UserRole)
            {
                return std::get<1>(row)[i][1];
            }
            if (role == Qt::ForegroundRole)
            {
                return std::get<1>(row)[i][2];
            }
            if (role == Qt::TextAlignmentRole)
            {
                return QVariant::fromValue(Qt::AlignHCenter | Qt::AlignVCenter);
            }
            return {};
        }

        throw std::out_of_range(std::format("Invalid column index: {}", column));
    }

    export struct RowModel : QAbstractTableModel
    {
        std::vector<Row> rows;

        explicit RowModel(QObject* parent = nullptr) : QAbstractTableModel(parent) { }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            return parent.isValid() ? 0 : static_cast<int>(this->rows.size());
        }
        int columnCount(const QModelIndex& parent = {}) const override
        {
            return std::tuple_size_v<std::tuple_element_t<0, Row>> + std::tuple_size_v<std::tuple_element_t<1, Row>>;
        }

        QVariant data(const QModelIndex& index, int role) const override
        {
            if (!index.isValid())
                return {};

            return row_data(this->rows[index.row()], index.column(), role);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            if (role != Qt::DisplayRole)
                return {};

            auto column = section;
            if (orientation == Qt::Horizontal)
                return column_name(column);

            return {};
        }

        void set_rows(std::vector<Row>&& rows)
        {
            this->beginResetModel();
            this->rows = std::move(rows);
            this->endResetModel();
        }

        void notifyAllChanged()
        {
            if (rows.empty())
                return;

            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    };

    export class RotatedHeaderView : public QHeaderView
    {
    public:
        enum class Rotation {
            Clockwise,
            CounterClockwise
        };

        explicit RotatedHeaderView(Qt::Orientation orientation, Rotation rotation = Rotation::CounterClockwise, QWidget *parent = nullptr)
            : QHeaderView(orientation, parent), rotation(rotation)
        {
            // this->setSectionResizeMode(QHeaderView::ResizeToContents);
            // this->setStretchLastSection(true);
            // this->setSectionsMovable(true);
            this->setSectionsClickable(true);
        }

    protected:
        void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
            painter->save();

            bool rotate = rotated_columns.contains(logicalIndex);

            if (!rotate) {
                // Default Qt rendering
                QHeaderView::paintSection(painter, rect, logicalIndex);
                painter->restore();
                return;
            }

            // Draw background/frame
            QStyleOptionHeader option;
            initStyleOption(&option);
            option.rect = rect;
            option.text.clear();

            style()->drawControl(
                QStyle::CE_Header,
                &option,
                painter,
                this);

            QString text = model()->headerData(
                logicalIndex,
                orientation(),
                Qt::DisplayRole).toString();

            painter->setPen(option.palette.color(
                QPalette::Text));

            if (rotation == Rotation::CounterClockwise) {
                painter->translate(rect.left(), rect.bottom());
                painter->rotate(-90);
            }
            else {
                painter->translate(rect.right(), rect.top());
                painter->rotate(90);
            }

            QRect textRect(
                0,
                0,
                rect.height(),
                rect.width());

            painter->drawText(
                textRect,
                Qt::AlignCenter,
                text);

            painter->restore();
        }

        QSize sectionSizeFromContents(int logicalIndex) const override {
            QSize size = QHeaderView::sectionSizeFromContents(logicalIndex);

            if (rotated_columns.contains(logicalIndex)) {
                // Width becomes height after rotation
                return QSize(
                    40,                  // narrow column header width
                    size.width() + 20);  // rotated text height
            }

            return size;
        }

    private:
        Rotation rotation;
        std::set<int> rotated_columns = std::views::iota(4, 4 + 2 + (int)enumerators_of<calculator::AttackPowerType>().size() + (int)enumerators_of<calculator::RelevantAttribute>().size()*3)
            | std::ranges::to<std::set>();
    };

    export class RowFilterModel : public QSortFilterProxyModel
    {
    public:

        explicit RowFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent)
        {
            this->setSortRole(Qt::UserRole);
        }

        // void setTextFilter(QString text)
        // {
        //     this->text_ = std::move(text);
        //     invalidateFilter();
        // }

        // void setMinimumValue(int value)
        // {
        //     this->minimumValue_ = value;
        //     invalidateFilter();
        // }

    protected:

        // bool filterAcceptsRow(int row, const QModelIndex& parent) const override
        // {
        //     QModelIndex idIndex =  sourceModel()->index(row,  static_cast<int>(Column::Id),  parent);

        //     QModelIndex valueIndex = sourceModel()->index( row,  static_cast<int>(Column::Value),  parent);

        //     QString id = sourceModel()->data(idIndex) .toString();

        //     int value = sourceModel()->data(valueIndex) .toInt();

        //     if (!this->text_.isEmpty() && !id.contains(this->text_, Qt::CaseInsensitive))
        //         return false;

        //     if (value < this->minimumValue_)
        //         return false;

        //     return true;
        // }


    private:

        // QString text_;

        // int minimumValue_ = std::numeric_limits<int>::min();
    };

    export class WeaponTable : public QTableView
    {
    public:

        RowModel* model = new RowModel(this);
        RowFilterModel* proxy_model = new RowFilterModel(this);
        RotatedHeaderView* header = new RotatedHeaderView(Qt::Horizontal, RotatedHeaderView::Rotation::Clockwise, this);

        explicit WeaponTable(QWidget *parent = nullptr) : QTableView(parent)
        {
            this->proxy_model->setSourceModel(this->model);
            this->setModel(this->proxy_model); // proxy_model model

            this->setHorizontalHeader(this->header);

            this->setSortingEnabled(true);
            this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

            // this->header->moveSection(this->header->visualIndex(3), 17);
        }

        void resize_columns_to_contents()
        {
            this->resizeColumnsToContents();
            // this->header->setSectionResizeMode(2, QHeaderView::Stretch);
            // this->header->setSectionResizeMode(3, QHeaderView::Stretch);

            // this->resizeColumnsToContents();
            // int extra = this->viewport()->width() - this->horizontalHeader()->length();
            // if (extra > 0)
            //     this->setColumnWidth(2, this->columnWidth(2) + extra);
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            this->QTableView::paintEvent(event);

            QPainter painter(this->viewport());
            QPen pen(Qt::black, 1);
            pen.setCosmetic(true);
            painter.setPen(pen);

            // Draw before columns 2 and 5
            for (auto col : {
                4,
                5,
                6 + (int)enumerators_of<calculator::DamageType>().size(),
                6 + (int)enumerators_of<calculator::AttackPowerType>().size(),
                6 + (int)enumerators_of<calculator::AttackPowerType>().size() + (int)enumerators_of<calculator::RelevantAttribute>().size(),
                6 + (int)enumerators_of<calculator::AttackPowerType>().size() + (int)enumerators_of<calculator::RelevantAttribute>().size() * 2,
            })
            {
                int x = this->columnViewportPosition(col) ; // + this->columnWidth(col)
                painter.drawLine(x, 0, x, this->viewport()->height());
            }
        }
    };
}