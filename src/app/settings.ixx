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

    class SettingBuilderBase : public QObject
    {
        Q_OBJECT

    public:
        Q_SIGNAL void changed();

        static constexpr auto changed_member_pointer = &SettingBuilderBase::changed;
    };

    template<typename T>
    struct SettingBuilder : SettingBuilderBase
    {

        T::value_type value;

        SettingBuilder(Settings& settings);

        operator const typename T::value_type&() const
        {
            return this->value;
        }
    };

    template<typename T>
    struct SpinBoxSetting
    {
        using value_type = int;
        using widget_type = QSpinBox;
        inline static const auto signal = &QSpinBox::valueChanged;

        static void initialize(widget_type* spinbox, value_type value)
        {
            spinbox->setMinimum(T::minimum_value);
            spinbox->setMaximum(T::maximum_value);
            spinbox->setValue(value);
        }
    };
    struct CalculationDelay : SpinBoxSetting<CalculationDelay>
    {
        using typename SpinBoxSetting<CalculationDelay>::value_type;

        constexpr static std::string_view section_name = "General";
        constexpr static std::string_view name = "calculation_delay";
        constexpr static std::string_view display_name = "Calculation Delay (ms)";

        constexpr static value_type default_value = 500;
        constexpr static value_type minimum_value = 100;
        constexpr static value_type maximum_value = 5000;
    };
    struct AttributeLevelLimit : SpinBoxSetting<AttributeLevelLimit>
    {
        using typename SpinBoxSetting<AttributeLevelLimit>::value_type;

        constexpr static std::string_view section_name = "Elden Ring";
        constexpr static std::string_view name = "attribute_level_limit";
        constexpr static std::string_view display_name = "Attribute Level Limit (Ingame: 99)";

        constexpr static value_type default_value = 99;
        constexpr static value_type minimum_value = 0;
        constexpr static value_type maximum_value = 148;
    };
    struct DecimalPlaces : SpinBoxSetting<DecimalPlaces>
    {
        using typename SpinBoxSetting<DecimalPlaces>::value_type;

        constexpr static std::string_view section_name = "Weapon Table";
        constexpr static std::string_view name = "decimal_places";
        constexpr static std::string_view display_name = "Decimal Places";

        constexpr static value_type default_value = 3;
        constexpr static value_type minimum_value = 0;
        constexpr static value_type maximum_value = 10;
    };

    struct CheckBoxSetting
    {
        using value_type = bool;
        using widget_type = QCheckBox;
        inline static const auto signal = &QCheckBox::toggled;

        static void initialize(widget_type* checkbox, value_type value)
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
        friend class SettingBuilder;

    public:
        SettingBuilder<CalculationDelay> calculation_delay{ *this };
        
        SettingBuilder<AttributeLevelLimit> attribute_level_limit{ *this };

        SettingBuilder<DecimalPlaces> decimal_places{ *this };

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
            QObject::connect(buttons, &QDialogButtonBox::clicked, [this, buttons](QAbstractButton* button) {
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
}

template<typename T>
erdo::ui::SettingBuilder<T>::SettingBuilder(Settings& settings)
{
    if (!settings.is_initialized)
        return;
    
    auto widget = new T::widget_type{};
    this->value = settings.qsettings->value(T::name, T::default_value).template value<typename T::value_type>();
    T::initialize(widget, this->value);

    connect(widget, T::signal, [&](T::value_type new_value){
        this->value = new_value;
        settings.qsettings->setValue(T::name, new_value);
        settings.qsettings->sync();
        emit this->changed();
    });

    settings.widgets.push_back({{T::section_name, T::display_name}, widget});
}

void erdo::ui::Settings::initialize()
{
    std::destroy_at(&erdo::ui::settings);
    std::construct_at(&erdo::ui::settings);
}

#include "settings.moc"