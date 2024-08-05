#pragma once


#define _CRT_SECURE_NO_WARNINGS
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <set>
#include <map>
#include <array>
#include <vector>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include <better-enums/enum.h>

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/activityindicator.h>
#include <wx/animate.h>
#include <wx/progdlg.h>
#include <wx/generic/stattextg.h>

#include "hhh/concepts.hpp"
#include "hhh/misc.hpp"
#include "hhh/math/functions.hpp"

#pragma comment(lib, "wxbase32u.lib")
#pragma comment(lib, "wxmsw32u_core.lib")

//using json = nlohmann::ordered_json;
using json = nlohmann::json;

using namespace hhh;