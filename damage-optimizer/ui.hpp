#pragma once

#include "include.hpp"
#include "calculator.hpp"

namespace ui
{
    enum
    {
        ID_LOAD_REGULATION = 1,
        ID_GENERATE_REGULATION,
    };

    class MyApp : public wxApp
    {
    public:
        bool OnInit() override;
    };

    class MyFrame : public wxFrame
    {
        std::unique_ptr<calculator::weapon_container> weapon_container;

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