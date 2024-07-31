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
        wxStaticText* stat_variations_text;

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

        calculator::weapon::all_filter_options all_filter_options;
        calculator::weapon::filter filter;

        void onListBox(wxCommandEvent& event);

    public:
        FilterPanel(wxWindow* parent);

        void update_filter_options(calculator::weapon::all_filter_options&& all_filter_options_);

        const calculator::weapon::filter& get_filter() const;
    };

    class MyFrame : public wxFrame
    {
        std::unique_ptr<calculator::weapon_container> weapon_container;
        StatsPanel* stats_panel;
        FilterPanel* filter_panel;

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