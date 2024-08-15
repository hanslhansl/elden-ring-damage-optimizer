#include "ui.hpp"


bool ui::MyApp::OnInit()
{
    MainFrame* frame = new MainFrame();
    frame->Show();
    return true;
}


ui::FullStatsPanel::FullStatsPanel(wxWindow* parent, wxWindowID attribute_ctrls_id) : wxPanel(parent)
{
    wxBoxSizer* root_sizer = new wxBoxSizer(wxHORIZONTAL);

    for (auto&& [attribute_name, spin_ctrl] : std::views::zip(this->attribute_names, this->attribute_ctrls))
    {
        wxBoxSizer* stat_sizer = new  wxBoxSizer(wxVERTICAL);
        stat_sizer->Add(new wxStaticText(this, wxID_ANY, attribute_name));
        spin_ctrl = new wxSpinCtrl(this, attribute_ctrls_id, "1", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 1, 99);
        stat_sizer->Add(spin_ctrl);
        root_sizer->Add(stat_sizer);
    }

    SetSizerAndFit(root_sizer);
}

calculator::FullStats ui::FullStatsPanel::get_full_stats() const
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


ui::StatsPanel::StatsPanel(wxWindow* parent) : wxPanel(parent)
{
    wxBoxSizer* root_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* minimum_stats_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "minimum stats");

    wxBoxSizer* class_sizer = new  wxBoxSizer(wxHORIZONTAL);
    class_sizer->Add(new wxStaticText(this, wxID_ANY, wxT("starting class: ")));
    this->class_choice = new wxChoice(this, ID_STAT_CLASS_CHOICE, wxDefaultPosition, wxDefaultSize, this->starting_class_names.size(), this->starting_class_names.data());
    class_sizer->Add(this->class_choice);
    minimum_stats_sizer->Add(class_sizer);
    this->full_stats_panel = new FullStatsPanel(this, ID_STAT_SPINCTRLS);
    minimum_stats_sizer->Add(this->full_stats_panel);
    root_sizer->Add(minimum_stats_sizer);

    wxStaticBoxSizer* player_level_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "character level");
    this->player_level_ctrl = new wxSpinCtrl(this, ID_STAT_SPINCTRLS, "1", wxDefaultPosition, { 80, 20 }, wxSP_ARROW_KEYS, 1, 713);
    player_level_static_sizer->Add(this->player_level_ctrl);
    root_sizer->Add(player_level_static_sizer);

    wxStaticBoxSizer* attribute_points_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "attribute points");
    this->attribute_points_text = new wxStaticText(this, wxID_ANY, "0");
    attribute_points_static_sizer->Add(this->attribute_points_text);
    root_sizer->Add(attribute_points_static_sizer);

    wxStaticBoxSizer* stat_variations_static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "stat variations");
    this->stat_variations_text = new wxStaticText(this, wxID_ANY, "0");
    stat_variations_static_sizer->Add(this->stat_variations_text);
    root_sizer->Add(stat_variations_static_sizer);

    SetSizerAndFit(root_sizer);
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


ui::UpgradeLevelPanel::UpgradeLevelPanel(wxWindow* parent, wxWindowID spin_ctrls_id) : wxPanel(parent)
{
	wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "upgrade level");

    wxBoxSizer* normal_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    normal_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "normal: ", wxDefaultPosition, { 50, 20 }), 0, wxEXPAND | wxALL, 2);
    this->normal_upgrade_level_ctrl = new wxSpinCtrl(this, spin_ctrls_id, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 25);
    normal_upgrade_level_sizer->Add(this->normal_upgrade_level_ctrl, 0, wxEXPAND | wxALL, 2);
    static_sizer->Add(normal_upgrade_level_sizer);

    wxBoxSizer* somber_upgrade_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    somber_upgrade_level_sizer->Add(new wxStaticText(this, wxID_ANY, "somber: ", wxDefaultPosition, { 50, 20 }), 0, wxEXPAND | wxALL, 2);
    this->somber_upgrade_level_ctrl = new wxSpinCtrl(this, spin_ctrls_id, "0", wxDefaultPosition, { 40, 20 }, wxSP_ARROW_KEYS, 0, 10);
    somber_upgrade_level_sizer->Add(this->somber_upgrade_level_ctrl, 0, wxEXPAND | wxALL, 2);
    static_sizer->Add(somber_upgrade_level_sizer);

	SetSizerAndFit(static_sizer);
}

calculator::UpgradeLevels ui::UpgradeLevelPanel::get_upgrade_levels() const
{
    return { 0, this->normal_upgrade_level_ctrl->GetValue(), this->somber_upgrade_level_ctrl->GetValue() };
}


ui::TwoHandingPanel::TwoHandingPanel(wxWindow* parent, wxWindowID checkbox_id) : wxPanel(parent)
{
    wxStaticBoxSizer* static_sizer = new wxStaticBoxSizer(wxVERTICAL, this);

	this->two_handing_checkbox = new wxCheckBox(this, checkbox_id, "two handing");
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
	wxStaticBoxSizer* root_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "optimize");

    wxBoxSizer* total_variations_and_threads_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* total_variations_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "variations");
    this->total_variations_text = new wxStaticText(this, wxID_ANY, "0");
    total_variations_sizer->Add(this->total_variations_text, 0, wxEXPAND);
    total_variations_and_threads_sizer->Add(total_variations_sizer, 1, wxEXPAND);

    wxBoxSizer* threads_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "threads");
    this->threads_ctrl = new wxSpinCtrl(this, ID_OPTIMIZE_THREADS_SPINCTRL, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100);
    threads_sizer->Add(this->threads_ctrl, 1, wxEXPAND);
    total_variations_and_threads_sizer->Add(threads_sizer, 1, wxEXPAND);

    root_sizer->Add(total_variations_and_threads_sizer, 0, wxEXPAND);

    wxBoxSizer* start_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* brute_force_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "brute force");
    this->brute_force_button = new wxButton(this, ID_OPTIMIZE_BRUTE_FORCE_BUTTON, "start");
    brute_force_sizer->Add(this->brute_force_button, 0, wxEXPAND);
    wxStaticBoxSizer* eta_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "eta");
    this->brute_force_eta_text = new wxStaticText(this, wxID_ANY, "nan");
    eta_sizer->Add(this->brute_force_eta_text, 0, wxEXPAND);
    brute_force_sizer->Add(eta_sizer, 0, wxEXPAND);
    start_sizer->Add(brute_force_sizer, 0, wxEXPAND);

    wxStaticBoxSizer* v2_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "v2");
    this->v2_button = new wxButton(this, ID_OPTIMIZE_V2_BUTTON, "start");
    v2_sizer->Add(this->v2_button, 0, wxEXPAND);
    wxStaticBoxSizer* v2_eta_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "eta");
    this->v2_eta_text = new wxStaticText(this, wxID_ANY, "nan");
    v2_eta_sizer->Add(this->v2_eta_text, 0, wxEXPAND);
    v2_sizer->Add(v2_eta_sizer, 0, wxEXPAND);
    start_sizer->Add(v2_sizer, 0, wxEXPAND);

    root_sizer->Add(start_sizer, 0, wxEXPAND);

	SetSizerAndFit(root_sizer);
}


ui::ResultPanel::ResultPanel(wxWindow* parent) : wxPanel(parent)
{
	wxStaticBoxSizer* root_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "result");

    wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* first_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* stats_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "stats");
    this->full_stats_panel = new FullStatsPanel(this, ID_RESULT_STAT_SPINCTRLS);
    stats_sizer->Add(this->full_stats_panel);
    first_level_sizer->Add(stats_sizer);
    wxBoxSizer* upgrade_level_and_two_hand_sizer = new wxBoxSizer(wxVERTICAL);
    this->upgrade_level_panel = new UpgradeLevelPanel(this, ID_RESULT_UPGRADE_LEVEL_SPINCTRLS);
    upgrade_level_and_two_hand_sizer->Add(this->upgrade_level_panel);
    this->two_handing_panel = new TwoHandingPanel(this, ID_RESULT_TWO_HANDING_CHECKBOX);
    upgrade_level_and_two_hand_sizer->Add(this->two_handing_panel);
    first_level_sizer->Add(upgrade_level_and_two_hand_sizer);
    left_sizer->Add(first_level_sizer);

    wxBoxSizer* second_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBoxSizer* base_names_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "name");
    this->base_names_list = new wxListBox(this, ID_RESULT_BASE_NAMES_LIST, wxDefaultPosition, { 200, 200 }, {}, wxLB_SINGLE);
    base_names_sizer->Add(this->base_names_list);
    second_level_sizer->Add(base_names_sizer);
    wxStaticBoxSizer* affinities_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "affinity");
    this->affinities_list = new wxListBox(this, ID_RESULT_AFFINITIES_LIST, wxDefaultPosition, { 200, 200 }, {}, wxLB_SINGLE);
    affinities_sizer->Add(this->affinities_list);
    second_level_sizer->Add(affinities_sizer);
    left_sizer->Add(second_level_sizer);

    root_sizer->Add(left_sizer);


    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* right_first_level_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBoxSizer* full_name_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "full name");
    this->full_name_text = new wxStaticText(this, wxID_ANY, "");
    full_name_sizer->Add(this->full_name_text, 0, wxEXPAND);
    right_first_level_sizer->Add(full_name_sizer, 0, wxEXPAND | wxRIGHT, 5);

    wxStaticBoxSizer* type_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "type");
    this->type_text = new wxStaticText(this, wxID_ANY, "");
    type_sizer->Add(this->type_text, 0, wxEXPAND);
    right_first_level_sizer->Add(type_sizer, 0, wxEXPAND | wxRIGHT, 5);

    wxStaticBoxSizer* dlc_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "base game/dlc");
    this->dlc_text = new wxStaticText(this, wxID_ANY, "");
    dlc_sizer->Add(this->dlc_text, 0, wxEXPAND);
    right_first_level_sizer->Add(dlc_sizer, 0, wxEXPAND);

    right_sizer->Add(right_first_level_sizer, 0, wxEXPAND);

    int border = 2;

    this->right_second_level_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* attack_power_and_spell_scaling_sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* attack_power_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "attack power");
    wxFlexGridSizer* attack_power_flex_sizer = new wxFlexGridSizer(calculator::DamageType::_size() + 1, 5, 0, 0);
    attack_power_flex_sizer->SetFlexibleDirection(wxBOTH);
    attack_power_flex_sizer->Add(new wxStaticText(this, wxID_ANY, "total:"), 0, wxALL, border);
    attack_power_flex_sizer->Add(this->total_damage_texts.at(0) = new wxGenericStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
    attack_power_flex_sizer->Add(this->total_damage_texts.at(1) = new wxGenericStaticText(this, wxID_ANY, "+0"), 0, wxALL, border);
    attack_power_flex_sizer->Add(new wxStaticText(this, wxID_ANY, "="), 0, wxALL, border);
    attack_power_flex_sizer->Add(this->total_damage_texts.at(2) = new wxGenericStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
    for (int i = 0; i < calculator::DamageType::_size(); i++)
    {
        std::string damage_type = calculator::DamageType::_from_integral(i)._to_string();
        std::ranges::transform(damage_type, damage_type.begin(), to_lower);
        auto& damage_type_text_array = this->damage_type_texts.at(i);

        attack_power_flex_sizer->Add(new wxStaticText(this, wxID_ANY, damage_type + ":"), 0, wxALL, border);
        attack_power_flex_sizer->Add(damage_type_text_array.at(0) = new wxGenericStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
        attack_power_flex_sizer->Add(damage_type_text_array.at(1) = new wxGenericStaticText(this, wxID_ANY, "+0"), 0, wxALL, border);
        attack_power_flex_sizer->Add(new wxStaticText(this, wxID_ANY, "="), 0, wxALL, border);
        attack_power_flex_sizer->Add(damage_type_text_array.at(2) = new wxGenericStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
    }
    attack_power_sizer->Add(attack_power_flex_sizer, 0, wxEXPAND);
    attack_power_and_spell_scaling_sizer->Add(attack_power_sizer, 0, wxEXPAND);

    wxStaticBoxSizer* spell_scaling_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "spell scaling");
    this->spell_scaling_text = new wxStaticText(this, wxID_ANY, "0");
    spell_scaling_sizer->Add(this->spell_scaling_text, 0, wxEXPAND);
    attack_power_and_spell_scaling_sizer->Add(spell_scaling_sizer, 0, wxEXPAND);

    this->right_second_level_sizer->Add(attack_power_and_spell_scaling_sizer, 0, wxEXPAND | wxRIGHT, 5);

    wxStaticBoxSizer* status_effect_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "status effect");
    wxFlexGridSizer* status_effect_flex_sizer = new wxFlexGridSizer(calculator::StatusType::_size() + 1, 5, 0, 0);
    status_effect_flex_sizer->SetFlexibleDirection(wxBOTH);
    for (int i = 0; i < calculator::StatusType::_size(); i++)
    {
        std::string status_type = calculator::StatusType::_from_index(i)._to_string();
        std::ranges::transform(status_type, status_type.begin(), to_lower);
        auto& status_effect_text_array = this->status_effect_texts.at(i);

        status_effect_flex_sizer->Add(new wxStaticText(this, wxID_ANY, status_type + ":"), 0, wxALL, border);
        status_effect_flex_sizer->Add(status_effect_text_array.at(0) = new wxStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
        status_effect_flex_sizer->Add(status_effect_text_array.at(1) = new wxStaticText(this, wxID_ANY, "+0"), 0, wxALL, border);
        status_effect_flex_sizer->Add(new wxStaticText(this, wxID_ANY, "="), 0, wxALL, border);
        status_effect_flex_sizer->Add(status_effect_text_array.at(2) = new wxStaticText(this, wxID_ANY, "0"), 0, wxALL, border);
    }
    status_effect_sizer->Add(status_effect_flex_sizer, 0, wxEXPAND);

    this->right_second_level_sizer->Add(status_effect_sizer, 0, wxEXPAND);

    right_sizer->Add(this->right_second_level_sizer, 0, wxEXPAND);

    root_sizer->Add(right_sizer, 0, wxEXPAND | wxLEFT, 10);

	SetSizerAndFit(root_sizer);
}


void ui::MainFrame::brute_force()
{
    // stat variations
    auto full_stats = this->stats_panel->full_stats_panel->get_full_stats();
    auto min_stats = calculator::full_stats_to_stats(full_stats);
    auto player_level = this->stats_panel->player_level_ctrl->GetValue();
    auto total_attribute_points = player_level + 79;
    auto available_attribute_points = total_attribute_points
        - full_stats.at(0)
        - full_stats.at(1)
        - full_stats.at(2);

    auto stat_variations = calculator::get_stat_variations(available_attribute_points, min_stats);
	auto stat_variation_count = stat_variations.size();

    // weapon count
    auto filtered_weapon_count = this->filtered_weapons.size();

    // upgrade levels
    auto upgrade_levels = this->upgrade_level_panel->get_upgrade_levels();

    // total variations
    auto total_variations = stat_variation_count * filtered_weapon_count;

    // two handing
    bool two_handing = this->two_handing_panel->two_handing_checkbox->GetValue();

    // threads
    int threads = this->optimize_panel->threads_ctrl->GetValue();

    // set up attack options
    calculator::AttackOptions atk_options = { upgrade_levels, two_handing };

    if (total_variations == 0)
    {
        wxMessageBox("number of total variations is 0, no optimization possible!", "no optimization possible", wxOK | wxICON_INFORMATION);
        return;
    }

    // run optimization
    std::unique_ptr<calculator::OptimizationContext> opt_context;
    if (this->optimize_for_panel->total_attack_power_button->GetValue())
        opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::total>{});
    else if (this->optimize_for_panel->individual_attack_power_button->GetValue())
    {
        auto index = this->optimize_for_panel->individual_attack_power_list->GetSelection();

        if (index == calculator::DamageType::PHYSICAL)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::physical>{});
		else if (index == calculator::DamageType::MAGIC)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::magic>{});
        else if (index == calculator::DamageType::FIRE)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::fire>{});
        else if (index == calculator::DamageType::LIGHTNING)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::lightning>{});
        else if (index == calculator::DamageType::HOLY)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::holy>{});
		else
			throw std::runtime_error("unknown damage type");
    }
    else if (this->optimize_for_panel->status_effect_button->GetValue())
    {
        auto index = this->optimize_for_panel->status_effect_list->GetSelection() + calculator::StatusType::POISON;

        if (index == calculator::StatusType::POISON)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::poison_status>{});
        else if (index == calculator::StatusType::SCARLET_ROT)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::scarlet_rot_status>{});
		else if (index == calculator::StatusType::BLEED)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::bleed_status>{});
		else if (index == calculator::StatusType::FROST)
			opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::frost_status>{});
        else if (index == calculator::StatusType::SLEEP)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::sleep_status>{});
        else if (index == calculator::StatusType::MADNESS)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::madness_status>{});
        else if (index == calculator::StatusType::DEATH_BLIGHT)
            opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::death_blight_status>{});
        else
            throw std::runtime_error("unknown status type");
    }
    else if (this->optimize_for_panel->spell_scaling_button->GetValue())
        opt_context = std::make_unique<calculator::OptimizationContext>(threads, stat_variations, this->filtered_weapons, atk_options, std::type_identity<calculator::AttackRating::spell_scaling>{});
    else
        throw std::runtime_error("unknown optimization type");

    // show a progress bar and wait for optimization to finish
    {
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        wxProgressDialog progressDialog("brute force", "running brute force optimization...", filtered_weapon_count, this, wxPD_APP_MODAL | wxPD_CAN_ABORT);
        bool was_canceled = false;
        while (opt_context->pool.wait_for(std::chrono::milliseconds(50)) == false)
        {
            if (was_canceled == false)
            {
                auto is_canceled = !progressDialog.Update(filtered_weapon_count - opt_context->pool.get_tasks_total());
                if (is_canceled == true)
                {
                    opt_context->pool.purge();
                    was_canceled = true;
                }
            }
            else
            {
                progressDialog.Pulse();
            }

            wxYield(); // keep the ui alive
        }

        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        if (was_canceled == false)
        {
            size_t thread_count;
            if (threads > 0)
                thread_count = threads;
            else if (std::thread::hardware_concurrency() > 0)
                thread_count = std::thread::hardware_concurrency();
            else
                thread_count = 1;
            auto active_thread_count = std::min(thread_count, filtered_weapon_count);

			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
            if (active_thread_count != 0 && seconds != 0)
            {
                this->variations_per_second_per_thread_brute_force = total_variations / active_thread_count / seconds;
                this->update_variation_labels();
            }
        }
    }

    // get result
    auto attack_rating = opt_context->wait_and_get_result();

    // show stats
    auto new_full_stats = calculator::merge_stats_and_full_stats(attack_rating.stats, full_stats);
    this->set_result_attack_rating(attack_rating, new_full_stats);
}

void ui::MainFrame::update_variation_labels()
{
    // stat variations
    auto full_stats = this->stats_panel->full_stats_panel->get_full_stats();
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

    // thread count
	size_t thread_count = this->optimize_panel->threads_ctrl->GetValue();
    if (thread_count == 0)
    {
        if (std::thread::hardware_concurrency() > 0)
            thread_count = std::thread::hardware_concurrency();
        else
            thread_count = 1;
    }
    auto active_thread_count = std::min(thread_count, filtered_weapon_count);

    // brute force eta
    if (this->variations_per_second_per_thread_brute_force == 0)
        this->optimize_panel->brute_force_eta_text->SetLabel("nan");
    else
    {
        auto eta = total_variations / this->variations_per_second_per_thread_brute_force / active_thread_count;
        this->optimize_panel->brute_force_eta_text->SetLabel(std::to_string(eta) + "s");
    }

    // v2 eta
    if (this->variations_per_second_per_thread_v2 == 0)
		this->optimize_panel->v2_eta_text->SetLabel("nan");
	else
	{
		auto eta = total_variations / this->variations_per_second_per_thread_v2 / active_thread_count;
		this->optimize_panel->v2_eta_text->SetLabel(std::to_string(eta) + "s");
	}
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
    this->result_panel->affinities_list->Clear();
    for (auto&& affinity : this->all_filter_options.affinities)
    {
        std::string affinity_str = affinity._to_string();
        std::ranges::transform(affinity_str, affinity_str.begin(), to_lower);
        this->filter_panel->affinities_list->Append(affinity_str);
        this->result_panel->affinities_list->Append(affinity_str);
    }

    this->filter_panel->base_names_list->Clear();
    this->result_panel->base_names_list->Clear();
    for (auto&& base_name : this->all_filter_options.base_names)
    {
        this->filter_panel->base_names_list->Append(base_name);
        this->result_panel->base_names_list->Append(base_name);
    }

    this->update_filtered_weapons();
}

void ui::MainFrame::update_result(wxCommandEvent&)
{
    calculator::AttackRating::full attack_rating{};

    calculator::AttackOptions attack_options = {
        this->result_panel->upgrade_level_panel->get_upgrade_levels(),
        this->result_panel->two_handing_panel->two_handing_checkbox->GetValue()
    };

    auto full_stats = this->result_panel->full_stats_panel->get_full_stats();
    auto stats = calculator::full_stats_to_stats(full_stats);

    auto full_name = this->result_panel->base_names_list->GetStringSelection();

    auto base_name_index = this->result_panel->base_names_list->GetSelection();
    auto affinity_index = this->result_panel->affinities_list->GetSelection();
    if (base_name_index != wxNOT_FOUND && affinity_index != wxNOT_FOUND)
    {
        auto base_name = this->all_filter_options.base_names.at(base_name_index);

        auto affinity = this->all_filter_options.affinities.at(affinity_index);

        auto weapon_it = std::ranges::find_if(this->weapon_container.weapons, [&](const calculator::Weapon& w) { return w.affinity == affinity && w.base_name == base_name; });
        if (weapon_it != this->weapon_container.weapons.end())
            weapon_it->get_attack_rating(attack_options, stats, attack_rating);
    }
    
    this->set_result_attack_rating(attack_rating, full_stats);
}

void ui::MainFrame::set_result_attack_rating(const calculator::AttackRating::full& attack_rating, const calculator::FullStats& full_stats)
{
    if (attack_rating.weapon == nullptr)
    {
        this->result_panel->full_name_text->SetLabel("");
        this->result_panel->type_text->SetLabel("");
        this->result_panel->dlc_text->SetLabel("");
        this->result_panel->spell_scaling_text->SetLabel("");
    }
    else
    {
        for (auto&& [stat, ctrl] : std::views::zip(full_stats, this->result_panel->full_stats_panel->attribute_ctrls))
            ctrl->SetValue(stat);
        this->result_panel->upgrade_level_panel->normal_upgrade_level_ctrl->SetValue(attack_rating.upgrade_levels[1]);
        this->result_panel->upgrade_level_panel->somber_upgrade_level_ctrl->SetValue(attack_rating.upgrade_levels[2]);
        this->result_panel->two_handing_panel->two_handing_checkbox->SetValue(attack_rating.two_handing);

        auto base_name_it = std::ranges::find(this->all_filter_options.base_names, attack_rating.weapon->base_name);
        if (base_name_it != this->all_filter_options.base_names.end())
			this->result_panel->base_names_list->SetSelection(std::distance(this->all_filter_options.base_names.begin(), base_name_it));

        auto affinity_it = std::ranges::find(this->all_filter_options.affinities, attack_rating.weapon->affinity);
        if (affinity_it != this->all_filter_options.affinities.end())
            this->result_panel->affinities_list->SetSelection(std::distance(this->all_filter_options.affinities.begin(), affinity_it));

        std::string full_name = attack_rating.weapon->full_name;
        std::ranges::transform(full_name, full_name.begin(), to_lower);
        this->result_panel->full_name_text->SetLabel(full_name);

        std::string type = attack_rating.weapon->type._to_string();
        std::ranges::transform(type, type.begin(), to_lower);
        this->result_panel->type_text->SetLabel(type);

        this->result_panel->dlc_text->SetLabel(attack_rating.weapon->dlc ? "dlc" : "base game");
    }

    bool contains = false;
    for (auto&& [index, aps_txts] : std::views::enumerate(std::views::zip(attack_rating.attack_power, this->result_panel->damage_type_texts)))
    {
        auto&& [aps, txts] = aps_txts;
        auto attack_power_type = calculator::AttackPowerType::_from_integral(index);

        txts.at(0)->SetLabel(std::format("{0:}", int(aps.at(0))));
        if (std::ranges::contains(attack_rating.ineffective_attack_power_types, attack_power_type))
        {
            contains = true;
            txts.at(1)->SetLabelMarkup(std::format("<span foreground='red'>{0:+}</span>", int(aps.at(1))));
            txts.at(2)->SetLabelMarkup(std::format("<span foreground='red'>{0:}</span>", int(aps.at(2))));
        }
        else
        {
            txts.at(1)->SetLabel(std::format("{0:+}", int(aps.at(1))));
            txts.at(2)->SetLabel(std::format("{0:}", int(aps.at(2))));
        }
    }

    this->result_panel->total_damage_texts.at(0)->SetLabel(std::format("{0:}", int(attack_rating.total_attack_power.at(0))));
    if (contains)
    {
        this->result_panel->total_damage_texts.at(1)->SetLabelMarkup(std::format("<span foreground='red'>{0:+}</span>", int(attack_rating.total_attack_power.at(1))));
        this->result_panel->total_damage_texts.at(2)->SetLabelMarkup(std::format("<span foreground='red'>{0:}</span>", + int(attack_rating.total_attack_power.at(2))));
    }
    else
    {
        this->result_panel->total_damage_texts.at(1)->SetLabel(std::format("{0:+}", int(attack_rating.total_attack_power.at(1))));
        this->result_panel->total_damage_texts.at(2)->SetLabel(std::format("{0:}", int(attack_rating.total_attack_power.at(2))));
    }


    for (auto&& [ses, txts] : std::views::zip(attack_rating.status_effect, this->result_panel->status_effect_texts))
    {
        txts.at(0)->SetLabel(std::format("{0:}", int(ses.at(0))));
        txts.at(1)->SetLabel(std::format("{0:+}", int(ses.at(1))));
        txts.at(2)->SetLabel(std::format("{0:}", int(ses.at(2))));
    }

    this->result_panel->spell_scaling_text->SetLabel(std::to_string(int(attack_rating.spell_scaling)));

    this->result_panel->right_second_level_sizer->Layout();
    this->result_panel->Fit();
    this->Fit();
}

void ui::MainFrame::OnChoiceSelected(wxCommandEvent& event)
{
    auto id = event.GetId();

    if (id == ID_STAT_CLASS_CHOICE)
    {
        auto selection = this->stats_panel->class_choice->GetSelection();
        for (auto&& [spin_ctrl, attr_val] : std::views::zip(this->stats_panel->full_stats_panel->attribute_ctrls, this->stats_panel->starting_class_stats.at(selection)))
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
    else if (id == ID_UPGRADE_LEVEL_SPINCTRLS)
        ;
    else if (id == ID_OPTIMIZE_THREADS_SPINCTRL)
        this->update_variation_labels();
    else
        throw std::runtime_error("unknown spin ctrl id");
}

void ui::MainFrame::OnButton(wxCommandEvent& event)
{
    auto id = event.GetId();

	if (id == ID_OPTIMIZE_BRUTE_FORCE_BUTTON)
		this->brute_force();
	else
        wxMessageBox("This feature is not yet implemented.", "Not Implemented", wxOK | wxICON_INFORMATION);
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
    auto openFileDialog = wxFileDialog(this, "choose regulation data file", std::filesystem::current_path().string(), "",
        "json files (*.json)|*.json|all files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    std::filesystem::path path = openFileDialog.GetPath().ToStdString();
    if (std::filesystem::exists(path))
    {
        try
        {
            this->weapon_container = calculator::WeaponContainer(path);
        }
        catch (const std::exception& e)
        {
            wxLogError(e.what());
            return;
        }

        this->update_filter_options();
    }
}

void ui::MainFrame::OnGenerateRegulation(wxCommandEvent& event)
{
    auto msg_dialog = wxMessageDialog(this,
        "do you want to create a new regulation data file? this might be necessary if the provided file is out of date.\na more thorough explanation is provided on github.",
        "generate regulation data file", wxYES_NO | wxHELP | wxNO_DEFAULT);
    msg_dialog.SetYesNoLabels("yes", "no");
    msg_dialog.SetHelpLabel("explanation on github");

    auto result = msg_dialog.ShowModal();
    if(result == wxID_NO)
        return;
    if (result == wxID_HELP)
    {
        wxLaunchDefaultBrowser("https://github.com/hanslhansl/elden-ring-damage-optimizer");
        return;
    }

    auto uxm_dir_dialog = wxDirDialog(this, "choose uxm target directory", "", wxDD_DIR_MUST_EXIST);
    if (uxm_dir_dialog.ShowModal() == wxID_CANCEL)
        return;
    std::filesystem::path uxm_target_directory = uxm_dir_dialog.GetPath().ToStdString();

    auto witchy_exe_dialog = wxFileDialog(this, "choose WitchyBND executable", std::filesystem::current_path().string(), "",
        "exe files (*.exe)|*.exe|all files|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (witchy_exe_dialog.ShowModal() == wxID_CANCEL)
        return;
    std::filesystem::path witchy_exe = witchy_exe_dialog.GetPath().ToStdString();

    auto parser = calculator::Parser(witchy_exe, uxm_target_directory);
    auto regulation_data_json = parser.get_regulation_data_json();
    std::ofstream("regulation_data.json") << regulation_data_json << std::endl;
}

void ui::MainFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void ui::MainFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageDialog dialog(this, "created by hanslhansl", "about elden ring damage optimizer", wxOK | wxHELP | wxICON_NONE);
    dialog.SetOKLabel("ok");
    dialog.SetHelpLabel("source on github");

    if (dialog.ShowModal() == wxID_HELP)
        wxLaunchDefaultBrowser("https://github.com/hanslhansl/elden-ring-damage-optimizer");
}

void ui::MainFrame::OnCheckForUpdates(wxCommandEvent& event)
{
    std::string user = "hanslhansl";
    std::string repo = "elden-ring-damage-optimizer";
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/latest";

    // Perform a GET request using CPR
    cpr::Response r = cpr::Get(cpr::Url{ url }, cpr::Header{ {"accept", "application/vnd.github+json"} });

    if (r.status_code != 200)
    {
		wxLogError("failed to check for updates");
		return;
    }

    auto newest_tag_name = json::parse(r.text).at("tag_name").get<std::string>();
    
    auto msg = "installed version: " + std::string(current_tag_name) + "\nlatest version: " + newest_tag_name + "\n\nopen download page?";
    wxMessageDialog dialog(this, msg, "check for updates", wxOK | wxCANCEL | wxICON_NONE);
    dialog.SetOKCancelLabels("yes", "cancel");

    if (dialog.ShowModal() == wxID_OK)
        wxLaunchDefaultBrowser("https://github.com/hanslhansl/elden-ring-damage-optimizer/releases");
}

ui::MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "elden ring damage optimizer")
{
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_LOAD_REGULATION, "&open regulation file", "choose a regulation file from the filesystem");
    menuFile->Append(ID_GENERATE_REGULATION, "&generate regulation file", "not implemented");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT, "&quit", "quit this program");

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(ID_CHECK_FOR_UPDATES, "&check for updates");
    menuHelp->Append(wxID_ABOUT, "&about");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&file");
    menuBar->Append(menuHelp, "&help");

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetStatusText("created by hanslhansl");

    Bind(wxEVT_MENU, &MainFrame::OnLoadRegulation, this, ID_LOAD_REGULATION);
    Bind(wxEVT_MENU, &MainFrame::OnGenerateRegulation, this, ID_GENERATE_REGULATION);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnCheckForUpdates, this, ID_CHECK_FOR_UPDATES);
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);


    auto root_panel = new wxPanel(this);

    this->stats_panel = new StatsPanel(root_panel);
    this->filter_panel = new FilterPanel(root_panel);
    this->optimize_for_panel = new OptimizeForPanel(root_panel);
    this->upgrade_level_panel = new UpgradeLevelPanel(root_panel, ID_UPGRADE_LEVEL_SPINCTRLS);
    this->two_handing_panel = new TwoHandingPanel(root_panel, ID_TWO_HANDING_CHECKBOX);
    this->optimize_panel = new OptimizePanel(root_panel);
    this->result_panel = new ResultPanel(root_panel);

    auto root_sizer = new wxBoxSizer(wxVERTICAL);

    root_sizer->Add(this->stats_panel, 0, wxEXPAND);

    root_sizer->Add(this->filter_panel, 0, wxEXPAND);


    wxBoxSizer* upgrade_level_and_two_handing_and_optimize_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* upgrade_level_and_two_handing_sizer = new wxBoxSizer(wxVERTICAL);
    upgrade_level_and_two_handing_sizer->Add(this->upgrade_level_panel, 0, wxEXPAND);
    upgrade_level_and_two_handing_sizer->Add(this->two_handing_panel, 0, wxEXPAND);
    upgrade_level_and_two_handing_and_optimize_sizer->Add(upgrade_level_and_two_handing_sizer, 0, wxEXPAND);

    wxBoxSizer* optimize_sizer = new wxBoxSizer(wxHORIZONTAL);
    optimize_sizer->Add(this->optimize_for_panel, 0, wxEXPAND);
    optimize_sizer->Add(this->optimize_panel, 0, wxEXPAND);
    upgrade_level_and_two_handing_and_optimize_sizer->Add(optimize_sizer, 0, wxEXPAND);

    root_sizer->Add(upgrade_level_and_two_handing_and_optimize_sizer, 0, wxEXPAND);

    root_sizer->Add(this->result_panel, 0, wxEXPAND | wxTOP, 20);

    root_panel->SetSizerAndFit(root_sizer);

    auto root_root_sizer = new wxBoxSizer(wxVERTICAL);
    root_root_sizer->Add(root_panel, 1, wxEXPAND);
    this->SetSizerAndFit(root_root_sizer);

    this->result_panel->Bind(wxEVT_SPINCTRL, &MainFrame::update_result, this); 
    this->result_panel->Bind(wxEVT_LISTBOX, &MainFrame::update_result, this);
    this->result_panel->Bind(wxEVT_CHECKBOX, &MainFrame::update_result, this);

    Bind(wxEVT_CHOICE, &MainFrame::OnChoiceSelected, this);
    Bind(wxEVT_SPINCTRL, &MainFrame::OnSpinCtrl, this);
    Bind(wxEVT_LISTBOX, &MainFrame::OnListBox, this); 
    Bind(wxEVT_BUTTON, &MainFrame::OnButton, this);
    Bind(wxEVT_RADIOBUTTON, &MainFrame::OnRadioButton, this);

    auto path = std::filesystem::path("regulation_data.json");
    if (std::filesystem::exists(path))
    {
        try
        {
            this->weapon_container = calculator::WeaponContainer(path);
        }
        catch (const std::exception& e)
        {
            wxLogError(e.what());
            return;
        }

        this->update_filter_options();
    }
    
    this->update_variation_labels();
}


