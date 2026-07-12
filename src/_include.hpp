
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include <better-enums/enum.h>
#include <pugixml.hpp>
#include <cpr/cpr.h>


#include <wx/setup.h>
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/activityindicator.h>
#include <wx/animate.h>
#include <wx/progdlg.h>
#include <wx/generic/stattextg.h>


using json = nlohmann::json;


constexpr auto current_tag_name = "v2.0";
