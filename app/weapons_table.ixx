module;
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPainter>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QDesktopServices>
#include <QUrl>
export module erdo.ui.weapons_table;

import std;
import erdo;


namespace erdo::ui
{
    template<typename T, typename Tuple, std::size_t... Is>
    constexpr std::size_t tuple_index_impl(std::index_sequence<Is...>)
    {
        constexpr bool matches[] = { std::same_as<T, std::tuple_element_t<Is, Tuple>>... };

        for (std::size_t i = 0; i < sizeof...(Is); ++i)
            if (matches[i])
                return i;

        throw std::out_of_range("Type not found in tuple");
    }
    template<typename T, typename Tuple>
    constexpr std::size_t tuple_index_v = tuple_index_impl<T, Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

    struct __tuple_base { };
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

    export QString string_to_display(const QString& str)
    {
        QStringList words = str.split(QRegularExpression("[_ ]+"), Qt::SkipEmptyParts);

        for (QString &word : words)
            word = word.toLower();

        return words.join(' ');
    }
    export QString string_to_display(const char* str)
    {
        return string_to_display(QString(str));
    }
    export QString string_to_display(const std::string& str)
    {
        return string_to_display(QString::fromStdString(str));
    }
    export QString string_to_display(std::string_view str)
    {
        return string_to_display(std::string(str));
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
 
    namespace sections
    {
        template<typename T>
        struct SectionBase : _tuple_base<T>
        {
            using _tuple_base<T>::_tuple_base;

            static constexpr bool draw_header_labels_rotated = false;
            static constexpr bool has_header_section_title = false;
            static constexpr bool draw_section_seperators = false;

            explicit SectionBase(const calculator::AttackRating& attack_rating) { }

            void update(const calculator::AttackRating& attack_rating) { }
        };

        export struct NameSection : SectionBase<std::array<std::array<QVariant, 2>, 1>>
        {
            inline const static std::vector<QString> column_names { string_to_display("name") };

            explicit NameSection(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();
                auto&& attack_options = attack_rating.attack_options;

                (*this)[0][0] = string_to_display(weapon.qualified_name(attack_options.upgrade_levels.at(weapon.upgrade_level_index)));
                (*this)[0][1] = QUrl(QString::fromStdString(weapon.url));
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole || role == Qt::UserRole)
                    return (*this)[column][0];
                
                if (role == Qt::UserRole + 1)
                    return (*this)[column][1];
                
                return {};
            }
        };

        export struct BinaryTextSection : SectionBase<std::array<std::array<QVariant, 2>, 1>>
        {
            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole)
                    return (*this)[column][0];

                if (role == Qt::UserRole)
                    return (*this)[column][1];
                
                return {};
            }
        };
        export struct AffinitySection : BinaryTextSection
        {
            inline const static std::vector<QString> column_names { string_to_display("affinity") };

            explicit AffinitySection(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                (*this)[0][0] = string_to_display(enum_to_string(weapon.affinity));
                (*this)[0][1] = std::to_underlying(weapon.affinity);
            }
        };
        export struct TypeSection : BinaryTextSection
        {
            inline const static std::vector<QString> column_names { string_to_display("type") };

            explicit TypeSection(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                (*this)[0][0] = string_to_display(enum_to_string(weapon.type));
                (*this)[0][1] = std::to_underlying(weapon.type);
            }
        };
        export struct BaseGameDLCSection : BinaryTextSection
        {
            inline const static std::vector<QString> column_names { string_to_display("base game\ndlc") };

            explicit BaseGameDLCSection(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                (*this)[0][0] = string_to_display(weapon.dlc ? "dlc" : "base game");
                (*this)[0][1] = weapon.dlc;
            }
        };

        struct UnaryTextSection : SectionBase<std::array<QVariant, 1>>
        {
            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole || role == Qt::UserRole)
                    return (*this)[column];
                
                return {};
            }
        };
        export struct BaseNameSection : UnaryTextSection
        {
            inline const static std::vector<QString> column_names { string_to_display("base name") };

            explicit BaseNameSection(const calculator::AttackRating& attack_rating)
            {
                (*this)[0] = string_to_display(attack_rating.weapon.get().base_name);
            }
        };

        template<std::size_t I>
        struct DataSection : SectionBase<std::array<std::array<QVariant, 3>, I>>
        {
            static constexpr bool draw_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole)
                    return this->at(column)[0];
                
                if (role == Qt::UserRole)
                    return this->at(column)[1];
                
                if (role == Qt::ForegroundRole)
                    return this->at(column)[2];
                
                static const auto alignment = QVariant::fromValue(Qt::AlignCenter);
                if (role == Qt::TextAlignmentRole)
                    return alignment;
                
                return {};
            }
        };
        export struct SpellScaling : DataSection<1>
        {
            static constexpr bool draw_header_labels_rotated = false;
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
        export struct AttackPowers : DataSection<enumerators_of<calculator::DamageType>().size() + 1>
        {
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "attack power";
            inline const static std::vector<QString> column_names = [](){
                auto result = enumerators_of<calculator::DamageType>()
                    | std::views::transform([](calculator::DamageType e){ return string_to_display(enum_to_string(e)); })
                    | std::ranges::to<std::vector>();
                result.emplace_back("total");
                return result;
            }();

            explicit AttackPowers(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                bool any_ineffective = false;
                for (auto&& [ap, is_ineffective, arr] : std::views::zip(
                    attack_rating.attack_powers | std::views::take(enumerators_of<calculator::DamageType>().size()),
                    attack_rating.ineffective_attack_power_types | std::views::take(enumerators_of<calculator::DamageType>().size()),
                    *this))
                {
                    arr[0] = /*format_number(ap[0]) + "/" +*/ format_number(ap[1]);
                    arr[1] = ap[1];
                    arr[2] = foreground_color(is_ineffective);
                    any_ineffective |= is_ineffective;
                }

                this->back()[0] = format_number(attack_rating.total_attack_power[1]);
                this->back()[1] = attack_rating.total_attack_power[1];
                this->back()[2] = foreground_color(any_ineffective);
            }
        };

        template<typename E>
        struct EnumDataSection : DataSection<enumerators_of<E>().size()>
        {
            using enum_type = E;

            inline const static std::vector<QString> column_names = enumerators_of<enum_type>()
                | std::views::transform([](enum_type e){ return string_to_display(enum_to_string(e)); })
                | std::ranges::to<std::vector>();
        };
        export struct StatusEffects : EnumDataSection<calculator::StatusEffectType>
        {
            using EnumDataSection::EnumDataSection;

            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "status effects";

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
        export struct AttributeScalings : EnumDataSection<calculator::RelevantAttribute>
        {
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "attribute scaling";

            explicit AttributeScalings(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                for (auto&& [attribute_scaling, is_ineffective, arr] : std::views::zip(
                    attack_rating.attribute_scalings,
                    attack_rating.ineffective_attributes,
                    *this))
                {
                    auto scaling_tier = weapon.calculate_scaling_tier(attribute_scaling);
                    if (scaling_tier.empty())
                        arr[0] = format_number(attribute_scaling * 100);
                    else
                        arr[0] = format_number(attribute_scaling * 100) + " (" + QString::fromStdString(scaling_tier) + ")";
                    arr[1] = attribute_scaling * 100;
                    arr[2] = foreground_color(is_ineffective);
                }
            }
        };
        export struct Requirements : EnumDataSection<calculator::RelevantAttribute>
        {
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "attribute requirements";

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
        export struct Stats : EnumDataSection<calculator::RelevantAttribute>
        {
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "character stats";

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
    }

    export template<typename...Args>
    struct BasicRow : _tuple_base<std::tuple<Args...>>
    {
        using _tuple_base = _tuple_base<std::tuple<Args...>>;
        using _tuple_base::_tuple_base;

        static constexpr std::array section_sizes = { std::tuple_size_v<Args>... };
        static constexpr std::array cumulative_section_sizes = []() {
            std::array<std::size_t, sizeof...(Args)> result{};
            std::partial_sum(section_sizes.begin(), section_sizes.end(), result.begin());
            return result;
        }();
        static constexpr std::array section_index_offsets = []() {
            std::array<std::size_t, sizeof...(Args)> result{};
            std::ranges::copy(cumulative_section_sizes | std::views::take(sizeof...(Args) - 1), result.begin() + 1);
            return result;
        }();
        static constexpr std::size_t total_size = std::accumulate(section_sizes.begin(), section_sizes.end(), 0);
        static constexpr std::array draw_section_seperators = { Args::draw_section_seperators... };
        static constexpr std::array draw_header_labels_rotated = { Args::draw_header_labels_rotated... };
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
                    if (index < section_sizes[I])
                        return std::get<I>(*this).data(index, role);
                    else
                        return self.template operator()<I + 1>(index - section_sizes[I]);
                }
                throw std::out_of_range("index out of range");
            }(column);
        }
    };

    export using Row = BasicRow<
        sections::NameSection,
        sections::AffinitySection,
        sections::TypeSection,
        sections::SpellScaling,
        sections::AttackPowers,
        sections::StatusEffects,
        sections::AttributeScalings,
        sections::Requirements,
        sections::Stats,
        sections::BaseGameDLCSection,

        sections::BaseNameSection
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

        for (auto i = 0; i < std::tuple_size_v<Row>; ++i)
        {
            if (Row::draw_section_seperators[i])
            {
                auto first_column = Row::section_index_offsets[i];
                auto last_column = Row::cumulative_section_sizes[i] - 1;

                if (!header->isSectionHidden(first_column))
                {
                    const int x = header->sectionViewportPosition(first_column);
                    painter.drawLine(x, bounds.top(), x, bounds.bottom());
                }

                if (!header->isSectionHidden(last_column))
                {
                    const int x = header->sectionViewportPosition(last_column) + header->sectionSize(last_column);
                    painter.drawLine(x, bounds.top(), x, bounds.bottom());
                }
            }
        }

        painter.restore();
    }

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

    protected:
        void paintSection(QPainter *painter, const QRect &rect_, int logicalIndex) const override
        {
            QRect rect = rect_;

            // Move your normal section contents down
            rect.translate(0, this->group_header_height());

            painter->save();

            bool rotate = std::ranges::contains(rotated_columns, logicalIndex);

            if (!rotate)
            {
                this->QHeaderView::paintSection(painter, rect, logicalIndex);
                painter->restore();
                return;
            }

            QStyleOptionHeader option;
            this->initStyleOption(&option);
            option.rect = rect;
            option.text.clear();   // prevent normal text drawing

            // Keep the sort indicator information
            if (sortIndicatorSection() == logicalIndex) {
                option.sortIndicator = sortIndicatorOrder() == Qt::AscendingOrder
                    ? QStyleOptionHeader::SortDown
                    : QStyleOptionHeader::SortUp;
            }

            this->style()->drawControl(QStyle::CE_Header, &option, painter, this);

            QString text = this->model()->headerData(
                logicalIndex,
                this->orientation(),
                Qt::DisplayRole).toString();

            painter->setPen(option.palette.color(QPalette::Text));

            painter->translate(
                rotation == Rotation::CounterClockwise ? rect.left() : rect.right(),
                rotation == Rotation::CounterClockwise ? rect.bottom() : rect.top());

            painter->rotate(rotation == Rotation::CounterClockwise ? -90 : 90);

            // Leave room for the indicator
            QRect textRect(0, 0, rect.height(), rect.width());

            int indicatorSize = this->style()->pixelMetric(
                QStyle::PM_HeaderMarkSize, &option, this);

            textRect.adjust(0, 0, -indicatorSize, 0);

            painter->drawText(textRect, Qt::AlignCenter, text);

            painter->restore();
        }

        QSize sectionSizeFromContents(int logicalIndex) const override
        {
            QSize size = QHeaderView::sectionSizeFromContents(logicalIndex);

            // Width becomes height after rotation
            if (std::ranges::contains(rotated_columns, logicalIndex))
                return QSize(
                    size.height(),
                    size.width()
                );

            return size;
        }

        QSize sizeHint() const override
        {
            QSize s = QHeaderView::sizeHint();

            // Preserve your sectionSizeFromContents() height
            s.setHeight(s.height() + this->group_header_height());

            return s;
        }

        void paintEvent(QPaintEvent *e) override
        {
            this->QHeaderView::paintEvent(e);

            auto viewport = this->viewport();
            QPainter p(viewport);
            p.save();

            auto height = this->group_header_height();
            const QRect bounds = viewport->rect();
            QPen pen(Qt::black, 1);
            pen.setCosmetic(true);
            p.setPen(pen);
            p.drawLine(bounds.left(), height, bounds.right(), height);
            p.drawLine(bounds.left(), this->height() - 1, bounds.right(), this->height() - 1);

            pen.setCosmetic(false);
            p.setPen(pen);

            [&]<std::size_t I = 0>(this auto&& self) -> void
            {
                if constexpr (I < std::tuple_size_v<Row>)
                {
                    if constexpr (std::tuple_element_t<I, Row>::has_header_section_title)
                    {
                        constexpr auto first_column = Row::section_index_offsets[I];
                        constexpr auto last_column = Row::cumulative_section_sizes[I] - 1;

                        auto left  = this->sectionViewportPosition(first_column);
                        auto right = this->sectionViewportPosition(last_column) + this->sectionSize(last_column);

                        QRect r(
                            left,
                            0,
                            right - left,
                            height
                        );

                        p.drawText(r, Qt::AlignCenter, std::tuple_element_t<I, Row>::header_section_title);
                    }
                    return self.template operator()<I + 1>();
                }
            }();

            draw_column_group_separators(this->viewport(), this);

            p.restore();
        }

    private:
        int group_header_height() const
        {
            return this->fontMetrics().height() + 8;
        }

        Rotation rotation;
        static const inline auto rotated_columns = std::views::iota(std::size_t{}, std::tuple_size_v<Row>)
            | std::views::filter([](std::size_t i) { return Row::draw_header_labels_rotated[i]; })
            | std::views::transform([](std::size_t i) { return std::views::iota(Row::section_index_offsets[i], Row::cumulative_section_sizes[i]); })
            | std::views::join
            | std::ranges::to<std::vector>();
    };

    export class RowSortFilterModel : public QSortFilterProxyModel
    {
    public:
        explicit RowSortFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent)
        {
            this->setSortRole(Qt::UserRole);
        }

        void set_selected_base_game_dlc(QSet<bool>&& base_game_dlc)
        {
            this->beginFilterChange();
            this->base_game_dlc = std::move(base_game_dlc);
            this->endFilterChange();
        }
        void set_selected_types(QSet<int>&& types)
        {
            this->beginFilterChange();
            this->types = std::move(types);
            this->endFilterChange();
        }
        void set_selected_base_names(QSet<QString>&& base_names)
        {
            this->beginFilterChange();
            this->base_names = std::move(base_names);
            this->endFilterChange();
        }
        void set_selected_affinities(QSet<int>&& affinities)
        {
            this->beginFilterChange();
            this->affinities = std::move(affinities);
            this->endFilterChange();
        }

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
        {
            auto source_model = this->sourceModel();

            auto check_filter = [&]<typename T>(std::size_t column, const QSet<T>& set){
                if (set.isEmpty())
                    return true;

                auto index = source_model->index(
                    sourceRow,
                    Row::section_index_offsets[column],
                    sourceParent
                );

                T value;
                if constexpr (std::same_as<T, QString>)
                    value = index.data(Qt::UserRole).toString();
                else if constexpr (std::same_as<T, int>)
                    value = index.data(Qt::UserRole).toInt();
                else if constexpr (std::same_as<T, bool>)
                    value = index.data(Qt::UserRole).toBool();

                return set.contains(value);
            };

            return check_filter(tuple_index_v<sections::BaseGameDLCSection, Row>, this->base_game_dlc)
                && check_filter(tuple_index_v<sections::TypeSection, Row>, this->types)
                && check_filter(tuple_index_v<sections::BaseNameSection, Row>, this->base_names)
                && check_filter(tuple_index_v<sections::AffinitySection, Row>, this->affinities);
        }

    private:
        QSet<bool> base_game_dlc; // true = dlc, false = base game
        QSet<int> types;
        QSet<QString> base_names;
        QSet<int> affinities;
    };

    class LinkDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            QStyleOptionViewItem opt(option);
            initStyleOption(&opt, index);

            opt.palette.setColor(QPalette::Text, Qt::blue);
            opt.font.setUnderline(true);

            QStyledItemDelegate::paint(painter, opt, index);
        }

        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &, const QModelIndex &index) override
        {
            if (event->type() == QEvent::MouseButtonRelease)
            {
                const QString url = index.data(Qt::UserRole + 1).toString();
                if (!url.isEmpty())
                {
                    QDesktopServices::openUrl(QUrl(url));
                    return true;
                }
            }
            return false;
        }
    };

    export class WeaponTable : public QTableView
    {
    public:

        RowModel* model = new RowModel(this);
        RowSortFilterModel* proxy_model = new RowSortFilterModel(this);
        RotatedHeaderView* header = new RotatedHeaderView(Qt::Horizontal, RotatedHeaderView::Rotation::Clockwise, this);

        explicit WeaponTable(QWidget *parent = nullptr) : QTableView(parent)
        {
            this->proxy_model->setSourceModel(this->model);
            this->setModel(this->proxy_model);

            this->setHorizontalHeader(this->header);

            this->setFrameStyle(QFrame::Box);
            this->setSortingEnabled(true);
            this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            this->setItemDelegateForColumn(Row::section_index_offsets[tuple_index_v<sections::NameSection, Row>], new LinkDelegate(this));

            this->hide_section<sections::BaseNameSection>();
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

                for (int i = 0; i < visibleColumns.size() - 1; ++i)
                {
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

        template<typename ColumnType>
        void hide_section()
        {
            static constexpr auto I = tuple_index_v<ColumnType, Row>;
            for (auto && index : std::views::iota(
                Row::section_index_offsets[I],
                Row::cumulative_section_sizes[I]
            ))
                this->header->hideSection(static_cast<int>(index));
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QTableView::paintEvent(event);

            draw_column_group_separators(this->viewport(), this->horizontalHeader());
        }
    };
}