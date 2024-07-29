#pragma once


#define _CRT_SECURE_NO_WARNINGS
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <set>
#include <map>
#include <array>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include <better-enums/enum.h>

#include <wx/wx.h>

#include "hhh/misc.hpp"
#include "hhh/math/functions.hpp"


//using json = nlohmann::ordered_json;
using json = nlohmann::json;

using namespace hhh;