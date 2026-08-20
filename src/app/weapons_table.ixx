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
#include <qmenu.h>
export module erdo.ui.weapons_table;

import std;
import erdo;
import erdo.ui.settings;

using namespace std::literals;

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
 

    namespace sections
    {
        static const auto alignment_center = QVariant::fromValue(Qt::AlignCenter);

        template<typename T>
        struct SectionBase : _tuple_base<T>
        {
            using _tuple_base<T>::_tuple_base;

            static constexpr bool draw_section_header_labels_rotated = false;
            static constexpr bool draw_section_seperators = false;
            static constexpr bool expand_section = false;
            static inline const QString section_name = "";

            explicit SectionBase(const calculator::Attack& attack) { }

            void update(const calculator::Attack& attack) { }
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
        export struct NameSection : BinaryTextSection
        {
            static constexpr std::array column_names { "Name" };

            using BinaryTextSection::BinaryTextSection;
            explicit NameSection(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                (*this)[0][0] = QString::fromStdString(weapon.qualified_name(attack.upgrade_level()));
                (*this)[0][1] = QString::fromStdString(weapon.full_name);
            }
        };
        export struct AffinitySection : BinaryTextSection
        {
            static constexpr std::array column_names { "Affinity" };

            using BinaryTextSection::BinaryTextSection;
            explicit AffinitySection(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                (*this)[0][0] = enum_to_display(weapon.affinity);
                (*this)[0][1] = std::to_underlying(weapon.affinity);
            }
        };
        export struct TypeSection : BinaryTextSection
        {
            static constexpr std::array column_names { "Type" };

            using BinaryTextSection::BinaryTextSection;
            explicit TypeSection(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                (*this)[0][0] = enum_to_display(weapon.type);
                (*this)[0][1] = std::to_underlying(weapon.type);
            }
        };
        
        export struct BaseGameDLCSection : SectionBase<std::array<std::array<QVariant, 2>, 1>>
        {
            static constexpr std::array column_names { "Base Game\nDLC" };

            using SectionBase::SectionBase;
            explicit BaseGameDLCSection(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                (*this)[0][0] = weapon.dlc ? "DLC" : "Base Game";
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
            static constexpr std::array column_names { "Base Name" };

            using SectionBase::SectionBase;
            explicit BaseNameSection(const calculator::Attack& attack)
            {
                (*this)[0] = QString::fromStdString(attack.weapon.get().base_name);
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
            static constexpr bool expand_section = true;
            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;

            static constexpr std::array column_names { "Character Level" };

            using SectionBase::SectionBase;
            explicit CharacterLevelSection(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                (*this)[0] = attack.stats.character_level();
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
            static constexpr std::array column_names = { "Spell Scaling" };

            using DataSection::DataSection;
            explicit SpellScaling(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                (*this)[0][0] = format_number(attack.spell_scaling * 100);
                (*this)[0][1] = attack.spell_scaling * 100;
                (*this)[0][2] = foreground_color(attack.is_spell_scaling_ineffective());
            }
        };
        export struct AttackPowers : DataSection<enumerators_of<calculator::DamageType>().size() + 1>
        {
            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;
            static inline const QString section_name = "Attack Power";
            inline const static std::vector<QString> column_names = [](){
                auto result = enumerators_of<calculator::DamageType>()
                    | std::views::transform(enum_to_display)
                    | std::ranges::to<std::vector>();
                result.emplace_back("Total");
                return result;
            }();

            using DataSection::DataSection;
            explicit AttackPowers(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                for (auto&& [ap, is_ineffective, arr] : std::views::zip(
                    attack.attack_powers | std::views::take(enumerators_of<calculator::DamageType>().size()),
                    attack.ineffective_attack_power_types | std::views::take(enumerators_of<calculator::DamageType>().size()),
                    *this))
                {
                    arr[0] = /*format_number(ap[0]) + "/" +*/ format_number(ap[1]);
                    arr[1] = ap[1];
                    arr[2] = foreground_color(is_ineffective);
                }

                this->back()[0] = format_number(attack.total_attack_power[1]);
                this->back()[1] = attack.total_attack_power[1];
                this->back()[2] = foreground_color(attack.is_total_attack_power_ineffective());
            }
        };

        template<typename E>
        struct EnumDataSection : DataSection<enumerators_of<E>().size()>
        {
            using enum_type = E;

            static constexpr bool draw_section_header_labels_rotated = true;
            static constexpr bool draw_section_seperators = true;
            
            inline const static std::vector<QString> column_names = enumerators_of<enum_type>()
                | std::views::transform(enum_to_display)
                | std::ranges::to<std::vector>();
        };
        export struct StatusEffects : EnumDataSection<calculator::StatusEffectType>
        {
            static inline const QString section_name = "Status Effects";

            using EnumDataSection::EnumDataSection;
            explicit StatusEffects(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                for (auto&& [ap, is_ineffective, arr] : std::views::zip(
                    attack.attack_powers | std::views::drop(enumerators_of<calculator::DamageType>().size()),
                    attack.ineffective_attack_power_types | std::views::drop(enumerators_of<calculator::DamageType>().size()),
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
            static inline const QString section_name = "Attribute Scaling at Upgrade Level";

            using EnumDataSection::EnumDataSection;
            explicit AttributeScalings(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                for (auto&& [attribute_scaling, scaling_tier, is_ineffective, arr] : std::views::zip(
                    attack.attribute_scalings_at_upgrade_level(),
                    attack.calculate_scaling_tiers(),
                    attack.ineffective_attributes,
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
            static inline const QString section_name = "Attribute Requirements";

            using EnumDataSection::EnumDataSection;
            explicit Requirements(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                for (auto&& [requirement, is_ineffective, arr] : std::views::zip(
                    weapon.requirements,
                    attack.ineffective_attributes,
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
            static inline const QString section_name = "Character Attributes";

            using EnumDataSection::EnumDataSection;
            explicit Stats(const calculator::Attack& attack)
            {
                this->update(attack);
            }

            void update(const calculator::Attack& attack)
            {
                for (auto&& [stat, is_ineffective, arr] : std::views::zip(
                    attack.stats.relevant_stats(),
                    attack.ineffective_attributes,
                    *this))
                {
                    arr[0] = stat;
                    arr[1] = stat;
                    arr[2] = foreground_color(is_ineffective);
                }
            }
        };

        export template<calculator::AttackPowerType apt>
        struct AttackPowerTypeAttributeScalings : EnumDataSection<calculator::RelevantAttribute>
        {
            static constexpr auto attack_power_type = apt;
            static inline const auto section_name = enum_to_display(attack_power_type) + " Scaling";

            using EnumDataSection::EnumDataSection;
            explicit AttackPowerTypeAttributeScalings(const calculator::Attack& attack)
            {
                auto&& weapon = attack.weapon.get();

                for (auto&& [attack_power_type_attribute_scalings, arr] : std::views::zip(
                    attack.attack_power_type_attribute_scalings(attack_power_type),
                    *this))
                {
                    arr[0] = format_number(attack_power_type_attribute_scalings);
                    arr[1] = attack_power_type_attribute_scalings;
                }
            }
        };
    }

    export template<std::default_initializable...Args>
    struct BasicRow : _tuple_base<std::tuple<Args...>>
    {
        using _tuple_base = _tuple_base<std::tuple<Args...>>;
        using _tuple_base::_tuple_base;

        calculator::Attack attack { calculator::Weapon::dummy, {}, {} };

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
        static constexpr std::size_t total_size = std::ranges::fold_left(section_sizes, 0, std::plus{});
        static constexpr std::array column_index_to_section_index = []() {
            std::array<std::size_t, total_size> result{};
            for (std::size_t section_index = 0; section_index < sizeof...(Args); ++section_index)
                std::ranges::fill(result | std::views::drop(section_index_offsets[section_index]) | std::views::take(section_sizes[section_index]), section_index);
            return result;
        }();

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

        static const QString& section_name(int column)
        {
            const static std::vector<QString> section_names{ Args::section_name... };
            return section_names.at(column);
        }
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
        explicit BasicRow(calculator::Attack&& attack) : _tuple_base(Args(attack)...), attack{ std::move(attack) } { }

        void update(calculator::Attack&& attack)
        {
            this->attack = std::move(attack);
            this->update();
        }
        void update()
        {
            std::apply(
                [&](auto&&...args) {
                    (std::forward<decltype(args)>(args).update(this->attack),...);
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
        sections::BaseNameSection,
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

        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::PHYSICAL>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::MAGIC>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::FIRE>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::LIGHTNING>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::HOLY>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::POISON>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::SCARLET_ROT>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::BLEED>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::FROST>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::SLEEP>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::MADNESS>,
        sections::AttackPowerTypeAttributeScalings<calculator::AttackPowerType::DEATH_BLIGHT>
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

        void update_rows(std::ranges::range auto&& attacks)
        {
            for (auto&& [attack, row] : std::views::zip(attacks, this->rows))
                row.update(std::move(attack));
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
            this->setSectionsClickable(true);
        }

        bool is_section_hidden(std::size_t section_index) const
        {
            return this->isSectionHidden(Row::section_index_offsets[section_index]);
        }
        void set_section_hidden(std::size_t section_index, bool hide)
        {
            for (auto && index : std::views::iota(
                Row::section_index_offsets[section_index],
                Row::cumulative_section_sizes[section_index]
            ))
                this->setSectionHidden(static_cast<int>(index), hide);
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

            for (auto section_index : std::views::iota(0ull, std::tuple_size_v<Row>))
            {
                auto section_name = Row::section_name(section_index);
                if (!section_name.isEmpty())
                {
                    auto first_column = Row::section_index_offsets[section_index];
                    auto last_column = Row::cumulative_section_sizes[section_index] - 1;

                    auto left  = this->sectionViewportPosition(first_column);
                    auto right = this->sectionViewportPosition(last_column) + this->sectionSize(last_column);

                    QRect r(
                        left,
                        0,
                        right - left,
                        height
                    );

                    p.drawText(r, Qt::AlignCenter, section_name);
                }
            }

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

        void set_selected_base_game_dlc(QSet<bool> base_game_dlc)
        {
            this->beginFilterChange();
            this->base_game_dlc = std::move(base_game_dlc);
            this->endFilterChange();
        }
        void set_selected_types(QSet<int> types)
        {
            this->beginFilterChange();
            this->types = std::move(types);
            this->endFilterChange();
        }
        void set_selected_base_names(QSet<QString> base_names)
        {
            this->beginFilterChange();
            this->base_names = std::move(base_names);
            this->endFilterChange();
        }
        void set_selected_affinities(QSet<int> affinities)
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

        void show_table_context_menu(const QPoint &pos)
        {
            auto row_indices = this->selectionModel()->selectedRows()
                | std::views::transform([this](const QModelIndex& index){
                    return this->proxy_model->mapToSource(index).row();
                })
                | std::ranges::to<std::vector>();

            auto selection_name = row_indices.size() == 1
                ? this->model->rows[row_indices[0]].attack.weapon.get().full_name
                : std::format("Selection ({})", row_indices.size());


            QMenu menu(this);

            auto action_fandom = menu.addAction(QString::fromStdString(std::format("Show {} on Fandom", selection_name)));
            auto action_fextralife = menu.addAction(QString::fromStdString(std::format("Show {} on Fextralife", selection_name)));

            auto selected_action = menu.exec(this->viewport()->mapToGlobal(pos));

            if (selected_action == action_fandom)
                for (auto row_index : row_indices)
                    QDesktopServices::openUrl(QUrl(QString::fromStdString(this->model->rows[row_index].attack.weapon.get().fandom_url())));
            else if (selected_action == action_fextralife)
                for (auto row_index : row_indices)
                    QDesktopServices::openUrl(QUrl(QString::fromStdString(this->model->rows[row_index].attack.weapon.get().fextralife_url())));
        }

    public:
        RowModel* model = new RowModel(this);
        RowSortFilterModel* proxy_model = new RowSortFilterModel(this);
        RotatedHeaderView* header = new RotatedHeaderView(Qt::Horizontal, RotatedHeaderView::Rotation::Clockwise, this);

        explicit WeaponTable(QWidget *parent = nullptr) : QTableView(parent)
        {
            // view
            this->setFrameStyle(QFrame::Box);
            this->setSortingEnabled(true);
            this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            this->setSelectionBehavior(QAbstractItemView::SelectRows);
            this->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QTableView::customContextMenuRequested, this, &WeaponTable::show_table_context_menu);

            // model
            this->proxy_model->setSourceModel(this->model);
            this->setModel(this->proxy_model);
            connect(this->model, &RowModel::dataChanged, this, &WeaponTable::resize_columns_to_contents);
            connect(this->model, &RowModel::modelReset, this, &WeaponTable::resize_columns_to_contents);

            // header
            this->setHorizontalHeader(this->header);
            this->header->setContextMenuPolicy(Qt::CustomContextMenu);
            this->set_section_hidden<sections::BaseNameSection>(true);
            this->set_section_hidden<sections::BaseGameDLCSection>(true);
            [&](auto){
                static constexpr auto [...apt] = enumerators_of<calculator::AttackPowerType>();
                (this->set_section_hidden<sections::AttackPowerTypeAttributeScalings<apt>>(true), ...);
            }(1);
            connect(this->header, &QHeaderView::customContextMenuRequested, this, [this](const QPoint &pos)
            {
                QMenu menu;

                for (auto section_index : std::views::iota(0ull, std::tuple_size_v<Row>))
                {
                    auto name = Row::section_name(section_index);
                    if (name.isEmpty())
                        name = Row::column_name(Row::section_index_offsets[section_index]);

                    QAction *action = menu.addAction(name);
                    action->setCheckable(true);
                    action->setChecked(!this->header->is_section_hidden(section_index));

                    connect(action, &QAction::toggled, this, [this, section_index](bool visible) {
                        this->header->set_section_hidden(section_index, !visible);
                        this->resize_columns_to_contents();
                    });
                }

                menu.exec(this->mapToGlobal(pos));
            });
        }

        void resize_columns_to_contents()
        {
            if (!this->isVisible())
                return;

            QTimer::singleShot(0, this, [this]()
            {
                this->doItemsLayout();

                if (!this->isVisible())
                    return;

                this->resize_columns_to_contents_impl();
            });
        }

        template<typename ColumnType>
        void set_section_hidden(bool hide)
        {
            this->header->set_section_hidden(tuple_index_v<ColumnType, Row>, hide);
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