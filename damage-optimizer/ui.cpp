#include "ui.hpp"


bool ui::MyApp::OnInit()
{
    MyFrame* frame = new MyFrame();
    frame->Show();
    return true;
}


void ui::StatsPanel::update_stat_variations()
{
    auto player_level = this->player_level_ctrl->GetValue();
    auto total_attribute_points = player_level + 79;
    auto available_attribute_points = total_attribute_points - this->attribute_ctrls.at(0)->GetValue() - this->attribute_ctrls.at(1)->GetValue() - this->attribute_ctrls.at(2)->GetValue();

    calculator::Stats stats = {
        this->attribute_ctrls.at(3)->GetValue(),
        this->attribute_ctrls.at(4)->GetValue(),
        this->attribute_ctrls.at(5)->GetValue(),
        this->attribute_ctrls.at(6)->GetValue(),
        this->attribute_ctrls.at(7)->GetValue(),
    };

    auto stat_variations_count = calculator::get_stat_variation_count(available_attribute_points, stats);
    this->attribute_points_text->SetLabel(std::to_string(total_attribute_points));
    this->stat_variations_text->SetLabel(std::to_string(stat_variations_count));
}

void ui::StatsPanel::OnChange(wxCommandEvent& event)
{
    this->update_stat_variations();
}

void ui::StatsPanel::OnChoiceSelected(wxCommandEvent& event)
{
    for (auto&& [spin_ctrl, attr_val] : std::views::zip(this->attribute_ctrls, starting_class_stats.at(this->choice->GetSelection())))
        spin_ctrl->SetValue(attr_val);

    this->update_stat_variations();
}

ui::StatsPanel::StatsPanel(wxWindow* parent) : wxPanel(parent)
{
    wxBoxSizer* root_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "minimum stats");

    wxBoxSizer* class_sizer = new  wxBoxSizer(wxHORIZONTAL);
    class_sizer->Add(new wxStaticText(this, wxID_ANY, wxT("starting class: ")));
    this->choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, this->starting_class_names.size(), this->starting_class_names.data());
    class_sizer->Add(this->choice);
    static_sizer->Add(class_sizer);

    wxBoxSizer* inner_stats_sizer = new  wxBoxSizer(wxHORIZONTAL);
    for (auto&& [attribute_name, spin_ctrl] : std::views::zip(this->attribute_names, this->attribute_ctrls))
    {
        wxBoxSizer* stat_sizer = new  wxBoxSizer(wxVERTICAL);
        stat_sizer->Add(new wxStaticText(this, wxID_ANY, attribute_name));
        spin_ctrl = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 99);
        stat_sizer->Add(spin_ctrl);
        inner_stats_sizer->Add(stat_sizer);
    }
    static_sizer->Add(inner_stats_sizer);

    wxStaticBoxSizer* player_level_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "character level");
    this->player_level_ctrl = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, { 80, 20 }, wxSP_ARROW_KEYS, 1, 713);
    player_level_static_sizer->Add(this->player_level_ctrl);

    wxStaticBoxSizer* attribute_points_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "attribute points");
    this->attribute_points_text = new wxStaticText(this, wxID_ANY, "0");
    attribute_points_static_sizer->Add(this->attribute_points_text);

    wxStaticBoxSizer* stat_variations_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "stat variations");
    this->stat_variations_text = new wxStaticText(this, wxID_ANY, "0");
    stat_variations_static_sizer->Add(this->stat_variations_text);

    root_sizer->Add(static_sizer);
    root_sizer->Add(player_level_static_sizer);
    root_sizer->Add(attribute_points_static_sizer);
    root_sizer->Add(stat_variations_static_sizer);

    SetSizerAndFit(root_sizer);

    Bind(wxEVT_CHOICE, &StatsPanel::OnChoiceSelected, this);
    Bind(wxEVT_SPINCTRL, &StatsPanel::OnChange, this);

    this->update_stat_variations();
}


void ui::FilterPanel::update_filtered_weapons()
{
    this->filtered_weapons.weapons.clear();
    this->filtered_weapons.weapons.reserve(this->weapon_container->weapons.size());
    this->weapon_container->apply_filter(this->filter, this->filtered_weapons);
    this->filtered_weapons_text->SetLabel(std::to_string(this->filtered_weapons.weapons.size()) + "/" + std::to_string(this->weapon_container->weapons.size()));
}

void ui::FilterPanel::onListBox(wxCommandEvent& event)
{
    auto selected = event.IsSelection();
    auto id = event.GetId();
    auto index = event.GetInt();

    if (id == ID_FILTER_DLC)
    {
        auto&& val = this->all_filter_options.dlc.at(index);
        if (selected)
            this->filter.dlc.emplace(val);
		else
            this->filter.dlc.erase(val);
    }
    else if (id == ID_FILTER_TYPES)
	{
        auto&& val = this->all_filter_options.types.at(index);
		if (selected)
			this->filter.types.emplace(val);
		else
			this->filter.types.erase(val);
	}
	else if (id == ID_FILTER_AFFINITIES)
	{
		auto&& val = this->all_filter_options.affinities.at(index);
		if (selected)
			this->filter.affinities.emplace(val);
		else
			this->filter.affinities.erase(val);
	}
	else if (id == ID_FILTER_BASE_NAMES)
	{
		auto&& val = this->all_filter_options.base_names.at(index);
		if (selected)
			this->filter.base_names.emplace(val);
		else
			this->filter.base_names.erase(val);
	}
	else
	{
		throw std::runtime_error("unknown filter id");
	}

    this->update_filtered_weapons();
}

ui::FilterPanel::FilterPanel(wxWindow* parent) : wxPanel(parent)
{
    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "weapon filters");

    auto types_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "type");
    this->types_list = new wxListBox(this, ID_FILTER_TYPES, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    types_sizer->Add(this->types_list);
    static_sizer->Add(types_sizer);

    auto affinities_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "affinity");
    this->affinities_list = new wxListBox(this, ID_FILTER_AFFINITIES, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    affinities_sizer->Add(this->affinities_list);
    static_sizer->Add(affinities_sizer);

    auto base_names_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "name");
    this->base_names_list = new wxListBox(this, ID_FILTER_BASE_NAMES, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    base_names_sizer->Add(this->base_names_list);
    static_sizer->Add(base_names_sizer);

    wxBoxSizer* dlc_and_count_sizer = new wxBoxSizer(wxVERTICAL);

    auto dlc_weapons_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "base game/dlc");
    this->dlc_weapons_list = new wxListBox(this, ID_FILTER_DLC, wxDefaultPosition, { 130, 40 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    dlc_weapons_sizer->Add(this->dlc_weapons_list);
    dlc_and_count_sizer->Add(dlc_weapons_sizer);

    wxStaticBoxSizer* weapons_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "weapons");
    this->filtered_weapons_text = new wxStaticText(this, wxID_ANY, "/", wxDefaultPosition, { 130, 20 });
    weapons_static_sizer->Add(this->filtered_weapons_text);
    dlc_and_count_sizer->Add(weapons_static_sizer);

    static_sizer->Add(dlc_and_count_sizer);

    Bind(wxEVT_LISTBOX, &FilterPanel::onListBox, this, ID_FILTER_DLC);
    Bind(wxEVT_LISTBOX, &FilterPanel::onListBox, this, ID_FILTER_TYPES);
    Bind(wxEVT_LISTBOX, &FilterPanel::onListBox, this, ID_FILTER_AFFINITIES);
    Bind(wxEVT_LISTBOX, &FilterPanel::onListBox, this, ID_FILTER_BASE_NAMES);

    SetSizerAndFit(static_sizer);
}

void ui::FilterPanel::update_filter_options(calculator::weapon_container& weapon_container_)
{
    this->weapon_container = &weapon_container_;
    this->all_filter_options = this->weapon_container->get_all_filter_options();
    this->filter.affinities.clear();
    this->filter.base_names.clear();
    this->filter.dlc.clear();
    this->filter.types.clear();
    this->filtered_weapons.weapons.clear();

	this->dlc_weapons_list->Clear();
	for (auto&& dlc : this->all_filter_options.dlc)
        this->dlc_weapons_list->Append(dlc ? "dlc weapons" : "base game weapons");

    this->types_list->Clear();
    for (auto&& type : this->all_filter_options.types)
    {
        std::string type_str = type._to_string();
        std::ranges::transform(type_str, type_str.begin(), [](unsigned char c) { if (c == '_') return int(' '); return std::tolower(c); });
        this->types_list->Append(type_str);
    }

    this->affinities_list->Clear();
    for (auto&& affinity : this->all_filter_options.affinities)
	{
		std::string affinity_str = affinity._to_string();
		std::ranges::transform(affinity_str, affinity_str.begin(), [](unsigned char c) { if (c == '_') return int(' '); return std::tolower(c); });
		this->affinities_list->Append(affinity_str);
	}

    this->base_names_list->Clear();
	for (auto&& base_name : this->all_filter_options.base_names)
		this->base_names_list->Append(base_name);

	this->update_filtered_weapons();
}

const calculator::weapon::filter& ui::FilterPanel::get_filter() const
{
	return this->filter;
}


ui::Upgrade_LevelPanel::Upgrade_LevelPanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "upgrade level");

    wxBoxSizer* normal_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    normal_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "normal: ", wxDefaultPosition, { 50, 20 }), 0, wxALL, 2);
    this->normal_upgrade_level_ctrl = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 25);
    normal_upgrade_level_sizer->Add(this->normal_upgrade_level_ctrl, 0, wxALL, 2);
    static_sizer->Add(normal_upgrade_level_sizer);

    wxBoxSizer* somber_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    somber_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "somber: ", wxDefaultPosition, { 50, 20 }), 0, wxALL, 2);
    this->somber_upgrade_level_ctrl = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 10);
    somber_upgrade_level_sizer->Add(this->somber_upgrade_level_ctrl, 0, wxALL, 2);
    static_sizer->Add(somber_upgrade_level_sizer);

	SetSizerAndFit(static_sizer);
}


ui::OptimizePanel::OptimizePanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "optimize");

    wxBoxSizer* total_attack_power_sizer = new wxBoxSizer(wxVERTICAL);
    this->total_attack_power_button = new wxRadioButton(this, wxID_ANY, "total attack power");
    total_attack_power_sizer->Add(this->total_attack_power_button, 0, wxALL, 4);
    static_sizer->Add(total_attack_power_sizer);

    wxBoxSizer* individual_attack_power_sizer = new wxBoxSizer(wxVERTICAL);
    this->individual_attack_power_button = new wxRadioButton(this, wxID_ANY, "individual attack power");
    individual_attack_power_sizer->Add(this->individual_attack_power_button, 0, wxALL, 4);
    this->individual_attack_power_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, { 150, 110 }, damage_type_names.size(), damage_type_names.data(), wxLB_SINGLE | wxLB_NEEDED_SB);
    individual_attack_power_sizer->Add(this->individual_attack_power_list);
    static_sizer->Add(individual_attack_power_sizer);

    wxBoxSizer* status_effect_sizer = new wxBoxSizer(wxVERTICAL);
    this->status_effect_button = new wxRadioButton(this, wxID_ANY, "status effect");
    status_effect_sizer->Add(this->status_effect_button, 0, wxALL, 4);
    this->status_effect_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, { 150, 110 }, status_type_names.size(), status_type_names.data(), wxLB_SINGLE | wxLB_NEEDED_SB);
    status_effect_sizer->Add(this->status_effect_list);
    static_sizer->Add(status_effect_sizer);

    wxBoxSizer* spell_scaling_sizer = new wxBoxSizer(wxVERTICAL);
    this->spell_scaling_button = new wxRadioButton(this, wxID_ANY, "spell scaling");
    spell_scaling_sizer->Add(this->spell_scaling_button, 0, wxALL, 4);
    static_sizer->Add(spell_scaling_sizer);


	SetSizerAndFit(static_sizer);
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

    this->filter_panel->update_filter_options(*this->weapon_container);
}

void ui::MyFrame::OnGenerateRegulation(wxCommandEvent& event)
{
    wxMessageBox("not implemented", "OnGenerateRegulation", wxOK | wxICON_INFORMATION);
}

ui::MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, "elden ring damage optimizer")
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


    auto root_panel = new wxPanel(this);

    this->stats_panel = new StatsPanel(root_panel);
    this->filter_panel = new FilterPanel(root_panel);
    this->upgrade_level_panel = new Upgrade_LevelPanel(root_panel);
    this->optimize_panel = new OptimizePanel(root_panel);

    auto root_sizer = new wxBoxSizer(wxVERTICAL);
    root_sizer->Add(this->stats_panel, 0, wxEXPAND);
    root_sizer->Add(this->filter_panel, 0, wxEXPAND);
    wxBoxSizer* upgrade_level_and_optimize_sizer = new wxBoxSizer(wxHORIZONTAL);
    upgrade_level_and_optimize_sizer->Add(this->upgrade_level_panel, 0, wxEXPAND);
    upgrade_level_and_optimize_sizer->Add(this->optimize_panel, 0, wxEXPAND);
    root_sizer->Add(upgrade_level_and_optimize_sizer, 0, wxEXPAND);

    root_panel->SetSizerAndFit(root_sizer);

    auto root_root_sizer = new wxBoxSizer(wxVERTICAL);
    root_root_sizer->Add(root_panel, 1, wxEXPAND);
    this->SetSizerAndFit(root_root_sizer);

    root_panel->SetFocus();
}


