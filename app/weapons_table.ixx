module;
#include <QSortFilterProxyModel>
#include <tuple>
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
    export using Row = std::tuple<QString, QString, QString, QString>;

    // Table Model
    export class RowModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        std::vector<Row> rows;

        explicit RowModel(QObject* parent = nullptr) : QAbstractTableModel(parent) { }

        int rowCount(const QModelIndex& parent = {}) const override {
            return parent.isValid() ? 0 : static_cast<int>(this->rows.size());
        }
        int columnCount(const QModelIndex& parent = {}) const override {
            return parent.isValid() ? 0 : std::tuple_size_v<Row>;
        }

        QVariant data( const QModelIndex& index, int role) const override {
            if (!index.isValid())
                return {};

            const Row& r = this->rows[index.row()];

            // display text
            if (role == Qt::DisplayRole)
                return tuple_to_variant(r, index.column());

            // sorting data, returning raw values here prevents string sorting bugs: "100" < "20"
            if (role == Qt::UserRole)
                return tuple_to_variant(r, index.column());
            
            return {};
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
            if (role != Qt::DisplayRole)
                return {};

            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                    case 0:
                        return "name";
                    case 1:
                        return "affinity";
                    case 2:
                        return "type";
                    case 3:
                        return "base game/dlc";
                    default:
                        return {};
                }
            }

            return {};
        }

        void set_rows(std::vector<Row> rows) {
            this->beginResetModel();
            this->rows = std::move(rows);
            this->endResetModel();
        }

        // Sorting
        void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override {
            emit layoutAboutToBeChanged();

            if (order == Qt::AscendingOrder)
                std::ranges::sort(this->rows, [&](const Row& a, const Row& b) {
                    return visit_tuple(
                        [](const auto& a_val, const auto& b_val) { return a_val < b_val; },
                        column,
                        a,
                        b
                    );
                });
            else
                std::ranges::sort(this->rows, [&](const Row& a, const Row& b) {
                    return visit_tuple(
                        [](const auto& a_val, const auto& b_val) { return a_val > b_val; },
                        column,
                        a,
                        b
                    );
                });

            emit layoutChanged();
        }

        void notifyColumnChanged(int column) {
            if (rows.empty())
                return;

            emit dataChanged(index(0, column), index(rowCount() - 1, column));
        }
        void notifyAllChanged() {
            if (rows.empty())
                return;

            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    };



    //============================================================
    // Filtering
    //============================================================

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

        void setTextFilter(QString text)
        {
            this->text_ = std::move(text);
            invalidateFilter();
        }

        void setMinimumValue(int value)
        {
            this->minimumValue_ = value;
            invalidateFilter();
        }

    protected:

        bool filterAcceptsRow(int row, const QModelIndex& parent) const override
        {
            QModelIndex idIndex =  sourceModel()->index(row,  static_cast<int>(Column::Id),  parent);

            QModelIndex valueIndex = sourceModel()->index( row,  static_cast<int>(Column::Value),  parent);

            QString id = sourceModel()->data(idIndex) .toString();

            int value = sourceModel()->data(valueIndex) .toInt();

            if (!this->text_.isEmpty() && !id.contains(this->text_, Qt::CaseInsensitive))
                return false;

            if (value < this->minimumValue_)
                return false;

            return true;
        }


    private:

        QString text_;

        int minimumValue_ = std::numeric_limits<int>::min();
    };
}

#include "weapons_table.moc"