#pragma once

#include "include.hpp"
#include "calculator.hpp"

namespace ui
{
    enum
    {
        ID_LOAD_REGULATION = 1,
        ID_GENERATE_REGULATION,

        ID_FILTER_DLC,
        ID_FILTER_TYPES,
        ID_FILTER_AFFINITIES,
        ID_FILTER_BASE_NAMES,
    };

    class MyApp : public wxApp
    {
    public:
        bool OnInit() override;
    };

    class StatsPanel : public wxPanel
    {
        //static constexpr std::array attribute_names = { "vigor", "mind", "endurance", "strength", "dexterity", "intelligence", "faith", "arcane" };
        static constexpr std::array attribute_names = { "vig", "min", "end", "str", "dex", "int", "fai", "arc" };
        inline static const std::array<wxString, 10> starting_class_names = {"hero", "bandit", "astrologer", "warrior", "prisoner", "confessor", "wretch", "vagabond", "prophet", "samurai"};
        static constexpr std::array<std::array<int, attribute_names.size()>, starting_class_names.size()> starting_class_stats{{
            { 14,  9, 12, 16,  9,  7,  8, 11 }, // hero
            { 10, 11, 10,  9, 13,  9,  8, 14 }, // bandit
            {  9, 15,  9,  8, 12, 16,  7,  9 },	// astrologer
            { 11, 12, 11, 10, 16, 10,  8,  9 },	// warrior
            { 11, 12, 11, 11, 14, 14,  6,  9 },	// prisoner
            { 10, 13, 10, 12, 12,  9, 14,  9 },	// confessor
            { 10, 10, 10, 10, 10, 10, 10, 10 }, // wretch
            { 15, 10, 11, 14, 13,  9,  9,  7 },	// vagabond
            { 10, 14,  8, 11, 10,  7, 16, 10 },	// prophet
            { 12, 11, 13, 12, 15,  9,  8,  8 },	// samurai
        } };

        wxChoice* choice;
        std::array<wxSpinCtrl*, attribute_names.size()> attribute_ctrls;
        wxSpinCtrl* player_level_ctrl;
        wxStaticText* attribute_points_text;
        wxStaticText* stat_variations_text;

        void update_stat_variations();

        void OnChange(wxCommandEvent& event);
        void OnChoiceSelected(wxCommandEvent& event);

    public:
        StatsPanel(wxWindow* parent);
    };

    class FilterPanel : public wxPanel
    {
        wxListBox* dlc_weapons_list;
        wxListBox* types_list;
        wxListBox* affinities_list;
        wxListBox* base_names_list;
        wxStaticText* filtered_weapons_text;

        calculator::weapon_container* weapon_container{};
        calculator::weapon::all_filter_options all_filter_options{};
        calculator::weapon::filter filter{};
        calculator::filtered_weapons filtered_weapons{};

        void update_filtered_weapons();

        void onListBox(wxCommandEvent& event);

    public:

        FilterPanel(wxWindow* parent);

        void update_filter_options(calculator::weapon_container& weapon_container);

        const calculator::weapon::filter& get_filter() const;
    };

    class Upgrade_LevelPanel : public wxPanel
    {
        wxSpinCtrl* normal_upgrade_level_ctrl;
        wxSpinCtrl* somber_upgrade_level_ctrl;

    public:
        Upgrade_LevelPanel(wxWindow* parent);
    };

    class OptimizePanel : public wxPanel
    {
        inline static const std::array<wxString, calculator::DamageTypes::_size()> damage_type_names =
            []() {  std::array<wxString, calculator::DamageTypes::_size()> ret{};
            std::ranges::transform(calculator::DamageTypes::_names(), ret.begin(), [](const char* p) { return p; }); return ret; }();

        inline static const std::array<wxString, calculator::StatusTypes::_size()> status_type_names =
            []() {  std::array<wxString, calculator::StatusTypes::_size()> ret{};
            std::ranges::transform(calculator::StatusTypes::_names(), ret.begin(), [](const char* p) { return p; }); return ret; }();

        wxRadioButton* total_attack_power_button;

        wxRadioButton* individual_attack_power_button;
        wxListBox* individual_attack_power_list;

        wxRadioButton* status_effect_button;
        wxListBox* status_effect_list;

        wxRadioButton* spell_scaling_button;

    public:
        OptimizePanel(wxWindow* parent);
    };

    class MyFrame : public wxFrame
    {
        std::unique_ptr<calculator::weapon_container> weapon_container;
        StatsPanel* stats_panel;
        FilterPanel* filter_panel;
        Upgrade_LevelPanel* upgrade_level_panel;
        OptimizePanel* optimize_panel;

        void OnLoadRegulation(wxCommandEvent& event);
        void OnGenerateRegulation(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);

    public:
        MyFrame();
    };
}




// This defines the equivalent of main() for the current platform.
//wxIMPLEMENT_APP(ui::MyApp);
wxIMPLEMENT_APP_CONSOLE(ui::MyApp);