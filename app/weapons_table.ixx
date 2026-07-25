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

    // Table Model
    export class RowModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        std::vector<calculator::AttackRating> rows;

        size_t counter = 0;

        explicit RowModel(QObject* parent = nullptr) : QAbstractTableModel(parent) { }

        int rowCount(const QModelIndex& parent = {}) const override {
            return parent.isValid() ? 0 : static_cast<int>(this->rows.size());
        }
        int columnCount(const QModelIndex& parent = {}) const override {
            return parent.isValid() ? 0 : 17;
        }

        QVariant data(const QModelIndex& index, int role) const override {
            if (!index.isValid())
                return {};

            const_cast<RowModel*>(this)->counter++;

            auto column = index.column();
            const auto& attack_rating = this->rows[index.row()];
            auto&& weapon = attack_rating.weapon.get();
            auto&& attack_options = attack_rating.attack_options;

            auto special_format_float = [role](double x, bool is_ineffective) -> QVariant  {
                // display data
                if (role == Qt::DisplayRole)
                {
                    if (x == 0)
                        return {};
                    return format_float(x);
                }

                // sorting data, returning raw values here prevents string sorting bugs: "100" < "20"
                if (role == Qt::UserRole)
                {
                    return x;
                }

                if (role == Qt::ForegroundRole)
                {
                    if (is_ineffective)
                        return QColor(Qt::red);
                    return {};
                }

                throw std::out_of_range("Invalid role");
            };

            if (role == Qt::DisplayRole || role == Qt::UserRole || role == Qt::ForegroundRole)
            {
                int i = 0;
                if (column == i++)
                    return string_to_display(weapon.qualified_name(attack_options.upgrade_levels.at(weapon.upgrade_level_index)));
                if (column == i++)
                    return string_to_display(enum_to_string(weapon.affinity));
                if (column == i++)
                    return string_to_display(enum_to_string(weapon.type));

                if (column == i++)
                    return special_format_float(attack_rating.total_attack_power[2], false);
                if (i <= column && column < i + enumerators_of<calculator::AttackPowerType>().size())
                    return special_format_float(attack_rating.attack_power.at(column - i)[2], attack_rating.ineffective_attack_power_types[column - i]);
                i += enumerators_of<calculator::AttackPowerType>().size();

                if (column == i++)
                    return string_to_display(weapon.dlc ? "dlc" : "base game");

                throw std::out_of_range("Invalid column index");
            }

            return {};
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
            if (role != Qt::DisplayRole)
                return {};

            auto column = section;
            if (orientation == Qt::Horizontal)
            {
                int i = 0;
                if (column == i++)
                    return string_to_display("name");
                if (column == i++)
                    return string_to_display("affinity");
                if (column == i++)
                    return string_to_display("type");

                if (column == i++)
                    return string_to_display("total");
                if (i <= column && column < i + enumerators_of<calculator::AttackPowerType>().size())
                    return string_to_display(enum_to_string(enumerators_of<calculator::AttackPowerType>().at(column - i)));
                i += enumerators_of<calculator::AttackPowerType>().size();

                if (column == i++)
                    return string_to_display("base game/dlc");

                throw std::out_of_range("Invalid column index");
            }

            return {};
        }

        void set_rows(std::vector<calculator::AttackRating>&& rows) {
            this->beginResetModel();
            this->rows = std::move(rows);
            this->endResetModel();
        }

        void notifyAllChanged() {
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
        std::set<int> rotated_columns = std::views::iota(3, 3 + 1 + (int)enumerators_of<calculator::AttackPowerType>().size())
            | std::ranges::to<std::set>();
    };


    // Filtering
    export enum class Column
    {
        Id,
        Value,
        Count
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
            this->setModel(this->proxy_model);

            this->setSortingEnabled(true);

            this->setHorizontalHeader(this->header);
            this->header->setStretchLastSection(true);
            this->header->setSectionsMovable(true);
            this->header->setSectionsClickable(true);
        }
    };

}

#include "weapons_table.moc"