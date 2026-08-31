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
#include <QAbstractitemModel>
#include <QMenu>
#include <QColorDialog>
#include <QMouseEvent>
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

            explicit SectionBase(const calculator::FullAttackOptions& attack_options) { }

            void update(const calculator::FullAttackOptions& attack_options) { }
        };

        struct UnaryTextSection : SectionBase<std::array<QVariant, 1>>
        {
            QVariant data(int column, int role) const
            {
                if (role == Qt::DisplayRole || role == Qt::UserRole)
                    return (*this)[0];
                
                return {};
            }
        };
        export struct BaseNameSection : UnaryTextSection
        {
            static constexpr std::array column_names { "Base Name" };

            using UnaryTextSection::UnaryTextSection;
            explicit BaseNameSection(const calculator::FullAttackOptions& attack_options)
            {
                (*this)[0] = QString::fromStdString(attack_options.weapon.get().base_name);
            }
        };

        struct AlignedUnaryTextSection : UnaryTextSection
        {
            QVariant data(int column, int role) const
            {
                if (role == Qt::TextAlignmentRole)
                    return alignment_center;
                
                return this->UnaryTextSection::data(column, role);
            }
        };
        export struct CharacterLevelSection : AlignedUnaryTextSection
        {
            static constexpr bool expand_section = true;
            static constexpr bool draw_section_header_labels_rotated = true;

            static constexpr std::array column_names { "Character Level" };

            using AlignedUnaryTextSection::AlignedUnaryTextSection;
            explicit CharacterLevelSection(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
            }

            void update(const calculator::FullAttackOptions& attack_options)
            {
                (*this)[0] = attack_options.stats.character_level();
            }
        };
        export struct UpgradeLevelSection : AlignedUnaryTextSection
        {
            static constexpr bool expand_section = true;
            static constexpr bool draw_section_header_labels_rotated = true;
            
            static constexpr std::array column_names { "Upgrade Level" };

            using AlignedUnaryTextSection::AlignedUnaryTextSection;
            explicit UpgradeLevelSection(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
            }

            void update(const calculator::FullAttackOptions& attack_options)
            {
                (*this)[0] = attack_options.upgrade_level();
            }
        };

        export struct ColorSection : SectionBase<std::array<QVariant, 1>>
        {
            static constexpr std::array column_names { "Color" };

            using SectionBase::SectionBase;
            explicit ColorSection(const calculator::FullAttackOptions& attack_options)
            {
                (*this)[0] = QColor();
            }

            QVariant data(int column, int role) const
            {
                if (role == Qt::DecorationRole || role == Qt::EditRole || role == Qt::UserRole)
                    return (*this)[0];

                return {};
            }
        };

        struct BinaryTextSection : SectionBase<std::array<std::array<QVariant, 2>, 1>>
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
            explicit NameSection(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
            }

            void update(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                (*this)[0][0] = QString::fromStdString(weapon.qualified_name(attack_options.upgrade_level()));
                (*this)[0][1] = QString::fromStdString(weapon.full_name);
            }
        };
        export struct AffinitySection : BinaryTextSection
        {
            static constexpr std::array column_names { "Affinity" };

            using BinaryTextSection::BinaryTextSection;
            explicit AffinitySection(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                (*this)[0][0] = enum_to_display(weapon.affinity);
                (*this)[0][1] = std::to_underlying(weapon.affinity);
            }
        };
        export struct TypeSection : BinaryTextSection
        {
            static constexpr std::array column_names { "Type" };

            using BinaryTextSection::BinaryTextSection;
            explicit TypeSection(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                (*this)[0][0] = enum_to_display(weapon.type);
                (*this)[0][1] = std::to_underlying(weapon.type);
            }
        };

        struct AlignedBinaryTextSection : BinaryTextSection
        {
            QVariant data(int column, int role) const
            {
                if (role == Qt::TextAlignmentRole)
                    return alignment_center;
                
                return this->BinaryTextSection::data(column, role);
            }
        };
        export struct BaseGameDLCSection : AlignedBinaryTextSection
        {
            static constexpr std::array column_names { "Base Game\nDLC" };

            using AlignedBinaryTextSection::AlignedBinaryTextSection;
            explicit BaseGameDLCSection(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                (*this)[0][0] = weapon.dlc ? "DLC" : "Base Game";
                (*this)[0][1] = weapon.dlc;
            }
        };
        export struct TwoHandingSection : AlignedBinaryTextSection
        {
            static constexpr bool expand_section = true;
            static constexpr bool draw_section_header_labels_rotated = true;

            static constexpr std::array column_names { "Two-Handing" };

            using AlignedBinaryTextSection::AlignedBinaryTextSection;
            explicit TwoHandingSection(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
            }

            void update(const calculator::FullAttackOptions& attack_options)
            {
                (*this)[0][0] = attack_options.two_handing ? "yes" : "no";
                (*this)[0][1] = attack_options.two_handing;
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
            static inline const QString section_name = "Attribute Scaling";

            using EnumDataSection::EnumDataSection;
            explicit AttributeScalings(const calculator::Attack& attack)
            {
                this->update(attack);
            }
            explicit AttributeScalings(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
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
            void update(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                for (auto&& [attribute_scaling, scaling_tier, arr] : std::views::zip(
                    attack_options.attribute_scalings_at_upgrade_level(),
                    attack_options.calculate_scaling_tiers(),
                    *this))
                {
                    if (scaling_tier.empty())
                        arr[0] = format_number(attribute_scaling * 100);
                    else
                        arr[0] = format_number(attribute_scaling * 100) + " (" + QString::fromStdString(scaling_tier) + ")";
                    arr[1] = attribute_scaling * 100;
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
            explicit Requirements(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
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
            void update(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                for (auto&& [requirement, arr] : std::views::zip(
                    weapon.requirements,
                    *this))
                {
                    arr[0] = format_number(requirement);
                    arr[1] = requirement;
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
            explicit Stats(const calculator::FullAttackOptions& attack_options)
            {
                this->update(attack_options);
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
            void update(const calculator::FullAttackOptions& attack_options)
            {
                for (auto&& [stat, arr] : std::views::zip(
                    attack_options.stats.relevant_stats(),
                    *this))
                {
                    arr[0] = stat;
                    arr[1] = stat;
                }
            }
        };

        export template<calculator::AttackPowerType apt>
        struct AttackPowerTypeAttributeScalings : EnumDataSection<calculator::RelevantAttribute>
        {
            static constexpr auto attack_power_type = apt;
            static inline const auto section_name = enum_to_display(attack_power_type) + " Scaling";

            using EnumDataSection::EnumDataSection;
            explicit AttackPowerTypeAttributeScalings(const calculator::FullAttackOptions& attack_options)
            {
                auto&& weapon = attack_options.weapon.get();

                for (auto&& [attack_power_type_attribute_scalings, arr] : std::views::zip(
                    attack_options.attack_power_type_attribute_scalings(attack_power_type),
                    *this))
                {
                    arr[0] = format_number(attack_power_type_attribute_scalings);
                    arr[1] = attack_power_type_attribute_scalings;
                }
            }
        };
    }

    template<typename T>
    concept sparse = std::default_initializable<T>
        && std::constructible_from<T, const calculator::FullAttackOptions&>
        && requires (T t, const calculator::FullAttackOptions& attack_options) { t.update(attack_options); };

    export template<std::default_initializable...Args>
    struct BasicRow : _tuple_base<std::tuple<Args...>>
    {
        using _tuple_base = _tuple_base<std::tuple<Args...>>;
        using _tuple_base::_tuple_base;
        static constexpr auto sparse = (ui::sparse<Args> && ...);

        std::conditional_t<sparse, calculator::FullAttackOptions, calculator::Attack> attack { calculator::Weapon::dummy, {}, {} };

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
            std::array<bool, total_size> result{};
            for (auto [draw_rotated, section_index_offset, section_size] : std::views::zip(
                std::array{ Args::draw_section_header_labels_rotated... },
                section_index_offsets,
                section_sizes
            ))
                if (draw_rotated)
                    std::ranges::fill(result | std::views::drop(section_index_offset) | std::views::take(section_size), true);
            return result;
        }();
        static constexpr std::array expand_column = [](){
            std::array<bool, total_size> result{};
            std::array expand_section = { Args::expand_section... };
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
        explicit BasicRow(const decltype(attack)& attack) : _tuple_base(Args(attack)...), attack{ attack } { }

        void update(const decltype(attack)& attack)
        {
            this->attack = attack;
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

    export template<typename Row>
    struct RowModel : QAbstractTableModel
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
        bool setData(const QModelIndex &index, const QVariant &value, int role) override
        {
            if constexpr (requires { tuple_index_v<sections::ColorSection, Row>; })
            {
                if (index.column() == Row::section_index_offsets.at(tuple_index_v<sections::ColorSection, Row>) && role == Qt::EditRole)
                {
                    const QColor color = value.value<QColor>();

                    if (!color.isValid())
                        return false;

                    std::get<sections::ColorSection>(this->rows[index.row()])[0] = color;

                    emit dataChanged(index, index, { Qt::DecorationRole, Qt::UserRole });

                    return true;
                }
            }
            
            return false;
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
        void add_rows(std::ranges::sized_range auto&& rows)
        {
            this->beginInsertRows({}, this->rowCount(), this->rowCount() + std::ranges::size(rows) - 1);
            this->rows.append_range(std::forward<decltype(rows)>(rows));
            this->endInsertRows();
        }
        void update_row(int i, auto&& attack)
        {
            if (i < 0 || i >= this->rows.size())
                throw std::invalid_argument("index out of bounds");

            this->rows[i].update(std::move(attack));
            emit dataChanged(this->index(i, 0), this->index(i, this->columnCount() - 1));
        }
        void update_rows(std::ranges::sized_range auto&& attacks)
        {
            if (this->rows.size() != std::ranges::size(attacks))
                throw std::invalid_argument("attacks size must match rows size");

            for (auto&& [attack, row] : std::views::zip(attacks, this->rows))
                row.update(std::move(attack));
            emit dataChanged(this->index(0, 0), this->index(this->rowCount() - 1, this->columnCount() - 1));
        }
        void remove_rows(std::vector<int>& row_indices)
        {
            std::ranges::sort(row_indices);
            std::vector<Row> new_rows{};
            new_rows.reserve(this->rows.size() - row_indices.size());
            auto j = 0;
            for (auto [i, row] : this->rows | std::views::enumerate)
            {
                if (j < row_indices.size() && i == row_indices.at(j))
                {
                    ++j;
                    continue;
                }

                new_rows.emplace_back(std::move(row));
            }

            this->set_rows(std::move(new_rows));
        }
    };

    template <typename Row>
    void draw_column_group_separators(auto *device, const QHeaderView *header)
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

    template<typename Row>
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
            // Move normal section contents down
            QRect rect = rect_;
            rect.setTop(rect.top() + this->group_header_height());

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
            option.sortIndicator = QStyleOptionHeader::None;
            if (this->sortIndicatorSection() == logicalIndex)
                option.sortIndicator = this->sortIndicatorOrder() == Qt::AscendingOrder ? QStyleOptionHeader::SortUp : QStyleOptionHeader::SortDown;

            this->style()->drawControl(QStyle::CE_Header, &option, painter, this);

            QString text = this->model()->headerData(logicalIndex, this->orientation(), Qt::DisplayRole).toString();

            painter->setPen(option.palette.color(QPalette::Text));

            painter->translate(
                rotation == Rotation::CounterClockwise ? rect.left() : rect.right(),
                rotation == Rotation::CounterClockwise ? rect.bottom() : rect.top()
            );

            painter->rotate(rotation == Rotation::CounterClockwise ? -90 : 90);

            // Leave room for the indicator
            QRect textRect(0, 0, rect.height(), rect.width());

            painter->drawText(textRect, Qt::AlignCenter, text);

            painter->restore();
        }

        QSize sectionSizeFromContents(int logicalIndex) const override
        {
            QSize size = this->QHeaderView::sectionSizeFromContents(logicalIndex);

            // Width becomes height after rotation
            if (Row::draw_column_header_label_rotated[logicalIndex])
            {
                size.transpose();
                size.rheight() += 8;
            }
            else
                size.rheight() += this->group_header_height();

            // size.rheight() += this->group_header_height();

            return size;
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
            p.drawLine(bounds.left(), height, bounds.right(), height); // inbetween
            p.drawLine(bounds.left(), this->height() - 1, bounds.right(), this->height() - 1); // below

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

            draw_column_group_separators<Row>(this->viewport(), this);

            p.restore();
        }

    private:
        int group_header_height() const
        {
            return this->fontMetrics().height() * 2;
            return this->fontMetrics().height() + 8;
        }

        Rotation rotation;
    };

    template<typename Row>
    class RowSortFilterModel : public QSortFilterProxyModel
    {
        QSet<bool> base_game_dlc; // true = dlc, false = base game
        QSet<int> types;
        QSet<QString> base_names;
        QSet<int> affinities;

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
    };

    class ColorDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
        {
            Q_UNUSED(option);

            if (event->type() != QEvent::MouseButtonRelease)
                return false;

            auto *mouseEvent = static_cast<QMouseEvent *>(event);

            if (mouseEvent->button() != Qt::LeftButton)
                return false;

            QColorDialog dialog;
            dialog.setCurrentColor(index.data(Qt::DecorationRole).value<QColor>());

            if (dialog.exec() == QDialog::Accepted)
            {
                auto selected_color = dialog.selectedColor();
                model->setData(index, selected_color, Qt::EditRole);
                emit row_color_changed(index, selected_color);
            }
            
            return true;
        }
    
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.decorationAlignment = Qt::AlignCenter;
            opt.decorationSize = option.rect.size();
            option.widget->style()->drawControl(
                QStyle::CE_ItemViewItem,
                &opt,
                painter,
                option.widget
            );
        }

    signals:
        void row_color_changed(QModelIndex, QColor);
    };
    
    class WeaponTableBase : public QTableView
    {
        Q_OBJECT

    public:
        using QTableView::QTableView;

    signals:
        void add_new_to_plot();
        void edit_row(int);
        void add_selection_to_plot(const std::vector<std::reference_wrapper<const calculator::FullAttackOptions>>&);
        void remove_selection_from_plot(const std::vector<int>&);
        void row_color_changed(int, QColor);
    };

    export template<typename Row>
    class WeaponTable : public WeaponTableBase
    {
        bool is_plot_table;

        void resize_columns_to_contents()
        {
            const int columns = this->model->columnCount();

            this->header->setSectionResizeMode(QHeaderView::ResizeToContents);
            this->resizeColumnsToContents();

            std::vector<int> widths(columns);
            std::vector<int> visible_columns;
            std::vector<int> expandable_columns;

            int sum_total_width = 0;

            // calculate expandable columns and the widths of all visible columns
            for (int c = 0; c < columns; ++c)
            {
                if (this->header->isSectionHidden(c))
                    continue;

                widths[c] = this->columnWidth(c)/*this->header->sectionSize(c)*/;
                sum_total_width += widths[c];
                visible_columns.push_back(c);

                if (Row::expand_column[c])
                    expandable_columns.push_back(c);
            }

            this->header->setSectionResizeMode(QHeaderView::Interactive);

            if (sum_total_width <= 0 || visible_columns.empty())
                return;

            const int total_available_width = this->viewport()->width();

            if (sum_total_width >= total_available_width || expandable_columns.empty())
                return;

            int total_extra_width = total_available_width - sum_total_width;

            // sort expandable columns from narrowest to widest.
            std::ranges::sort(expandable_columns, [&](int a, int b) { return widths[a] < widths[b]; });

            // determine how many expandable columns to expand, also determine their expanded sum total width
            auto sum_total_width_of_expanded_columns = total_extra_width;
            auto expandable_columns_to_expand = 0;
            auto current_width = 0;
            for (auto c : expandable_columns)
            {
                if (current_width < widths[c])
                {
                    if (sum_total_width_of_expanded_columns < widths[c] * expandable_columns_to_expand)
                        break;

                    current_width = widths[c];
                }

                sum_total_width_of_expanded_columns += widths[c];
                ++expandable_columns_to_expand;
            }

            // if all expandable colums can be expanded to the highest expandable width (or further)
            if(expandable_columns_to_expand == expandable_columns.size())
            {
                // first, expand them to the highest expandable width
                auto highest_expandable_width = widths[expandable_columns.back()];
                for (int expandable_column : expandable_columns)
                    widths[expandable_column] = highest_expandable_width;

                // afterwards, distribute the remaining extra width proportionally among all visible columns
                auto remaining_extra_width = sum_total_width_of_expanded_columns - (highest_expandable_width * expandable_columns.size());
                auto current_sum_total_width = std::ranges::fold_left(widths, 0, std::plus{});
                auto remainder = remaining_extra_width;
                for (auto& c : visible_columns)
                {
                    auto& width = widths[c];
                    int add = width * remaining_extra_width / current_sum_total_width;
                    width += add;
                    remainder -= add;
                }

                // distribute leftover points (due to integer division), max 1 per width.
                for (auto& c : visible_columns)
                {
                    auto& width = widths[c];
                    if (remainder != 0)
                    {
                        ++width;
                        --remainder;
                    }

                    this->header->resizeSection(c, width);
                }
            }
            else
            {
                // otherwise expand the subset of expandable columns to the sum total width previously calculated
                auto each = sum_total_width_of_expanded_columns / expandable_columns_to_expand;
                auto remainder = sum_total_width_of_expanded_columns % expandable_columns_to_expand;
                for (auto i = 0; i < expandable_columns_to_expand; ++i)
                {
                    auto c = expandable_columns[i];
                    auto new_width = each + (i < remainder ? 1 : 0);
                    if (new_width != widths[c])
                        this->header->resizeSection(c, new_width);
                }
            }
        }

        void show_header_context_menu(const QPoint &pos)
        {
            QMenu menu;

            menu.addAction(
                QString::fromStdString("Adjust Column Widths to Contents"),
                [&](){ this->resize_columns_to_contents(); }
            );
            menu.addSeparator();

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
                });
            }

            menu.exec(this->mapToGlobal(pos));
        }
        void show_table_context_menu(const QPoint &pos)
        {
            auto row_indices = this->selectionModel()->selectedRows()
                | std::views::transform([this](const QModelIndex& index){
                    return this->proxy_model->mapToSource(index).row();
                })
                | std::ranges::to<std::vector>();

            auto selection_name = row_indices.size() == 1
                ? this->model->rows.at(row_indices.front()).attack.weapon.get().full_name
                : std::format("Selection ({})", row_indices.size());

            QMenu menu(this);

            if (is_plot_table)
            {
                menu.addAction("Add New Dataset", [&](){
                    emit this->add_new_to_plot();
                });
            }

            if(row_indices.size() > 0)
            {
                menu.addSeparator();

                if (is_plot_table)
                {
                    if (row_indices.size() == 1)
                    {
                        menu.addAction(
                            QString::fromStdString(std::format("Edit {}", selection_name)),
                            [&](){ emit this->edit_row(row_indices.front()); }
                        );
                    }

                    menu.addAction(
                        QString::fromStdString(std::format("Remove {} from Plot", selection_name)),
                        [&](){ emit this->remove_selection_from_plot(row_indices); }
                    );
                }
                else
                {
                    menu.addAction(
                        QString::fromStdString(std::format("Add {} to Plot", selection_name)),
                        [&](){
                            emit this->add_selection_to_plot(row_indices
                                | std::views::transform([this](auto row_index){
                                    return std::cref<calculator::FullAttackOptions>(this->model->rows.at(row_index).attack);
                                })
                                | std::ranges::to<std::vector>()
                            );
                        }
                    );
                }
                
                menu.addSeparator();
                menu.addAction(
                    QString::fromStdString(std::format("Show {} on Fandom", selection_name)),
                    [&](){
                        for (auto row_index : row_indices)
                            QDesktopServices::openUrl(QUrl(QString::fromStdString(this->model->rows.at(row_index).attack.weapon.get().fandom_url())));
                    }
                );
                menu.addAction(
                    QString::fromStdString(std::format("Show {} on Fextralife", selection_name)),
                    [&](){
                        for (auto row_index : row_indices)
                            QDesktopServices::openUrl(QUrl(QString::fromStdString(this->model->rows.at(row_index).attack.weapon.get().fextralife_url())));
                    }
                );
            }

            menu.addSeparator();
            menu.addAction(
                QString::fromStdString("Adjust Column Widths to Contents"),
                [&](){ this->resize_columns_to_contents(); }
            );

            menu.exec(this->viewport()->mapToGlobal(pos));
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QTableView::paintEvent(event);

            if (this->model->rowCount() == 0)
            {
                QPainter painter(this->viewport());
                painter.setPen(Qt::gray);
                painter.drawText(this->viewport()->rect(), Qt::AlignCenter, this->placeholder_string);
            }
            else
                draw_column_group_separators<Row>(this->viewport(), this->horizontalHeader());
        }
    
    public:
        QString placeholder_string;
        RowModel<Row>* model = new RowModel<Row>(this);
        RowSortFilterModel<Row>* proxy_model = new RowSortFilterModel<Row>(this);
        RotatedHeaderView<Row>* header = new RotatedHeaderView<Row>(Qt::Horizontal, RotatedHeaderView<Row>::Rotation::Clockwise, this);

        explicit WeaponTable(bool is_plot_table, QString placeholder_string, QWidget *parent = nullptr)
            : is_plot_table{is_plot_table}, placeholder_string{placeholder_string}, WeaponTableBase(parent)
        {
            // view
            this->setFrameStyle(QFrame::Box);
            this->setSortingEnabled(true);
            this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            this->setSelectionBehavior(QAbstractItemView::SelectRows);
            this->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &QTableView::customContextMenuRequested, this, &WeaponTable::show_table_context_menu);

            // color delegate
            if constexpr (requires { tuple_index_v<sections::ColorSection, Row>; })
            {
                auto delegate = new ColorDelegate(this);
                this->setItemDelegateForColumn(Row::section_index_offsets.at(tuple_index_v<sections::ColorSection, Row>), delegate);
                connect(delegate, &ColorDelegate::row_color_changed, [this](QModelIndex index, QColor color){
                    emit row_color_changed(this->proxy_model->mapToSource(index).row(), color);
                });
            }

            // model
            this->proxy_model->setSourceModel(this->model);
            this->setModel(this->proxy_model);
            this->proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
            this->proxy_model->setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

            // header
            this->setHorizontalHeader(this->header);
            this->header->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this->header, &QHeaderView::customContextMenuRequested, this, &WeaponTable::show_header_context_menu);

            // sorting
            auto sort_column = Row::section_index_offsets.at(tuple_index_v<sections::BaseNameSection, Row>);
            auto sort_order = Qt::SortOrder::AscendingOrder;
            this->proxy_model->sort(sort_column, sort_order);
            this->header->setSortIndicator(sort_column, sort_order);

            this->set_section_hidden<sections::BaseNameSection>(true);
            this->set_section_hidden<sections::BaseGameDLCSection>(true);
            [&](auto){
                static constexpr auto [...apt] = enumerators_of<calculator::AttackPowerType>();
                (this->set_section_hidden<sections::AttackPowerTypeAttributeScalings<apt>>(true), ...);
            }(1);
        }

        template<typename ColumnType>
        void set_section_hidden(bool hide)
        {
            if constexpr (requires { tuple_index_v<ColumnType, Row>; })
                this->header->set_section_hidden(tuple_index_v<ColumnType, Row>, hide);
        }
    };
}

#include "weapons_table.moc"