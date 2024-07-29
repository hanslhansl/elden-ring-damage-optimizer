#include "ui.hpp"


bool ui::MyApp::OnInit()
{
    MyFrame* frame = new MyFrame();
    frame->Show();
    return true;
}

ui::MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, "Hello World")
{
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_LOAD_REGULATION, "&open regulation file", "choose a regulation file from the filesystem");
    menuFile->Append(ID_GENERATE_REGULATION, "&generate regulation file", "not implemented");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT, "&quit", "quit this program");

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&file");
    menuBar->Append(menuHelp, "&help");

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetStatusText("created by hanslhansl");

    Bind(wxEVT_MENU, &MyFrame::OnLoadRegulation, this, ID_LOAD_REGULATION);
    Bind(wxEVT_MENU, &MyFrame::OnGenerateRegulation, this, ID_GENERATE_REGULATION);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);

    wxFlexGridSizer* stat_sizer = new wxFlexGridSizer(3, 8, 10, 10);

    stat_sizer->Add(new wxStaticText(this, wxID_ANY, wxT("starting class: ")));
    stat_sizer->Add(new wxChoice(this, wxID_ANY, wxDefaultPosition, { 200, 50 }, { "1", "2" , "3" , "4", "5", "6", "7", "2" , "3" , "4", "5", "6", "7" }));

    SetSizerAndFit(stat_sizer);
}

void ui::MyFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void ui::MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION);
}

void ui::MyFrame::OnLoadRegulation(wxCommandEvent& event)
{
    auto openFileDialog = wxFileDialog(this, "choose regulation file", std::filesystem::current_path().string(), "",
        "javascript files (*.js)|*.js|all files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    auto path = openFileDialog.GetPath();

    try
    {
        this->weapon_container = std::make_unique<calculator::weapon_container>(path.ToStdString());
    }
    catch (const std::exception& e)
    {
        wxLogError(e.what());
        return;
    }
}

void ui::MyFrame::OnGenerateRegulation(wxCommandEvent& event)
{
    wxMessageBox("not implemented", "OnGenerateRegulation", wxOK | wxICON_INFORMATION);
}