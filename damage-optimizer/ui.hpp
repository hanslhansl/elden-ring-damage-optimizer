#pragma once

#include "include.hpp"
#include "calculator.hpp"

namespace ui
{
    constexpr auto to_lower(unsigned char c)
    {
        if (c == '_')
            return int(' ');
        return std::tolower(c);
    }

    enum
    {
        ID_LOAD_REGULATION = 1,
        ID_GENERATE_REGULATION,

        ID_STAT_CLASS_CHOICE,
        ID_STAT_SPINCTRLS,

        ID_FILTER_DLC_LIST,
        ID_FILTER_TYPES_LIST,
        ID_FILTER_AFFINITIES_LIST,
        ID_FILTER_BASE_NAMES_LIST,

        ID_UPGRADE_LEVEL_SPINCTRLS,

        ID_TWO_HANDING_CHECKBOX,

        ID_OPTIMIZE_FOR_TOTAL_ATTACK_POWER_RADIO,
        ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_RADIO,
        ID_OPTIMIZE_FOR_STATUS_EFFECT_RADIO,
        ID_OPTIMIZE_FOR_SPELL_SCALING_RADIO,
        ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_LIST,
        ID_OPTIMIZE_FOR_STATUS_EFFECT_LIST,
        ID_OPTIMIZE_FOR_SPELL_SCALING_LIST,

        ID_OPTIMIZE_BRUTE_FORCE_BUTTON,
        ID_OPTIMIZE_V2_BUTTON,
        ID_OPTIMIZE_THREADS_SPINCTRL,

        ID_RESULT_STAT_SPINCTRLS,
        ID_RESULT_TWO_HANDING_CHECKBOX,
        ID_RESULT_UPGRADE_LEVEL_SPINCTRLS,
        ID_RESULT_BASE_NAMES_LIST,
        ID_RESULT_AFFINITIES_LIST,
    };

    class MyApp : public wxApp
    {
    public:
        bool OnInit() override;
    };

    struct FullStatsPanel : wxPanel
    {
        static constexpr std::array attribute_names = { "vig", "min", "end", "str", "dex", "int", "fai", "arc" };

        std::array<wxSpinCtrl*, attribute_names.size()> attribute_ctrls;

        FullStatsPanel(wxWindow* parent, wxWindowID attribute_ctrls_id);

        calculator::FullStats get_full_stats() const;
    };

    struct StatsPanel : wxPanel
    {
        inline static const std::array<wxString, 10> starting_class_names = {"hero", "bandit", "astrologer", "warrior", "prisoner", "confessor", "wretch", "vagabond", "prophet", "samurai"};
        static constexpr std::array<std::array<int, FullStatsPanel::attribute_names.size()>, starting_class_names.size()> starting_class_stats{{
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

        wxChoice* class_choice;
        FullStatsPanel* full_stats_panel;
        wxSpinCtrl* player_level_ctrl;
        wxStaticText* attribute_points_text;
        wxStaticText* stat_variations_text;

        StatsPanel(wxWindow* parent);
    };

    struct FilterPanel : wxPanel
    {
        wxListBox* dlc_weapons_list;
        wxListBox* types_list;
        wxListBox* affinities_list;
        wxListBox* base_names_list;
        wxStaticText* filtered_weapons_text;

        FilterPanel(wxWindow* parent);
    };

    struct UpgradeLevelPanel : wxPanel
    {
        wxSpinCtrl* normal_upgrade_level_ctrl;
        wxSpinCtrl* somber_upgrade_level_ctrl;

        UpgradeLevelPanel(wxWindow* parent, wxWindowID spin_ctrls_id);

        calculator::UpgradeLevels get_upgrade_levels() const;
    };

    struct TwoHandingPanel : wxPanel
    {
        wxCheckBox* two_handing_checkbox;

        TwoHandingPanel(wxWindow* parent, wxWindowID checkbox_id);
    };

    struct OptimizeForPanel : wxPanel
    {
        inline static const std::array<wxString, calculator::DamageType::_size()> damage_type_names =
            []() {  std::array<wxString, calculator::DamageType::_size()> ret{};
            std::ranges::transform(calculator::DamageType::_names(), ret.begin(), [](std::string s) { std::ranges::transform(s, s.begin(), to_lower); return s; }); return ret; }();

        inline static const std::array<wxString, calculator::StatusType::_size()> status_type_names =
            []() {  std::array<wxString, calculator::StatusType::_size()> ret{};
            std::ranges::transform(calculator::StatusType::_names(), ret.begin(), [](std::string s) { std::ranges::transform(s, s.begin(), to_lower); return s; }); return ret; }();

        wxRadioButton* total_attack_power_button;

        wxRadioButton* individual_attack_power_button;
        wxListBox* individual_attack_power_list;

        wxRadioButton* status_effect_button;
        wxListBox* status_effect_list;

        wxRadioButton* spell_scaling_button;

        OptimizeForPanel(wxWindow* parent);
    };

    struct OptimizePanel : wxPanel
    {
        wxSpinCtrl* threads_ctrl;
        wxStaticText* total_variations_text;
        wxButton* brute_force_button;
        wxStaticText* brute_force_eta_text;
        wxButton* v2_button;
        wxStaticText* v2_eta_text;

        OptimizePanel(wxWindow* parent);
    };

    struct ResultPanel : wxPanel
    {
        FullStatsPanel* full_stats_panel;
        TwoHandingPanel* two_handing_panel;
        UpgradeLevelPanel* upgrade_level_panel;
        wxListBox* base_names_list;
        wxListBox* affinities_list;

        wxStaticText* full_name_text;
        wxStaticText* type_text;
        wxStaticText* dlc_text;

        wxBoxSizer* right_second_level_sizer; // just for resizing
        std::array<wxGenericStaticText*, 3> total_damage_texts{};
        std::array<std::array<wxGenericStaticText*, 3>, calculator::DamageType::_size()> damage_type_texts{};
        std::array<std::array<wxStaticText*, 3>, calculator::StatusType::_size()> status_effect_texts{};
        wxStaticText* spell_scaling_text;

        ResultPanel(wxWindow* parent);
    };

    class MainFrame : public wxFrame
    {
        calculator::WeaponContainer weapon_container;
        calculator::Weapon::AllFilterOptions all_filter_options{};
        calculator::Weapon::Filter filter{};
        calculator::FilteredWeapons filtered_weapons{};

        StatsPanel* stats_panel;
        FilterPanel* filter_panel;
        UpgradeLevelPanel* upgrade_level_panel;
        TwoHandingPanel* two_handing_panel;
        OptimizeForPanel* optimize_for_panel;
        OptimizePanel* optimize_panel;
        ResultPanel* result_panel;

        std::chrono::duration<double> time_per_million_variations_brute_force{};
        std::chrono::duration<double> time_per_million_variations_v2{};

        void brute_force();
        void update_variation_labels();
        void update_filtered_weapons();
        void update_filter_options();
        void update_result(wxCommandEvent&);
        void set_result_attack_rating(const calculator::AttackRating::full& attack_rating, const calculator::FullStats& full_stats);

        void OnChoiceSelected(wxCommandEvent& event);
        void OnListBox(wxCommandEvent& event);
        void OnSpinCtrl(wxCommandEvent& event);
        void OnButton(wxCommandEvent& event);
        void OnRadioButton(wxCommandEvent& event);

        void OnLoadRegulation(wxCommandEvent& event);
        void OnGenerateRegulation(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);

    public:
        MainFrame();
    };
}




// This defines the equivalent of main() for the current platform.
//wxIMPLEMENT_APP(ui::MyApp);
wxIMPLEMENT_APP_CONSOLE(ui::MyApp);

//int main()
//{
//    calculator::test2();
//	return 0;
//}