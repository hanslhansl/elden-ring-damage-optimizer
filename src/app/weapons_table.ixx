module;
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPainter>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <qabstractitemmodel.h>
export module erdo.ui.weapons_table;

import std;
import erdo;
import erdo.ui.settings;


namespace erdo::ui
{
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
    auto foreground_color(bool is_ineffective)
    {
        return is_ineffective ? QColor(Qt::red) : QColor(Qt::black);
    }
 
    enum QtDataRole
    {
        LinkRole = Qt::UserRole + 1
    };

    namespace sections
    {
        static const auto alignment_center = QVariant::fromValue(Qt::AlignCenter);

        template<typename T>
        struct SectionBase : _tuple_base<T>
        {
            using _tuple_base<T>::_tuple_base;

            static constexpr bool draw_section_header_labels_rotated = false;
            static constexpr bool has_header_section_title = false;
            static constexpr bool draw_section_seperators = false;
            static constexpr bool expand_section = false;

            explicit SectionBase(const calculator::AttackRating& attack_rating) { }

            void update(const calculator::AttackRating& attack_rating) { }
        };

        export struct NameSection : SectionBase<std::array<std::array<QVariant, 3>, 1>>
        {
            inline const static std::vector<QString> column_names { string_to_display("name") };

            using SectionBase::SectionBase;
            explicit NameSection(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                if (settings.display_base_names_instead_of_full_names)
                    (*this)[0][0] = string_to_display(weapon.qualified_base_name(attack_rating.upgrade_level()));
                else
                    (*this)[0][0] = string_to_display(weapon.qualified_name(attack_rating.upgrade_level()));

                if (settings.sort_by_base_names_instead_of_full_names)
                    (*this)[0][1] = string_to_display(weapon.base_name);
                else
                    (*this)[0][1] = string_to_display(weapon.full_name);

                if (settings.link_to_fextralife_instead_of_fandom)
                    (*this)[0][2] = QUrl(QString::fromStdString(weapon.fextralife_link()));
                else
                    (*this)[0][2] = QUrl(QString::fromStdString(weapon.fandom_link()));
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole)
                    return (*this)[column][0];

                if (role == Qt::UserRole)
                    return (*this)[column][1];
                
                if (role == QtDataRole::LinkRole)
                    return (*this)[column][2];
                
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

            using BinaryTextSection::BinaryTextSection;
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

            using BinaryTextSection::BinaryTextSection;
            explicit TypeSection(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                (*this)[0][0] = string_to_display(enum_to_string(weapon.type));
                (*this)[0][1] = std::to_underlying(weapon.type);
            }
        };
        
        export struct BaseGameDLCSection : SectionBase<std::array<std::array<QVariant, 2>, 1>>
        {
            inline const static std::vector<QString> column_names { string_to_display("base game\ndlc") };

            using SectionBase::SectionBase;
            explicit BaseGameDLCSection(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                (*this)[0][0] = string_to_display(weapon.dlc ? "dlc" : "base game");
                (*this)[0][1] = weapon.dlc;
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole)
                    return (*this)[column][0];

                if (role == Qt::UserRole)
                    return (*this)[column][1];

                if (role == Qt::TextAlignmentRole)
                    return alignment_center;
                
                return {};
            }
        };

        export struct BaseNameSection : SectionBase<std::array<QVariant, 1>>
        {
            inline const static std::vector<QString> column_names { string_to_display("base name") };

            using SectionBase::SectionBase;
            explicit BaseNameSection(const calculator::AttackRating& attack_rating)
            {
                (*this)[0] = string_to_display(attack_rating.weapon.get().base_name);
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole || role == Qt::UserRole)
                    return (*this)[column];
                
                return {};
            }
        };
        
        export struct CharacterLevelSection : SectionBase<std::array<QVariant, 1>>
        {
            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;

            inline const static std::vector<QString> column_names { string_to_display("character level") };

            using SectionBase::SectionBase;
            explicit CharacterLevelSection(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                (*this)[0] = attack_rating.stats.character_level();
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole || role == Qt::UserRole)
                    return (*this)[column];
                
                if (role == Qt::TextAlignmentRole)
                    return alignment_center;

                return {};
            }
        };

        template<std::size_t I>
        struct DataSection : SectionBase<std::array<std::array<QVariant, 3>, I>>
        {
            static constexpr bool expand_section = true;

            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole)
                    return this->at(column)[0];
                
                if (role == Qt::UserRole)
                    return this->at(column)[1];
                
                if (role == Qt::ForegroundRole)
                    return this->at(column)[2];
                
                if (role == Qt::TextAlignmentRole)
                    return alignment_center;
                
                return {};
            }
        };
        export struct SpellScaling : DataSection<1>
        {
            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;
            inline const static std::vector<QString> column_names = { string_to_display("spell scaling") };

            using DataSection::DataSection;
            explicit SpellScaling(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                (*this)[0][0] = format_number(attack_rating.spell_scaling * 100);
                (*this)[0][1] = attack_rating.spell_scaling * 100;
                (*this)[0][2] = foreground_color(attack_rating.is_spell_scaling_ineffective());
            }
        };
        export struct AttackPowers : DataSection<enumerators_of<calculator::DamageType>().size() + 1>
        {
            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "attack power";
            inline const static std::vector<QString> column_names = [](){
                auto result = enumerator_strings_of<calculator::DamageType>()
                    | std::views::transform([](std::string_view e){ return string_to_display(e); })
                    | std::ranges::to<std::vector>();
                result.emplace_back("total");
                return result;
            }();

            using DataSection::DataSection;
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
                    arr[0] = /*format_number(ap[0]) + "/" +*/ format_number(ap[1]);
                    arr[1] = ap[1];
                    arr[2] = foreground_color(is_ineffective);
                }

                this->back()[0] = format_number(attack_rating.total_attack_power[1]);
                this->back()[1] = attack_rating.total_attack_power[1];
                this->back()[2] = foreground_color(attack_rating.is_total_attack_power_ineffective());
            }
        };

        template<typename E>
        struct EnumDataSection : DataSection<enumerators_of<E>().size()>
        {
            using enum_type = E;

            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;
            
            inline const static std::vector<QString> column_names = enumerator_strings_of<enum_type>()
                | std::views::transform([](std::string_view e){ return string_to_display(e); })
                | std::ranges::to<std::vector>();
        };
        export struct StatusEffects : EnumDataSection<calculator::StatusEffectType>
        {
            static constexpr bool has_header_section_title = true;
            inline static const QString header_section_title = "status effects";

            using EnumDataSection::EnumDataSection;
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

            using EnumDataSection::EnumDataSection;
            explicit AttributeScalings(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                auto&& weapon = attack_rating.weapon.get();

                for (auto&& [attribute_scaling, scaling_tier, is_ineffective, arr] : std::views::zip(
                    attack_rating.attribute_scalings(),
                    attack_rating.calculate_scaling_tiers(),
                    attack_rating.ineffective_attributes,
                    *this))
                {
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

            using EnumDataSection::EnumDataSection;
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

            using EnumDataSection::EnumDataSection;
            explicit Stats(const calculator::AttackRating& attack_rating)
            {
                this->update(attack_rating);
            }

            void update(const calculator::AttackRating& attack_rating)
            {
                for (auto&& [stat, is_ineffective, arr] : std::views::zip(
                    attack_rating.stats.relevant_stats(),
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

    export template<std::default_initializable...Args>
    struct BasicRow : _tuple_base<std::tuple<Args...>>
    {
        using _tuple_base = _tuple_base<std::tuple<Args...>>;
        using _tuple_base::_tuple_base;

        calculator::AttackRating attack_rating { calculator::Weapon::dummy, {}, {} };

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
        static constexpr std::array draw_column_header_label_rotated = [](){
            std::array draw_section_header_labels_rotated = { Args::draw_section_header_labels_rotated... };
            std::array<bool, total_size> result{};
            for (auto [draw_rotated, section_index_offset, section_size] : std::views::zip(draw_section_header_labels_rotated, section_index_offsets, section_sizes))
                if (draw_rotated)
                    std::ranges::fill(result | std::views::drop(section_index_offset) | std::views::take(section_size), true);
            
            return result;
        }();
        static constexpr std::array expand_column = [](){
            std::array expand_section = { Args::expand_section... };
            std::array<bool, total_size> result{};
            for (auto [expand, section_index_offset, section_size] : std::views::zip(expand_section, section_index_offsets, section_sizes))
                if (expand)
                    std::ranges::fill(result | std::views::drop(section_index_offset) | std::views::take(section_size), true);
            
            return result;
        }();

        static const QString& column_name(int column)
        {
            const static std::vector<QString> column_names = [](){
                std::vector<QString> result{};
                result.reserve(total_size);
                (result.append_range(Args::column_names), ...);
                return result;
            }();
            return column_names.at(column);
        }

        BasicRow() = default;
        explicit BasicRow(calculator::AttackRating&& attack_rating) : _tuple_base(Args(attack_rating)...), attack_rating{ std::move(attack_rating) } { }

        void update(calculator::AttackRating&& attack_rating)
        {
            this->attack_rating = std::move(attack_rating);
            this->update();
        }
        void update()
        {
            std::apply(
                [&](auto&&...args) {
                    (std::forward<decltype(args)>(args).update(this->attack_rating),...);
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
        sections::CharacterLevelSection,
        sections::BaseGameDLCSection,

        sections::BaseNameSection
    >;

    export struct RowModel : QAbstractTableModel
    {
        friend class WeaponsTable;

        std::vector<Row> rows;

        explicit RowModel(QObject* parent = nullptr) : QAbstractTableModel(parent)
        {
            connect(&settings.decimal_places, settings.decimal_places.changed_member_pointer, [this](){
                for (auto&& row : this->rows)
                    row.update();
                emit dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, this->columnCount() - 1));
            });

            static constexpr auto name_section_index = tuple_index_v<sections::NameSection, Row>;

            connect(&settings.display_base_names_instead_of_full_names, settings.display_base_names_instead_of_full_names.changed_member_pointer, [this](){
                for (auto&& row : this->rows)
                    std::get<sections::NameSection>(row).update(row.attack_rating);
                emit dataChanged(
                    this->index(0, Row::section_index_offsets[name_section_index]),
                    this->index(this->rowCount() - 1, Row::cumulative_section_sizes[name_section_index]-1),
                    { Qt::DisplayRole }
                );
            });
            connect(&settings.sort_by_base_names_instead_of_full_names, settings.sort_by_base_names_instead_of_full_names.changed_member_pointer, [this](){
                for (auto&& row : this->rows)
                    std::get<sections::NameSection>(row).update(row.attack_rating);
                emit dataChanged(
                    this->index(0, Row::section_index_offsets[name_section_index]),
                    this->index(this->rowCount() - 1, Row::cumulative_section_sizes[name_section_index]-1),
                    {  Qt::UserRole }
                );
            });
            
            connect(&settings.link_to_fextralife_instead_of_fandom, settings.link_to_fextralife_instead_of_fandom.changed_member_pointer, [this](){
                for (auto&& row : this->rows)
                    std::get<sections::NameSection>(row).update(row.attack_rating);
                emit dataChanged(
                    this->index(0, Row::section_index_offsets[name_section_index]),
                    this->index(this->rowCount() - 1, Row::cumulative_section_sizes[name_section_index]-1),
                    {  QtDataRole::LinkRole }
                );
            });
        }

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
                return Row::column_name(section);

            return {};
        }

        void set_rows(std::vector<Row>&& rows)
        {
            this->beginResetModel();
            this->rows = std::move(rows);
            this->endResetModel();
        }

        void update_rows(std::ranges::range auto&& attack_ratings)
        {
            for (auto&& [attack_rating, row] : std::views::zip(attack_ratings, this->rows))
                row.update(std::move(attack_rating));
            emit dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, this->columnCount() - 1));
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

    class RotatedHeaderView : public QHeaderView
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

            bool rotate = Row::draw_column_header_label_rotated[logicalIndex];

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
            if (Row::draw_column_header_label_rotated[logicalIndex])
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
    };

    class RowSortFilterModel : public QSortFilterProxyModel
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
                const QString url = index.data(QtDataRole::LinkRole).toString();
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
        void resize_columns_to_contents_impl()
        {
            const int columns = this->model->columnCount();

            this->header->setSectionResizeMode(QHeaderView::ResizeToContents);
            this->resizeColumnsToContents();

            QVector<int> widths(columns);
            QVector<int> visibleColumns;
            QVector<int> expandableColumns;

            int total = 0;

            for (int c = 0; c < columns; ++c)
            {
                if (this->header->isSectionHidden(c))
                    continue;

                widths[c] = std::max({
                    this->header->sectionSize(c),
                    this->header->sectionSizeHint(c),
                    this->header->minimumSectionSize()
                });
                total += widths[c];
                visibleColumns.append(c);

                if (Row::expand_column[c])
                    expandableColumns.append(c);
            }

            this->header->setSectionResizeMode(QHeaderView::Interactive);

            if (total <= 0 || visibleColumns.isEmpty())
                return;

            const int available = this->viewport()->width();

            // First restore the content-based widths.
            for (int c : visibleColumns)
                this->header->resizeSection(c, widths[c]);

            if (total >= available || expandableColumns.isEmpty())
                return;

            int extra = available - total;

            // Sort expandable columns from narrowest to widest.
            std::sort(
                expandableColumns.begin(),
                expandableColumns.end(),
                [&](int a, int b)
                {
                    return widths[a] < widths[b];
                });

            // Raise the narrowest columns until they reach the next width level.
            int level = widths[expandableColumns[0]];

            for (int i = 1; i < expandableColumns.size() && extra > 0; ++i)
            {
                const int nextLevel = widths[expandableColumns[i]];
                const int count = i;

                const int required = (nextLevel - level) * count;

                if (required > extra)
                {
                    // Can't reach the next level.
                    const int increase = extra / count;
                    const int remainder = extra % count;

                    for (int j = 0; j < count; ++j)
                    {
                        const int c = expandableColumns[j];
                        const int delta = increase + (j < remainder ? 1 : 0);

                        this->header->resizeSection(c, widths[c] + delta);
                    }

                    return;
                }

                // Raise the first `count` columns to the next level.
                for (int j = 0; j < count; ++j)
                {
                    const int c = expandableColumns[j];
                    widths[c] = nextLevel;
                    this->header->resizeSection(c, widths[c]);
                }

                extra -= required;
                level = nextLevel;
            }

            // All expandable columns have reached the same width.
            // Distribute any remaining space evenly.
            if (extra > 0)
            {
                const int count = expandableColumns.size();
                const int increase = extra / count;
                const int remainder = extra % count;

                for (int i = 0; i < count; ++i)
                {
                    const int c = expandableColumns[i];
                    const int delta = increase + (i < remainder ? 1 : 0);

                    this->header->resizeSection(c, widths[c] + delta);
                }
            }
        }

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

            connect(this->model, &RowModel::dataChanged, this, &WeaponTable::resize_columns_to_contents);
            connect(this->model, &RowModel::modelReset, this, &WeaponTable::resize_columns_to_contents);

            if (settings.hide_base_game_dlc_column)
                this->hide_section<sections::BaseGameDLCSection>();
            connect(&settings.hide_base_game_dlc_column, settings.hide_base_game_dlc_column.changed_member_pointer, [this](){
                if (settings.hide_base_game_dlc_column)
                    this->hide_section<sections::BaseGameDLCSection>();
                else
                    this->show_section<sections::BaseGameDLCSection>();
                this->resize_columns_to_contents();
            });
        }

        void resize_columns_to_contents()
        {
            if (!this->isVisible())
                return;

            QTimer::singleShot(0, this, [this]()
            {
                this->doItemsLayout();

                QTimer::singleShot(0, this, [this]()
                {
                    if (!this->isVisible())
                        return;

                    this->resize_columns_to_contents_impl();
                });
            });
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
        template<typename ColumnType>
        void show_section()
        {
            static constexpr auto I = tuple_index_v<ColumnType, Row>;
            for (auto && index : std::views::iota(
                Row::section_index_offsets[I],
                Row::cumulative_section_sizes[I]
            ))
                this->header->showSection(static_cast<int>(index));
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QTableView::paintEvent(event);

            draw_column_group_separators(this->viewport(), this->horizontalHeader());
        }
    
        void showEvent(QShowEvent *event) override
        {
            QTableView::showEvent(event);

            this->resize_columns_to_contents();
        }
    };
}