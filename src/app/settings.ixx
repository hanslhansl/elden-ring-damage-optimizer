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


#define DEFINE_SPINBOX_SETTING(NAME, DEFAULT_VALUE, MINIMUM_VALUE, MAXIMUM_VALUE) \
    public: \
        int NAME = settings.value(#NAME, DEFAULT_VALUE).toInt();   \
        Q_SIGNAL void NAME##_changed(int new_value);  \
    private: \
        QSpinBox* NAME##_widget = [this](){ \
            auto spinbox = new QSpinBox{};  \
            spinbox->setMinimum(MINIMUM_VALUE); \
            spinbox->setMaximum(MAXIMUM_VALUE); \
            spinbox->setValue(this->NAME);   \
            connect(spinbox, &QSpinBox::valueChanged, [this](int new_value){ this->NAME = new_value; });   \
            connect(spinbox, &QSpinBox::valueChanged, [this](int new_value){ this->set_value(#NAME, new_value); });   \
            connect(spinbox, &QSpinBox::valueChanged, this, &Settings::NAME##_changed);   \
            this->widgets.push_back(spinbox); \
            this->names.push_back(#NAME); \
            return spinbox; \
        }()

#define DEFINE_CHECKBOX_SETTING(NAME, DEFAULT_VALUE) \
    public: \
        bool NAME = settings.value(#NAME, DEFAULT_VALUE).toBool();   \
        Q_SIGNAL void NAME##_changed(bool new_value);  \
    private: \
        QCheckBox* NAME##_widget = [this](){ \
            auto checkbox = new QCheckBox{};  \
            checkbox->setChecked(this->NAME);   \
            connect(checkbox, &QCheckBox::toggled, [this](bool new_value){ this->NAME = new_value; });   \
            connect(checkbox, &QCheckBox::toggled, [this](bool new_value){ this->set_value(#NAME, new_value); });   \
            connect(checkbox, &QCheckBox::toggled, this, &Settings::NAME##_changed);   \
            this->widgets.push_back(checkbox); \
            this->names.push_back(#NAME); \
            return checkbox; \
        }()


namespace erdo::ui
{
    export class Settings : public QObject
    {
        Q_OBJECT


        std::vector<QString> names{};
        std::vector<QWidget*> widgets{};

        void set_value(const QString& name, const QVariant& value)
        {
            this->settings.setValue(name, value);
            this->settings.sync();
        }

    public:
        QSettings settings{ "hanslhansl", "elden-ring-damage-optimizer" };
        QDialog* dialog = new QDialog{};

        Settings();

        DEFINE_SPINBOX_SETTING(decimal_places, 3, 0, 10);
        DEFINE_CHECKBOX_SETTING(display_base_names_instead_of_full_names, false);
        DEFINE_CHECKBOX_SETTING(sort_by_base_names_instead_of_full_names, false);
    };

    export Settings& settings()
    {
        static Settings instance{};
        return instance;
    } 

    export QString format_float(double x)
    {
        auto s = QString::number(x, 'f', settings().decimal_places);

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

erdo::ui::Settings::Settings()
{
    auto layout = new QFormLayout{};

    for (auto&& [name, widget] : std::views::zip(this->names, this->widgets))
        layout->addRow(string_to_display(name), widget);

    this->dialog->setLayout(layout);
    this->dialog->setWindowTitle("settings");
}


#include "settings.moc"