module;
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPainter.h>
#include <qobject.h>
#include <qtableview.h>
#include <tuple>
export module erdo.ui.weapons_table;

import std;
import erdo;


namespace erdo::ui
{
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

    struct __tuple_base
    {

    };
    template<typename T>
    struct _tuple_base : T, __tuple_base
    {
        using tuple_base = T;
        using tuple_base::tuple_base;
    };
}

template<std::derived_from<erdo::ui::__tuple_base> T>
struct std::tuple_size<T> : std::tuple_size<typename T::tuple_base> {};
template<std::size_t I, std::derived_from<erdo::ui::__tuple_base> T>
struct std::tuple_element<I, T> : std::tuple_element<I, typename T::tuple_base> {};

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

    template<typename T>
    auto format_number(T x)
    {
        if (x == 0)
            return QString("\u2012");
        if constexpr (std::integral<T>)
            return QString::number(x);
        return format_float(x);
    };
    auto foreground_color(bool is_ineffective)
    {
        return is_ineffective ? QColor(Qt::red) : QColor(Qt::black);
    }
 
    struct TextColumns : _tuple_base<std::array<QVariant, 4>>
    {
        using _tuple_base::_tuple_base;

        inline const static std::vector<QString> column_names {
            string_to_display("name"),
            string_to_display("affinity"),
            string_to_display("type"),
            string_to_display("base game/dlc")
        };

        explicit TextColumns(const calculator::AttackRating& attack_rating)
        {
            auto&& weapon = attack_rating.weapon.get();
            auto&& attack_options = attack_rating.attack_options;

            (*this)[0] = string_to_display(weapon.qualified_name(attack_options.upgrade_levels[weapon.upgrade_level_index]));
            (*this)[1] = string_to_display(enum_to_string(weapon.affinity));
            (*this)[2] = string_to_display(enum_to_string(weapon.type));
            (*this)[3] = string_to_display(weapon.dlc ? "dlc" : "base game");
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            auto&& weapon = attack_rating.weapon.get();
            auto&& attack_options = attack_rating.attack_options;

            (*this)[0] = string_to_display(weapon.qualified_name(attack_options.upgrade_levels.at(weapon.upgrade_level_index)));
        }

        QVariant data(int column, int role) const
        {
            if (role == Qt::DisplayRole || role == Qt::UserRole)
            {
                return this->at(column);
            }
            return {};
        }
    };

    template<std::size_t I>
    struct DataColumns : _tuple_base<std::array<std::array<QVariant, 3>, I>>
    {
        using _tuple_base = _tuple_base<std::array<std::array<QVariant, 3>, I>>;
        using _tuple_base::_tuple_base;

        QVariant data(int column, int role) const
        {
            if (role == Qt::DisplayRole)
                return this->at(column)[0];
            
            if (role == Qt::UserRole)
                return this->at(column)[1];
            
            if (role == Qt::ForegroundRole)
                return this->at(column)[2];
            
            if (role == Qt::TextAlignmentRole)
                return QVariant::fromValue(Qt::AlignHCenter | Qt::AlignVCenter);
            
            return {};
        }
    };

    struct TotalAttackPower : DataColumns<1>
    {
        inline const static std::vector<QString> column_names = { string_to_display("total") };

        explicit TotalAttackPower(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            (*this)[0][0] = format_number(attack_rating.total_attack_power[1]);
            (*this)[0][1] = attack_rating.total_attack_power[1];
            (*this)[0][2] = foreground_color(true);
        }
    };

    struct SpellScaling : DataColumns<1>
    {
        inline const static std::vector<QString> column_names = { string_to_display("spell scaling") };

        explicit SpellScaling(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            (*this)[0][0] = format_number(attack_rating.spell_scaling * 100);
            (*this)[0][1] = attack_rating.spell_scaling * 100;
            (*this)[0][2] = foreground_color(true);
        }
    };

    template<typename E>
    struct EnumDataColumns : DataColumns<enumerators_of<E>().size()>
    {
        inline const static std::vector<QString> column_names = enumerators_of<E>()
            | std::views::transform([](E e){ return string_to_display(enum_to_string(e)); })
            | std::ranges::to<std::vector>();
    };

    struct AttackPowers : EnumDataColumns<calculator::DamageType>
    {
        explicit AttackPowers(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            for (auto&& [ap, is_ineffective, arr] : std::views::zip(
                attack_rating.attack_powers | std::views::take(enumerators_of<calculator::DamageType>().size()),
                attack_rating.ineffective_attack_power_types | std::views::take(enumerators_of<calculator::DamageType>().size()),
                *this))
            {
                arr[0] = format_number(ap[1]);
                arr[1] = ap[1];
                arr[2] = foreground_color(is_ineffective);
            }
        }
    };

    struct StatusEffects : EnumDataColumns<calculator::StatusEffectType>
    {
        explicit StatusEffects(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            for (auto&& [ap, is_ineffective, arr] : std::views::zip(
                attack_rating.attack_powers | std::views::drop(enumerators_of<calculator::DamageType>().size()),
                attack_rating.ineffective_attack_power_types | std::views::drop(enumerators_of<calculator::DamageType>().size()),
                *this))
            {
                arr[0] = format_number(ap[1]);
                arr[1] = ap[1];
                arr[2] = foreground_color(is_ineffective);
            }
        }
    };

    struct AttributeScalings : EnumDataColumns<calculator::RelevantAttribute>
    {
        explicit AttributeScalings(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            auto&& weapon = attack_rating.weapon.get();

            for (auto&& [attribute_scaling, arr] : std::views::zip(
                attack_rating.attribute_scalings,
                *this))
            {
                auto scaling_tier = weapon.calculate_scaling_tier(attribute_scaling);
                if (scaling_tier.empty())
                    arr[0] = format_number(attribute_scaling * 100);
                else
                    arr[0] = format_number(attribute_scaling * 100) + " (" + QString::fromStdString(scaling_tier) + ")";
                arr[1] = attribute_scaling * 100;
                arr[2] = foreground_color(false);
            }
        }
    };

    struct Requirements : EnumDataColumns<calculator::RelevantAttribute>
    {
        explicit Requirements(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            auto&& weapon = attack_rating.weapon.get();

            for (auto&& [requirement, is_ineffective, arr] : std::views::zip(
                weapon.requirements,
                attack_rating.ineffective_attributes,
                *this))
            {
                arr[0] = format_number(requirement);
                arr[1] = requirement;
                arr[2] = foreground_color(is_ineffective);
            }
        }
    };

    struct Stats : EnumDataColumns<calculator::RelevantAttribute>
    {
        explicit Stats(const calculator::AttackRating& attack_rating)
        {
            this->update(attack_rating);
        }

        void update(const calculator::AttackRating& attack_rating)
        {
            for (auto&& [stat, is_ineffective, arr] : std::views::zip(
                attack_rating.stats,
                attack_rating.ineffective_attributes,
                *this))
            {
                arr[0] = stat;
                arr[1] = stat;
                arr[2] = foreground_color(is_ineffective);
            }
        }
    };

    export template<typename...Args>
    struct BasicRow : _tuple_base<std::tuple<Args...>>
    {
        using _tuple_base = _tuple_base<std::tuple<Args...>>;
        using _tuple_base::_tuple_base;

        static constexpr std::array element_sizes = { std::tuple_size_v<Args>... };
        static constexpr std::array cumulative_element_sizes = []() {
            std::array<std::size_t, sizeof...(Args)> result{};
            std::partial_sum(element_sizes.begin(), element_sizes.end(), result.begin());
            return result;
        }();
        static constexpr std::array element_index_offset = []() {
            std::array<std::size_t, sizeof...(Args)> result{};
            std::ranges::copy(cumulative_element_sizes | std::views::take(sizeof...(Args) - 1), result.begin() + 1);
            return result;
        }();
        static constexpr std::size_t total_size = std::accumulate(element_sizes.begin(), element_sizes.end(), 0);
        inline const static std::vector<QString> column_names = [](){
            std::vector<QString> result{};
            result.reserve(total_size);
            (result.append_range(Args::column_names), ...);
            return result;
        }();

        explicit BasicRow(const calculator::AttackRating& attack_rating) : _tuple_base(Args(attack_rating)...) { }

        void update(const calculator::AttackRating& attack_rating)
        {
            std::apply(
                [&](auto&&...args) {
                    (std::forward<decltype(args)>(args).update(attack_rating),...);
                },
                *this
            );
        }

        QVariant data(int column, int role) const
        {
            return [&]<std::size_t I = 0>(this auto&& self, std::size_t index)->QVariant {
                if constexpr (I < std::tuple_size_v<BasicRow>)
                {
                    if (index < element_sizes[I])
                        return std::get<I>(*this).data(index, role);
                    else
                        return self.template operator()<I + 1>(index - element_sizes[I]);
                }
                throw std::out_of_range("index out of range");
            }(column);
        }
    };

    export using Row = BasicRow<
        TextColumns,
        SpellScaling,
        AttackPowers,
        TotalAttackPower,
        StatusEffects,
        AttributeScalings,
        Requirements,
        Stats
    >;

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
            return Row::total_size;
        }

        QVariant data(const QModelIndex& index, int role) const override
        {
            if (!index.isValid())
                return {};

            return this->rows[index.row()].data(index.column(), role);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            if (role != Qt::DisplayRole)
                return {};

            if (orientation == Qt::Horizontal)
                return Row::column_names.at(section);

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
        enum class Rotation
        {
            Clockwise,
            CounterClockwise
        };

        explicit RotatedHeaderView(Qt::Orientation orientation, Rotation rotation = Rotation::CounterClockwise, QWidget *parent = nullptr)
            : QHeaderView(orientation, parent), rotation(rotation)
        {
            // this->setSectionsMovable(true);
            this->setSectionsClickable(true);
        }

        template <typename PaintDevice>
        static void draw_column_group_separators(PaintDevice *device, const QHeaderView *header)
        {
            QPainter painter(device);

            const QRect bounds = device->rect();

            painter.save();

            painter.setClipRect(bounds);
            QPen pen(Qt::black, 1);
            pen.setCosmetic(true);
            painter.setPen(pen);

            for (auto logicalColumn : Row::cumulative_element_sizes)
            {
                if (header->isSectionHidden(logicalColumn))
                    continue;

                const int x = header->sectionViewportPosition(logicalColumn);

                painter.drawLine(x, bounds.top(), x, bounds.bottom());
            }

            painter.restore();
        }

    protected:
        void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
            painter->save();

            bool rotate = std::ranges::contains(rotated_columns, logicalIndex);

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

            // Width becomes height after rotation
            if (std::ranges::contains(rotated_columns, logicalIndex))
                return QSize(
                    size.height(),
                    size.width()
                );

            return size;
        }

        void paintEvent(QPaintEvent *e) override
        {
            QHeaderView::paintEvent(e);

            RotatedHeaderView::draw_column_group_separators(this->viewport(), this);
        }

    private:
        Rotation rotation;
        static const inline std::vector<std::size_t> rotated_columns = []<std::size_t I = 0>(this auto&& self, std::vector<std::size_t> indices = {} ) -> std::vector<std::size_t> {
            if constexpr (I == std::tuple_size_v<Row>)
            {
                return indices;
            }
            else
            {
                if constexpr (std::derived_from<std::tuple_element_t<I, Row>, DataColumns<Row::element_sizes[I]>>)
                {
                    indices.append_range(
                        std::views::iota(
                            Row::element_index_offset[I],
                            Row::element_index_offset[I] + Row::element_sizes[I]
                        )
                    );
                }

                return self.template operator()<I + 1>(std::move(indices));
            }
        }();
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
            // this->header->hideSection(3);
        }

        void resize_columns_to_contents()
        {
            const int columns = this->model->columnCount();

            this->header->setSectionResizeMode(QHeaderView::ResizeToContents);
            this->resizeColumnsToContents();

            QVector<int> widths(columns);
            int total = 0;
            QVector<int> visibleColumns;

            for (int c = 0; c < columns; ++c)
            {
                if (this->header->isSectionHidden(c))
                    continue;

                widths[c] = this->header->sectionSize(c);
                total += widths[c];
                visibleColumns.append(c);
            }

            this->header->setSectionResizeMode(QHeaderView::Interactive);

            if (total <= 0 || visibleColumns.isEmpty())
                return;

            const int available = this->viewport()->width();

            if (total < available)
            {
                const double factor = double(available) / total;

                int used = 0;

                for (int i = 0; i < visibleColumns.size() - 1; ++i) {
                    int c = visibleColumns[i];
                    int w = qRound(widths[c] * factor);

                    this->header->resizeSection(c, w);
                    used += w;
                }

                // Last visible column gets the remainder
                int last = visibleColumns.back();
                this->header->resizeSection(last, available - used);
            }
            else
            {
                for (int c : visibleColumns)
                    this->header->resizeSection(c, widths[c]);
            }
        }

        void hide_section(std::size_t section)
        {
            [&]<std::size_t I = 0>(this auto&& self) -> void {
                if constexpr (I == std::tuple_size_v<Row>)
                {
                    throw std::out_of_range("section index out of range");
                }
                else
                {
                    if (section == I)
                    {
                        for (auto && index : std::views::iota(
                            Row::element_index_offset[I],
                            Row::element_index_offset[I] + Row::element_sizes[I]
                        ))
                            this->header->hideSection(static_cast<int>(index));
                        return;
                    }
                    return self.template operator()<I + 1>();
                }
            }();
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QTableView::paintEvent(event);

            RotatedHeaderView::draw_column_group_separators(this->viewport(), this->horizontalHeader());
        }
    };
}