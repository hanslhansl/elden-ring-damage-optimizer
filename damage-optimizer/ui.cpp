#include "ui.hpp"


bool ui::MyApp::OnInit()
{
    MainFrame* frame = new MainFrame();
    frame->Show();
    return true;
}


ui::StatsPanel::StatsPanel(wxWindow* parent) : wxPanel(parent)
{
    wxBoxSizer* root_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "minimum stats");

    wxBoxSizer* class_sizer = new  wxBoxSizer(wxHORIZONTAL);
    class_sizer->Add(new wxStaticText(this, wxID_ANY, wxT("starting class: ")));
    this->class_choice = new wxChoice(this, ID_STAT_CLASS_CHOICE, wxDefaultPosition, wxDefaultSize, this->starting_class_names.size(), this->starting_class_names.data());
    class_sizer->Add(this->class_choice);
    static_sizer->Add(class_sizer);

    wxBoxSizer* inner_stats_sizer = new  wxBoxSizer(wxHORIZONTAL);
    for (auto&& [attribute_name, spin_ctrl] : std::views::zip(this->attribute_names, this->attribute_ctrls))
    {
        wxBoxSizer* stat_sizer = new  wxBoxSizer(wxVERTICAL);
        stat_sizer->Add(new wxStaticText(this, wxID_ANY, attribute_name));
        spin_ctrl = new wxSpinCtrl(this, ID_STAT_SPINCTRLS, "1", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 1, 99);
        stat_sizer->Add(spin_ctrl);
        inner_stats_sizer->Add(stat_sizer);
    }
    static_sizer->Add(inner_stats_sizer);

    wxStaticBoxSizer* player_level_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "character level");
    this->player_level_ctrl = new wxSpinCtrl(this, ID_STAT_SPINCTRLS, "1", wxDefaultPosition, { 80, 20 }, wxSP_ARROW_KEYS, 1, 713);
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
}

calculator::FullStats ui::StatsPanel::get_full_stats() const
{
    return {
		this->attribute_ctrls.at(0)->GetValue(),
		this->attribute_ctrls.at(1)->GetValue(),
		this->attribute_ctrls.at(2)->GetValue(),
		this->attribute_ctrls.at(3)->GetValue(),
		this->attribute_ctrls.at(4)->GetValue(),
		this->attribute_ctrls.at(5)->GetValue(),
		this->attribute_ctrls.at(6)->GetValue(),
		this->attribute_ctrls.at(7)->GetValue(),
    };
}


ui::FilterPanel::FilterPanel(wxWindow* parent) : wxPanel(parent)
{
    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "weapon filters");

    auto types_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "type");
    this->types_list = new wxListBox(this, ID_FILTER_TYPES_LIST, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    types_sizer->Add(this->types_list);
    static_sizer->Add(types_sizer);

    auto affinities_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "affinity");
    this->affinities_list = new wxListBox(this, ID_FILTER_AFFINITIES_LIST, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    affinities_sizer->Add(this->affinities_list);
    static_sizer->Add(affinities_sizer);

    auto base_names_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "name");
    this->base_names_list = new wxListBox(this, ID_FILTER_BASE_NAMES_LIST, wxDefaultPosition, { 200, 200 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    base_names_sizer->Add(this->base_names_list);
    static_sizer->Add(base_names_sizer);

    wxBoxSizer* dlc_and_count_sizer = new wxBoxSizer(wxVERTICAL);

    auto dlc_weapons_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "base game/dlc");
    this->dlc_weapons_list = new wxListBox(this, ID_FILTER_DLC_LIST, wxDefaultPosition, { 130, 40 }, {}, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    dlc_weapons_sizer->Add(this->dlc_weapons_list);
    dlc_and_count_sizer->Add(dlc_weapons_sizer);

    wxStaticBoxSizer* weapons_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "weapons");
    this->filtered_weapons_text = new wxStaticText(this, wxID_ANY, "0/0", wxDefaultPosition, { 130, 20 });
    weapons_static_sizer->Add(this->filtered_weapons_text);
    dlc_and_count_sizer->Add(weapons_static_sizer);

    static_sizer->Add(dlc_and_count_sizer);

    SetSizerAndFit(static_sizer);
}


ui::UpgradeLevelPanel::UpgradeLevelPanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "upgrade level");

    wxBoxSizer* normal_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    normal_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "normal: ", wxDefaultPosition, { 50, 20 }), 0, wxALL, 2);
    this->normal_upgrade_level_ctrl = new wxSpinCtrl(this, ID_UPGRADE_LEVEL_SPINCTRLS, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 25);
    normal_upgrade_level_sizer->Add(this->normal_upgrade_level_ctrl, 0, wxALL, 2);
    static_sizer->Add(normal_upgrade_level_sizer);

    wxBoxSizer* somber_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    somber_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "somber: ", wxDefaultPosition, { 50, 20 }), 0, wxALL, 2);
    this->somber_upgrade_level_ctrl = new wxSpinCtrl(this, ID_UPGRADE_LEVEL_SPINCTRLS, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 10);
    somber_upgrade_level_sizer->Add(this->somber_upgrade_level_ctrl, 0, wxALL, 2);
    static_sizer->Add(somber_upgrade_level_sizer);

	SetSizerAndFit(static_sizer);
}


ui::TwoHandingPanel::TwoHandingPanel(wxWindow* parent) : wxPanel(parent)
{
    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this);

	this->two_handing_checkbox = new wxCheckBox(this, wxID_ANY, "two handing");
	static_sizer->Add(this->two_handing_checkbox);

	SetSizerAndFit(static_sizer);
}


ui::OptimizeForPanel::OptimizeForPanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "optimize for");

    wxBoxSizer* total_attack_power_sizer = new wxBoxSizer(wxVERTICAL);
    this->total_attack_power_button = new wxRadioButton(this, ID_OPTIMIZE_FOR_TOTAL_ATTACK_POWER_RADIO, "total attack power");
    this->total_attack_power_button->SetValue(true);
    total_attack_power_sizer->Add(this->total_attack_power_button, 0, wxALL, 4);
    static_sizer->Add(total_attack_power_sizer);

    wxBoxSizer* individual_attack_power_sizer = new wxBoxSizer(wxVERTICAL);
    this->individual_attack_power_button = new wxRadioButton(this, ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_RADIO, "individual attack power");
    individual_attack_power_sizer->Add(this->individual_attack_power_button, 0, wxALL, 4);
    this->individual_attack_power_list = new wxListBox(this, ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_LIST, wxDefaultPosition, { 150, 110 }, damage_type_names.size(), damage_type_names.data(), wxLB_SINGLE | wxLB_NEEDED_SB);
    this->individual_attack_power_list->SetSelection(0);
    this->individual_attack_power_list->Disable();
    individual_attack_power_sizer->Add(this->individual_attack_power_list);
    static_sizer->Add(individual_attack_power_sizer);

    wxBoxSizer* status_effect_sizer = new wxBoxSizer(wxVERTICAL);
    this->status_effect_button = new wxRadioButton(this, ID_OPTIMIZE_FOR_STATUS_EFFECT_RADIO, "status effect");
    status_effect_sizer->Add(this->status_effect_button, 0, wxALL, 4);
    this->status_effect_list = new wxListBox(this, ID_OPTIMIZE_FOR_STATUS_EFFECT_LIST, wxDefaultPosition, { 150, 110 }, status_type_names.size(), status_type_names.data(), wxLB_SINGLE | wxLB_NEEDED_SB);
    this->status_effect_list->SetSelection(0);
    this->status_effect_list->Disable();
    status_effect_sizer->Add(this->status_effect_list);
    static_sizer->Add(status_effect_sizer);

    wxBoxSizer* spell_scaling_sizer = new wxBoxSizer(wxVERTICAL);
    this->spell_scaling_button = new wxRadioButton(this, ID_OPTIMIZE_FOR_SPELL_SCALING_RADIO, "spell scaling");
    spell_scaling_sizer->Add(this->spell_scaling_button, 0, wxALL, 4);
    static_sizer->Add(spell_scaling_sizer);


	SetSizerAndFit(static_sizer);
}


ui::OptimizePanel::OptimizePanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "optimize");

    this->optimize_button = new wxButton(this, ID_OPTIMIZE_BRUTE_FORCE_BUTTON, "brute force");
    static_sizer->Add(this->optimize_button, 0, wxALL, 4);

    wxBoxSizer* threads_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "threads");
    this->threads_ctrl = new wxSpinCtrl(this, ID_OPTIMIZE_THREADS_SPINCTRL, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100);
    threads_sizer->Add(this->threads_ctrl, 1, wxEXPAND);
    static_sizer->Add(threads_sizer, 0, wxEXPAND);

    wxStaticBoxSizer* total_variations_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "variations");
	this->total_variations_text = new wxStaticText(this, wxID_ANY, "0");
    total_variations_sizer->Add(this->total_variations_text, 0, wxEXPAND);
    static_sizer->Add(total_variations_sizer, 0, wxEXPAND);

    wxStaticBoxSizer* eta_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "eta");
    this->eta_text = new wxStaticText(this, wxID_ANY, "nan");
    eta_sizer->Add(this->eta_text, 0, wxEXPAND);
    static_sizer->Add(eta_sizer, 0, wxEXPAND);

	SetSizerAndFit(static_sizer);
}


ui::ResultPanel::ResultPanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "result");

	SetSizerAndFit(static_sizer);
}


void ui::MainFrame::optimize()
{
    // stat variations
    auto full_stats = this->stats_panel->get_full_stats();
    auto min_stats = calculator::full_stats_to_stats(full_stats);
    auto player_level = this->stats_panel->player_level_ctrl->GetValue();
    auto total_attribute_points = player_level + 79;
    auto available_attribute_points = total_attribute_points
        - full_stats.at(0)
        - full_stats.at(1)
        - full_stats.at(2);
    auto stat_variations = calculator::get_stat_variations(available_attribute_points, min_stats);

    // upgrade levels
    auto normal_upgrade_level = this->upgrade_level_panel->normal_upgrade_level_ctrl->GetValue();
    auto somber_upgrade_level = this->upgrade_level_panel->somber_upgrade_level_ctrl->GetValue();

    // two handing
    bool two_handing = this->two_handing_panel->two_handing_checkbox->GetValue();

    // threads
    int threads = this->optimize_panel->threads_ctrl->GetValue();

    // set up attack options
    calculator::AttackOptions atk_options = { available_attribute_points, { normal_upgrade_level, somber_upgrade_level }, two_handing };

    // run optimization
    std::unique_ptr<calculator::OptimizationContext> opt_context;

    if (this->optimize_for_panel->total_attack_power_button->GetValue())
        opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::total>{});
    else if (this->optimize_for_panel->individual_attack_power_button->GetValue())
    {
        auto index = this->optimize_for_panel->individual_attack_power_list->GetSelection();

        if (index == calculator::DamageType::PHYSICAL)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::physical>{});
		else if (index == calculator::DamageType::MAGIC)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::magic>{});
        else if (index == calculator::DamageType::FIRE)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::fire>{});
        else if (index == calculator::DamageType::LIGHTNING)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::lightning>{});
        else if (index == calculator::DamageType::HOLY)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::holy>{});
		else
			throw std::runtime_error("unknown damage type");
    }
    else if (this->optimize_for_panel->status_effect_button->GetValue())
    {
        auto index = this->optimize_for_panel->status_effect_list->GetSelection() + calculator::StatusType::POISON;

        if (index == calculator::StatusType::POISON)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::poison_status>{});
        else if (index == calculator::StatusType::SCARLET_ROT)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::scarlet_rot_status>{});
		else if (index == calculator::StatusType::BLEED)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::bleed_status>{});
		else if (index == calculator::StatusType::FROST)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::frost_status>{});
        else if (index == calculator::StatusType::SLEEP)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::sleep_status>{});
        else if (index == calculator::StatusType::MADNESS)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::madness_status>{});
        else if (index == calculator::StatusType::DEATH_BLIGHT)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::attack_rating::death_blight_status>{});
        else
            throw std::runtime_error("unknown status type");
    }
    else if (this->optimize_for_panel->spell_scaling_button->GetValue())
        throw std::runtime_error("not implemented");
    else
        throw std::runtime_error("unknown optimization type");
    
    // wait for result
    auto attack_rating = opt_context->wait_and_get_result();

    // print stats
    misc::printl();
    misc::print("stats: ");
    for (auto stat : attack_rating.stats)
        misc::print(stat, " ");
    misc::printl("\n");

    misc::printl(attack_rating.weapon->full_name, ": ", attack_rating.total_attack_power);
    misc::printl();

    misc::printl("attack rating:");
    for (int i = 0; i < attack_rating.attack_power.size(); i++)
        if (attack_rating.attack_power.at(i).first != 0)
            misc::printl(calculator::DamageType::_from_integral(i)._to_string(), ": ", attack_rating.attack_power.at(i).second.at(0), " + ", attack_rating.attack_power.at(i).second.at(1),
                " = ", attack_rating.attack_power.at(i).first);
    misc::printl();

    misc::printl("status effects:");
    for (int i = 0; i < attack_rating.status_effect.size(); i++)
        if (attack_rating.status_effect.at(i).first != 0)
            misc::printl(calculator::StatusType::_from_index(i)._to_string(), ": ", attack_rating.status_effect.at(i).second.at(0), " + ", attack_rating.status_effect.at(i).second.at(1),
                " = ", attack_rating.status_effect.at(i).first);
    misc::printl();

    misc::printl("spell scaling:");
    for (int i = 0; i < attack_rating.spell_scaling.size(); i++)
        if (attack_rating.spell_scaling.at(i) != 0)
            misc::printl(calculator::DamageType::_from_integral(i)._to_string(), ": ", attack_rating.spell_scaling.at(i), "%");
    misc::printl();
}

void ui::MainFrame::update_variation_labels()
{
    // stat variations
    auto full_stats = this->stats_panel->get_full_stats();
    auto min_stats = calculator::full_stats_to_stats(full_stats);
    auto player_level = this->stats_panel->player_level_ctrl->GetValue();
    auto total_attribute_points = player_level + 79;
    auto available_attribute_points = total_attribute_points
        - full_stats.at(0)
        - full_stats.at(1)
        - full_stats.at(2);

    auto stat_variation_count = calculator::get_stat_variation_count(available_attribute_points, min_stats);
    this->stats_panel->attribute_points_text->SetLabel(std::to_string(total_attribute_points));
    this->stats_panel->stat_variations_text->SetLabel(std::to_string(stat_variation_count));

    // weapon count
    auto weapon_count = this->weapon_container.weapons.size();
    auto filtered_weapon_count = this->filtered_weapons.size();
    this->filter_panel->filtered_weapons_text->SetLabel(std::to_string(filtered_weapon_count) + "/" + std::to_string(weapon_count));

    // total variations
    auto total_variations = stat_variation_count * filtered_weapon_count;
    this->optimize_panel->total_variations_text->SetLabel(std::to_string(total_variations));
}

void ui::MainFrame::update_filtered_weapons()
{
    this->filtered_weapons = this->weapon_container.apply_filter(this->filter);

    this->update_variation_labels();
}

void ui::MainFrame::update_filter_options()
{
    this->all_filter_options = this->weapon_container.get_all_filter_options();
    this->filter.affinities.clear();
    this->filter.base_names.clear();
    this->filter.dlc.clear();
    this->filter.types.clear();
    this->filtered_weapons.clear();

    this->filter_panel->dlc_weapons_list->Clear();
    for (auto&& dlc : this->all_filter_options.dlc)
        this->filter_panel->dlc_weapons_list->Append(dlc ? "dlc weapons" : "base game weapons");

    this->filter_panel->types_list->Clear();
    for (auto&& type : this->all_filter_options.types)
    {
        std::string type_str = type._to_string();
        std::ranges::transform(type_str, type_str.begin(), to_lower);
        this->filter_panel->types_list->Append(type_str);
    }

    this->filter_panel->affinities_list->Clear();
    for (auto&& affinity : this->all_filter_options.affinities)
    {
        std::string affinity_str = affinity._to_string();
        std::ranges::transform(affinity_str, affinity_str.begin(), to_lower);
        this->filter_panel->affinities_list->Append(affinity_str);
    }

    this->filter_panel->base_names_list->Clear();
    for (auto&& base_name : this->all_filter_options.base_names)
        this->filter_panel->base_names_list->Append(base_name);

    this->update_filtered_weapons();
}

void ui::MainFrame::OnChoiceSelected(wxCommandEvent& event)
{
    auto id = event.GetId();

    if (id == ID_STAT_CLASS_CHOICE)
    {
        auto selection = this->stats_panel->class_choice->GetSelection();
        for (auto&& [spin_ctrl, attr_val] : std::views::zip(this->stats_panel->attribute_ctrls, this->stats_panel->starting_class_stats.at(selection)))
            spin_ctrl->SetValue(attr_val);

        this->update_variation_labels();
    }
    else
        throw std::runtime_error("unknown class id");
}

void ui::MainFrame::OnListBox(wxCommandEvent& event)
{
    auto selected = event.IsSelection();
    auto id = event.GetId();
    auto index = event.GetInt();

    switch (id)
    {
    case ID_FILTER_DLC_LIST:
    {
        auto&& val = this->all_filter_options.dlc.at(index);
        if (selected)
            this->filter.dlc.emplace(val);
        else
            this->filter.dlc.erase(val);
        this->update_filtered_weapons();
        return;
    }
    case ID_FILTER_TYPES_LIST:
    {
        auto&& val = this->all_filter_options.types.at(index);
        if (selected)
            this->filter.types.emplace(val);
        else
            this->filter.types.erase(val);
        this->update_filtered_weapons();
        return;
    }
    case ID_FILTER_AFFINITIES_LIST:
    {
        auto&& val = this->all_filter_options.affinities.at(index);
        if (selected)
            this->filter.affinities.emplace(val);
        else
            this->filter.affinities.erase(val);
        this->update_filtered_weapons();
        return;
    }
    case ID_FILTER_BASE_NAMES_LIST:
    {
        auto&& val = this->all_filter_options.base_names.at(index);
        if (selected)
            this->filter.base_names.emplace(val);
        else
            this->filter.base_names.erase(val);
        this->update_filtered_weapons();
        return;
    }

    case ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_LIST:
    case ID_OPTIMIZE_FOR_STATUS_EFFECT_LIST:
    case ID_OPTIMIZE_FOR_SPELL_SCALING_LIST:
        return;

    default:
        throw std::runtime_error("unknown filter id");
    }
}

void ui::MainFrame::OnSpinCtrl(wxCommandEvent& event)
{
    auto id = event.GetId();

    if (id == ID_STAT_SPINCTRLS)
        this->update_variation_labels();
    else if (ID_UPGRADE_LEVEL_SPINCTRLS)
        ;
    else
        throw std::runtime_error("unknown spin ctrl id");
}

void ui::MainFrame::OnButton(wxCommandEvent& event)
{
    auto id = event.GetId();

	if (id == ID_OPTIMIZE_BRUTE_FORCE_BUTTON)
		this->optimize();
	else
		throw std::runtime_error("unknown button id");
}

void ui::MainFrame::OnRadioButton(wxCommandEvent& event)
{
    auto id = event.GetId();

    switch (id)
    {
    case ID_OPTIMIZE_FOR_TOTAL_ATTACK_POWER_RADIO:
        this->optimize_for_panel->individual_attack_power_list->Disable();
        this->optimize_for_panel->status_effect_list->Disable();
        break;
    case ID_OPTIMIZE_FOR_INDIVIDUAL_ATTACK_POWER_RADIO:
        this->optimize_for_panel->individual_attack_power_list->Enable();
        this->optimize_for_panel->status_effect_list->Disable();
        break;
    case ID_OPTIMIZE_FOR_STATUS_EFFECT_RADIO:
        this->optimize_for_panel->individual_attack_power_list->Disable();
        this->optimize_for_panel->status_effect_list->Enable();
        break;
    case ID_OPTIMIZE_FOR_SPELL_SCALING_RADIO:
        this->optimize_for_panel->individual_attack_power_list->Disable();
        this->optimize_for_panel->status_effect_list->Disable();
        break;
    default:
		throw std::runtime_error("unknown radio button id");
    }
}

void ui::MainFrame::OnLoadRegulation(wxCommandEvent& event)
{
    auto openFileDialog = wxFileDialog(this, "choose regulation file", std::filesystem::current_path().string(), "",
        "javascript files (*.js)|*.js|all files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    auto path = openFileDialog.GetPath();

    try
    {
        this->weapon_container = calculator::WeaponContainer(path.ToStdString());
    }
    catch (const std::exception& e)
    {
        wxLogError(e.what());
        return;
    }

    this->update_filter_options();
}

void ui::MainFrame::OnGenerateRegulation(wxCommandEvent& event)
{
    wxMessageBox("not implemented", "OnGenerateRegulation", wxOK | wxICON_INFORMATION);
}

void ui::MainFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void ui::MainFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION);
}

ui::MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "elden ring damage optimizer")
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

    Bind(wxEVT_MENU, &MainFrame::OnLoadRegulation, this, ID_LOAD_REGULATION);
    Bind(wxEVT_MENU, &MainFrame::OnGenerateRegulation, this, ID_GENERATE_REGULATION);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);


    auto root_panel = new wxPanel(this);

    this->optimize_panel = new OptimizePanel(root_panel);
    this->stats_panel = new StatsPanel(root_panel);
    this->filter_panel = new FilterPanel(root_panel);
    this->two_handing_panel = new TwoHandingPanel(root_panel);
    this->upgrade_level_panel = new UpgradeLevelPanel(root_panel);
    this->optimize_for_panel = new OptimizeForPanel(root_panel);
    this->result_panel = new ResultPanel(root_panel);

    auto root_sizer = new wxBoxSizer(wxVERTICAL);
    root_sizer->Add(this->stats_panel, 0, wxEXPAND);
    root_sizer->Add(this->filter_panel, 0, wxEXPAND);

    wxBoxSizer* upgrade_level_and_two_handing_sizer = new wxBoxSizer(wxVERTICAL);
    upgrade_level_and_two_handing_sizer->Add(this->upgrade_level_panel, 0, wxEXPAND);
    upgrade_level_and_two_handing_sizer->Add(this->two_handing_panel, 0, wxEXPAND);

    wxBoxSizer* upgrade_level_and_tow_handing_and_optimize_sizer = new wxBoxSizer(wxHORIZONTAL);
    upgrade_level_and_tow_handing_and_optimize_sizer->Add(upgrade_level_and_two_handing_sizer, 0, wxEXPAND);
    upgrade_level_and_tow_handing_and_optimize_sizer->Add(this->optimize_for_panel, 0, wxEXPAND);
    upgrade_level_and_tow_handing_and_optimize_sizer->Add(this->optimize_panel, 0, wxEXPAND);
    root_sizer->Add(upgrade_level_and_tow_handing_and_optimize_sizer, 0, wxEXPAND);

    root_sizer->Add(this->result_panel, 0, wxEXPAND);

    root_panel->SetSizerAndFit(root_sizer);

    auto root_root_sizer = new wxBoxSizer(wxVERTICAL);
    root_root_sizer->Add(root_panel, 1, wxEXPAND);
    this->SetSizerAndFit(root_root_sizer);

    Bind(wxEVT_CHOICE, &MainFrame::OnChoiceSelected, this);
    Bind(wxEVT_SPINCTRL, &MainFrame::OnSpinCtrl, this);
    Bind(wxEVT_LISTBOX, &MainFrame::OnListBox, this); 
    Bind(wxEVT_BUTTON, &MainFrame::OnButton, this);
    Bind(wxEVT_RADIOBUTTON, &MainFrame::OnRadioButton, this);

    this->update_variation_labels();
}


