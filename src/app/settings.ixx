module;
#include <QSettings>
#include <QSpinBox>
#include <QCheckBox>
#include <QObject>
#include <QRegularExpression>
#include <QFormLayout>
#include <QDialog>
#include <QLabel>
#include <QApplication>
#include <QPushButton>
#include <QTabWidget>
#include <QDialogButtonBox>
export module erdo.ui.settings;

import std;
import erdo;

namespace erdo::ui
{
    class Settings;

    class SettingMemberBase : public QObject
    {
        Q_OBJECT

    public:
        Q_SIGNAL void changed();

        static constexpr auto changed_member_pointer = &SettingMemberBase::changed;
    };

    template<typename T>
    struct SettingMember : SettingMemberBase
    {
        T value;

        template<typename U> requires std::same_as<T, typename U::value_type>
        SettingMember(U&& member_info);

        operator const T&() const
        {
            return this->value;
        }
    };

    struct SpinBoxSetting
    {
        using value_type = int;
        using widget_type = QSpinBox;
        inline static const auto signal = &QSpinBox::valueChanged;

        std::string_view section_name;
        std::string_view name;
        std::string_view display_name;

        value_type default_value;
        value_type minimum_value;
        value_type maximum_value;

        void initialize(widget_type* spinbox, value_type value) const
        {
            spinbox->setMinimum(this->minimum_value);
            spinbox->setMaximum(this->maximum_value);
            spinbox->setValue(value);
        }
    };

    struct CheckBoxSetting
    {
        using value_type = bool;
        using widget_type = QCheckBox;
        inline static const auto signal = &QCheckBox::toggled;

        std::string_view section_name;
        std::string_view name;
        std::string_view display_name;

        value_type default_value;

        void initialize(widget_type* checkbox, value_type value) const
        {
            checkbox->setChecked(value);
        }
    };

    class Settings
    {
        bool is_initialized = false;

        std::vector<std::pair<std::array<std::string_view, 2>, QWidget*>> widgets{};
        std::unique_ptr<QSettings> qsettings{};
        std::unique_ptr<QDialog> dialog{};

        template<typename T>
        friend class SettingMember;

    public:
        SettingMember<int> decimal_places{ SpinBoxSetting{
            .section_name = "General",
            .name = "decimal_places",
            .display_name = "Decimal Places",
            .default_value = 3,
            .minimum_value = 0,
            .maximum_value = 10
        } };
        SettingMember<bool> show_attack_power_split{ CheckBoxSetting{
            .section_name = "General",
            .name = "show_attack_power_split",
            .display_name = "Show Attack Power Split",
            .default_value = false
        } };

        SettingMember<int> plot_data_line_width{ SpinBoxSetting{
            .section_name = "Plot",
            .name = "plot_data_line_width",
            .display_name = "Data Line Width",
            .default_value = 2,
            .minimum_value = 0,
            .maximum_value = 20
        } };
        SettingMember<int> plot_point_diameter{ SpinBoxSetting{
            .section_name = "Plot",
            .name = "plot_point_diameter",
            .display_name = "Point Diameter",
            .default_value = 8,
            .minimum_value = 0,
            .maximum_value = 20
        } };
        SettingMember<int> plot_axis_line_width{ SpinBoxSetting{
            .section_name = "Plot",
            .name = "plot_axis_line_width",
            .display_name = "Axis Line Width",
            .default_value = 2,
            .minimum_value = 0,
            .maximum_value = 20
        } };
        SettingMember<int> plot_grid_line_width{ SpinBoxSetting{
            .section_name = "Plot",
            .name = "plot_grid_line_width",
            .display_name = "Grid Line Width",
            .default_value = 1,
            .minimum_value = 0,
            .maximum_value = 20
        } };

        Settings(int) {};
        Settings() : is_initialized{ true }, dialog{ std::make_unique<QDialog>() }, qsettings{ std::make_unique<QSettings>() }
        {
            auto tab_widget = new QTabWidget{};

            std::vector<std::string_view> section_names;
            std::unordered_map<std::string_view, std::vector<std::size_t>> sections;
            for (std::size_t i = 0; i < this->widgets.size(); ++i)
            {
                auto key = this->widgets[i].first[0];

                if (sections.find(key) == sections.end())
                    section_names.push_back(key);

                sections[key].push_back(i);
            }

            for (auto&& section_name : section_names)
            {
                auto tab = new QWidget{};
                auto form_layout = new QFormLayout{};
                tab->setLayout(form_layout);

                tab_widget->addTab(tab, QString::fromStdString(std::string(section_name)));

                for (auto i : sections[section_name])
                {
                    auto&& [key, widget] = this->widgets[i];
                    auto&& [_, display_name] = key;

                    form_layout->addRow(QString::fromStdString(std::string(display_name) + ":"), widget);
                }
            }

            auto layout = new QVBoxLayout{};
            layout->addWidget(tab_widget);

            QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Reset, this->dialog.get());
            QObject::connect(buttons, &QDialogButtonBox::accepted, this->dialog.get(), &QDialog::accept);
            QObject::connect(buttons, &QDialogButtonBox::clicked, this->dialog.get(), [this, buttons](QAbstractButton* button) {
                if (buttons->buttonRole(button) == QDialogButtonBox::ResetRole)
                {
                    this->dialog->reject();
                    this->qsettings->clear();
                    QApplication::quit();
                }
            });
            layout->addWidget(buttons);

            this->dialog->setLayout(layout);
            this->dialog->setWindowTitle("Settings");
        }

        void show()
        {
            this->dialog->exec();
        }

        static void initialize();
    };
    export Settings settings { 1 };

    export QString format_float(double x)
    {
        auto s = QString::number(x, 'f', settings.decimal_places.value);

        // Remove trailing zeros
        if (s.contains('.'))
            while (s.endsWith('0'))
                s.chop(1);

        // Remove trailing decimal point
        if (s.endsWith('.'))
            s.chop(1);

        return s;
    }

    export struct {
        template<typename E> requires std::is_enum_v<E>
        static QString operator()(E e)
        {
            QStringList words = QString::fromStdString(std::string(enum_to_string(e))).split(QRegularExpression("[_ ]+"), Qt::SkipEmptyParts);

            for (QString &word : words)
            {
                word = word.toLower();
                word[0] = word[0].toUpper();
            }

            return words.join(' ');
        }
    } enum_to_display;

    export template<typename T>
    auto format_number(T x)
    {
        if (x == 0)
            return QString("\u2012");
        if constexpr (std::integral<T>)
            return QString::number(x);
        return format_float(x);
    };
    export template<typename T>
    auto format_number_pair(T x, T y)
    {
        if (x == 0 && y == 0)
        {
            return QString("\u2012");
        }
        else if (y < 0)
        {
            if constexpr (std::integral<T>)
                return QString::number(x) + " - " + QString::number(-y);
            return format_float(x) + " - " + format_float(-y);
        }
        else
        {
            if constexpr (std::integral<T>)
                return QString::number(x) + " + " + QString::number(y);
            return format_float(x) + " + " + format_float(y);
        }
    };
}

template<typename T>
template<typename U> requires std::same_as<T, typename U::value_type>
erdo::ui::SettingMember<T>::SettingMember(U&& member_info)
{
    if (!settings.is_initialized)
        return;
    
    auto widget = new U::widget_type{};
    this->value = settings.qsettings->value(member_info.name, member_info.default_value).template value<T>();
    member_info.initialize(widget, this->value);

    settings.widgets.push_back({{member_info.section_name, member_info.display_name}, widget});

    connect(widget, U::signal, this, [this, member_info = std::move(member_info)](T new_value){
        this->value = new_value;
        settings.qsettings->setValue(member_info.name, new_value);
        settings.qsettings->sync();
        emit this->changed();
    });
}

void erdo::ui::Settings::initialize()
{
    std::destroy_at(&erdo::ui::settings);
    std::construct_at(&erdo::ui::settings);
}

#include "settings.moc"