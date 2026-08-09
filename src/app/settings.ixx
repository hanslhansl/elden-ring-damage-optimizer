module;
#include <QSettings>
#include <QSpinBox>
#include <QCheckBox>
#include <QObject>
#include <QRegularExpression>
#include <QFormLayout>
#include <QDialog>
export module erdo.ui.settings;

import std;


namespace erdo::ui
{
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

        SettingBuilder(QSettings* settings, QFormLayout* layout);

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
        static constexpr auto signal = &QSpinBox::valueChanged;

        static void initialize(widget_type* spinbox, value_type value)
        {
            spinbox->setMinimum(T::minimum_value);
            spinbox->setMaximum(T::maximum_value);
            spinbox->setValue(value);
        }
    };
    struct DecimalPlaces : SpinBoxSetting<DecimalPlaces>
    {
        using typename SpinBoxSetting<DecimalPlaces>::value_type;

        constexpr static std::string_view name = "decimal_places";
        constexpr static std::string_view display_name = "decimal places";

        constexpr static value_type default_value = 3;
        constexpr static value_type minimum_value = 0;
        constexpr static value_type maximum_value = 10;
    };

    struct CheckBoxSetting
    {
        using value_type = bool;
        using widget_type = QCheckBox;
        static constexpr auto signal = &QCheckBox::toggled;

        static void initialize(widget_type* checkbox, value_type value)
        {
            checkbox->setChecked(value);
        }
    };
    struct DisplayBaseNamesInsteadOfFullNames : CheckBoxSetting
    {
        using typename CheckBoxSetting::value_type;

        constexpr static std::string_view name = "display_base_names_instead_of_full_names";
        constexpr static std::string_view display_name = "display base names instead of full names";

        constexpr static value_type default_value = false;
    };
    struct SortByBaseNamesInsteadOfFullNames : CheckBoxSetting
    {
        using typename CheckBoxSetting::value_type;

        constexpr static std::string_view name = "sort_by_base_names_instead_of_full_names";
        constexpr static std::string_view display_name = "sort by base names instead of full names";

        constexpr static value_type default_value = false;
    };
    struct HideBaseGameDLCColumn : CheckBoxSetting
    {
        using typename CheckBoxSetting::value_type;

        constexpr static std::string_view name = "hide_base_game_dlc_column";
        constexpr static std::string_view display_name = "hide base game/dlc column";

        constexpr static value_type default_value = false;
    };

    class Settings
    {
        QFormLayout* layout{};
        std::unique_ptr<QSettings> settings{};

    public:
        QDialog* dialog{};

        SettingBuilder<DecimalPlaces> decimal_places{ settings.get(), layout };
        SettingBuilder<DisplayBaseNamesInsteadOfFullNames> display_base_names_instead_of_full_names{ settings.get(), layout };
        SettingBuilder<SortByBaseNamesInsteadOfFullNames> sort_by_base_names_instead_of_full_names{ settings.get(), layout };
        SettingBuilder<HideBaseGameDLCColumn> hide_base_game_dlc_column{ settings.get(), layout };

        Settings(int) {};
        Settings() : layout{ new QFormLayout{} }, dialog{ new QDialog{} }, settings{ std::make_unique<QSettings>() }
        {
            this->dialog->setLayout(layout);
            this->dialog->setWindowTitle("settings");
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
        static QString operator()(const QString& str)
        {
            QStringList words = str.split(QRegularExpression("[_ ]+"), Qt::SkipEmptyParts);

            for (QString &word : words)
                word = word.toLower();

            return words.join(' ');
        }
        static QString operator()(const char* str)
        {
            return operator()(QString(str));
        }
        static QString operator()(const std::string& str)
        {
            return operator()(QString::fromStdString(str));
        }
        static QString operator()(std::string_view str)
        {
            return operator()(std::string(str));
        }
    } string_to_display;

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
erdo::ui::SettingBuilder<T>::SettingBuilder(QSettings* settings, QFormLayout* layout)
{
    if (settings == nullptr || layout == nullptr)
        return;
    
    auto widget = new T::widget_type{};
    this->value = settings->value(T::name, T::default_value).template value<typename T::value_type>();
    T::initialize(widget, this->value);
    layout->addRow(string_to_display(T::display_name), widget);

    connect(widget, T::signal, [settings, this](T::value_type new_value){
        this->value = new_value;
        settings->setValue(T::name, new_value);
        settings->sync();
        emit this->changed();
    });
}

void erdo::ui::Settings::initialize()
{
    std::destroy_at(&erdo::ui::settings);
    std::construct_at(&erdo::ui::settings);
}

#include "settings.moc"