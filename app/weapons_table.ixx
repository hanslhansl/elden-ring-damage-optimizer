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
    export QString format_float(double x) {
        std::string s = std::format("{:.3f}", x);

        // Remove trailing zeros
        while (!s.empty() && s.back() == '0')
            s.pop_back();

        // Remove trailing decimal point
        if (!s.empty() && s.back() == '.')
            s.pop_back();

        return QString::fromStdString(s);
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
        std::array<std::array<QVariant, 3>, 14> // spell scaling, total, ...
    >;
    QString column_name(int column)
    {
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
        if (6 <= column && column < 6 + enumerators_of<calculator::AttackPowerType>().size())
            return string_to_display(enum_to_string(enumerators_of<calculator::AttackPowerType>().at(column - 6)));;

        throw std::out_of_range("Invalid column index");
    }
    export void update_row_impl(Row& row, const calculator::AttackRating& attack_rating)
    {
        std::get<1>(row)[0][0] = format_float(attack_rating.spell_scaling);
        std::get<1>(row)[0][1] = attack_rating.spell_scaling;
        // std::get<1>(row)[0][2] = QColor(Qt::red);

        auto is_any_ineffective = false;
        for (auto&& [ap, is_ineffective, arr] : std::views::zip(
            attack_rating.attack_power,
            attack_rating.ineffective_attack_power_types,
            std::get<1>(row) | std::views::drop(2)))
        {
            arr[0] = format_float(ap[2]);
            arr[1] = ap[2];
            if (is_ineffective)
            {
                arr[2] = QColor(Qt::red);
                is_any_ineffective = true;
            }
        }
        std::get<1>(row)[1][0] = format_float(attack_rating.total_attack_power[2]);
        std::get<1>(row)[1][1] = attack_rating.total_attack_power[2];
        if (is_any_ineffective)
            std::get<1>(row)[1][2] = QColor(Qt::red);
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
        if (0 <= column && column < 4)
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                return std::get<0>(row)[column];
            }
        }
        else if (4 <= column && column < 6 + enumerators_of<calculator::AttackPowerType>().size())
        {
            auto i = column - 4;

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
        }

        return {};
    }

    export class RowModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        std::vector<Row> rows;

        explicit RowModel(QObject* parent = nullptr) : QAbstractTableModel(parent) { }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            return parent.isValid() ? 0 : static_cast<int>(this->rows.size());
        }
        int columnCount(const QModelIndex& parent = {}) const override
        {
            return parent.isValid() ? 0 : std::tuple_size_v<std::tuple_element_t<0, Row>> + std::tuple_size_v<std::tuple_element_t<1, Row>>;
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

        explicit RotatedHeaderView(Qt::Orientation orientation, Rotation rotation = Rotation::CounterClockwise, QWidget *parent = nullptr) : QHeaderView(orientation, parent), rotation(rotation) { }

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
        std::set<int> rotated_columns = std::views::iota(4, 4 + 2 + (int)enumerators_of<calculator::AttackPowerType>().size())
            | std::ranges::to<std::set>();
    };

    export class RowFilterModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:

        explicit RowFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) { }

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

        explicit WeaponTable(QWidget *parent = nullptr) : QTableView(parent) {

            this->proxy_model->setSortRole(Qt::UserRole);
            this->proxy_model->setSourceModel(this->model);
            this->setModel(this->proxy_model); // proxy_model model

            this->setSortingEnabled(true);

            this->setHorizontalHeader(this->header);
            this->header->setStretchLastSection(true);
            this->header->setSectionsMovable(true);
            this->header->setSectionsClickable(true);
        }
    };
}

#include "weapons_table.moc"