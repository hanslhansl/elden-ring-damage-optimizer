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

#include <KDChartChart>
#include <KDChartWidget>
#include <KDChartCartesianAxis>
#include <KDChartLineDiagram>
#include <KDChartGridAttributes>
#include <KDChartPlotter>
#include <KDChartDataValueAttributes>
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

    class SearchableComboBox : public QComboBox
    {
        Q_OBJECT

    public:
        explicit SearchableComboBox(QWidget* parent = nullptr)
            : QComboBox(parent)
            , m_filterModel(new QSortFilterProxyModel(this))
            , m_completer(new QCompleter(m_filterModel, this))
            , m_lastValidIndex(-1)
            , m_updatePending(false)
            , m_internalUpdate(false)
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

        void restoreSearchText(
            const QString& text,
            int cursorPosition)
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

            return BasicRow<true,
                sections::ColorSection,
                sections::NameSection,
                sections::BaseNameSection,
                sections::AffinitySection,
                sections::TypeSection,
                sections::BaseGameDLCSection,

                sections::UpgradeLevelSection,
                sections::TwoHandingSection,
                sections::CharacterLevelSection,
                sections::Stats,
                sections::Requirements,
                sections::AttributeScalings
            >{};
        }(0));

        std::shared_ptr<const std::vector<calculator::Weapon>> active_weapon_data{};

        QStandardItemModel *model;
        KDChart::Chart* chart;
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
                this->model->setData(this->model->index(j, 0), universal_xs[j]);

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

                auto dataset_color = std::get<sections::ColorSection>(this->weapon_table->model->rows.at(wi))[0].value<QColor>();
                auto pen = this->plotter->pen(i);
                pen.setColor(dataset_color);
                this->plotter->setPen(i, pen);

                this->model->setHeaderData(
                    column,
                    Qt::Horizontal,
                    this->weapon_table->model->rows.at(wi).attack.weapon.get().full_name.data()
                );

                // this->model->setData(index,
                //     QString("<table><tr><td>Row</td><td>Column</td>"
                //     "<td>Value</td></tr>"
                //     "<tr><th>%1</th><th>%2</th><th>%3</th></tr></table>")
                //     .arg(0)
                //     .arg(column)
                //     .arg(this->model->data(index).toInt()),
                //     Qt::ToolTipRole
                // );

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
            connect(base_name_combobox, &QComboBox::currentIndexChanged, [&](int index){
                if (index < 0)
                    throw std::runtime_error("Invalid base name index");

                const QString text = base_name_combobox->itemText(index);

                possible_weapons.clear();
                possible_weapons.insert_range(
                    *this->active_weapon_data
                    | std::views::filter([&](const calculator::Weapon& w) {
                        return w.base_name.data() == text;
                    })
                );
                if (possible_weapons.empty())
                    throw std::runtime_error(std::format("No weapons found for base name: {}", text.toStdString()));

                auto blocker = QSignalBlocker(affinity_combobox);
                affinity_combobox->clear();
                for (auto&& affinity : possible_weapons
                    | std::views::transform(&calculator::Weapon::affinity)
                )
                    affinity_combobox->addItem(enum_to_display(affinity), std::to_underlying(affinity));

                set_weapon();
            });

            // connect to weapon affinity
            connect(affinity_combobox, &QComboBox::currentIndexChanged, set_weapon);

            // connect to upgrade level
            connect(upgrade_level_spinbox, &QSpinBox::valueChanged, [&attack_options](int value){
                attack_options.upgrade_levels.at(attack_options.weapon.get().upgrade_level_index) = value;
            });

            // connect to two-handing
            connect(two_handing_checkbox, &QCheckBox::toggled, [&attack_options](bool checked){
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

                connect(attribute_spinbox, &QSpinBox::valueChanged, [&, i](int value) {
                    attack_options.stats.at(i) = value;
                });
            }

            form->addRow(buttons);

            // populate weapon base name
            for (auto&& base_name : *this->active_weapon_data
                | std::views::transform(&calculator::Weapon::base_name)
                | std::ranges::to<std::set>()
            )
                base_name_combobox->addItem(QString::fromStdString(base_name));

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
                    std::get<sections::ColorSection>(row)[0] = get_distinctive_color();
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
            this->weapon_table->set_section_hidden<sections::AttributeScalings>(true);
            connect(this->weapon_table, &WeaponTable<Row>::remove_selection_from_plot, this, &PlotTab::remove_datasets);
            connect(this->weapon_table, &WeaponTable<Row>::row_color_changed, [this](int wi, QColor color){ this->update_datasets(wi, 1); });
            connect(this->weapon_table, &WeaponTable<Row>::edit_row, this, &PlotTab::edit_dataset_dialog);
            connect(this->weapon_table, &WeaponTable<Row>::add_new_to_plot, this, &PlotTab::add_new_dataset_dialog);

            // plotting backend
            this->model = new QStandardItemModel(this);
            // this->model->setRowCount(1);
            this->model->setColumnCount(2);
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

            // Initialize the model with dummy data (KDChart::Plotter is buggy...)
            this->model->setData(this->model->index(0, 0), 0.);
            this->model->setData(this->model->index(0, 0 + 1), 0.);
            this->model->setData(this->model->index(1, 0), 1.);
            this->model->setData(this->model->index(1, 0 + 1), 1.);
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
            this->chart = new KDChart::Chart(this);
            upper_layout->addWidget(this->chart);
            this->chart->coordinatePlane()->replaceDiagram(this->plotter);
            this->chart->coordinatePlane()->globalGridAttributes().setSubGridVisible(false);

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
            connect(&settings.plot_data_line_width, settings.plot_data_line_width.changed_member_pointer, set_data_line_width);

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
            connect(&settings.plot_grid_line_width, settings.plot_grid_line_width.changed_member_pointer, set_grid_line_width);

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
            connect(&settings.plot_axis_line_width, settings.plot_axis_line_width.changed_member_pointer, set_axis_line_width);

            this->addWidget(this->weapon_table);
            this->setSizes({600, 400});
        }
    };
}

#include "plot_tab.moc"