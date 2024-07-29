#pragma once

#include "include.hpp"

namespace ui
{
    enum
    {
        ID_GENERATE_REGULATION = 1,
        ID_LOAD_REGULATION,
    };

    class MyApp : public wxApp
    {
    public:
        bool OnInit() override;
    };

    class MyFrame : public wxFrame
    {
        void OnLoadRegulation(wxCommandEvent& event);
        void OnGenerateRegulation(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);

    public:
        MyFrame();
    };
}




// This defines the equivalent of main() for the current platform.
wxIMPLEMENT_APP(ui::MyApp);