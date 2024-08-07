#pragma once


#define _CRT_SECURE_NO_WARNINGS
#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <set>
#include <map>
#include <array>
#include <vector>
#include <string>
#include <variant>
#include <fstream>

#include <nlohmann/json.hpp>
#include <BS_thread_pool.hpp>
#include <better-enums/enum.h>
#include <pugixml.hpp>

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/activityindicator.h>
#include <wx/animate.h>
#include <wx/progdlg.h>
#include <wx/generic/stattextg.h>

//#pragma comment(lib, "wxbase32u.lib")
//#pragma comment(lib, "wxmsw32u_core.lib")

using json = nlohmann::json;

namespace misc
{
	template<typename T>
	concept is_coutable = requires(T t)
	{
		std::cout << t;
	};
	template<typename T>
	concept is_wcoutable = requires(T t)
	{
		std::wcout << t;
	};
	template<typename T>
	concept is_outable = (is_coutable<T> || is_wcoutable<T>);

	template<typename...Args>
	concept are_coutable = (is_coutable<Args> && ...);
	template<typename...Args>
	concept are_wcoutable = (is_wcoutable<Args> && ...);
	template<typename...Args>
	concept are_outable = (is_outable<Args> && ...);

	template<is_outable T>
	void print(T&& arg)
	{
		if constexpr (is_coutable<T>)	// std::cout is default
			std::cout << std::forward<T>(arg);
		else if constexpr (is_wcoutable<T>)
			std::wcout << std::forward<T>(arg);
	}
	template <are_outable...Args>
	void print(Args&&...args)
	{
		(print(std::forward<Args>(args)), ...);
	}
	template <are_outable...Args>
	void printl(Args&&...args)
	{
		print(std::forward<Args>(args)..., '\n');
	}

	template <typename T>
	std::pair<std::invoke_result_t<T&&>, std::chrono::nanoseconds> TimeFunctionExecution(T&& func)
	{
		std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

		auto&& result = func();

		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

		// Getting number of milliseconds as a double
		return { std::forward<decltype(result)>(result), t2 - t1 };
	}

	template <typename Map>
	typename std::remove_reference_t<Map>::mapped_type
		map_get(Map&& m, const typename std::remove_reference_t<Map>::key_type& key, const typename std::remove_reference_t<Map>::mapped_type& default_)
	{
		auto it = m.find(key);
		if (it == m.end())
			return default_;
		return it->second;
	}
	template <typename Map>
	typename Map::mapped_type& map_get(Map& m, const typename Map::key_type& key, typename Map::mapped_type& default_)
	{
		auto it = m.find(key);
		if (it == m.end())
			return default_;
		return it->second;
	}
	template <typename Map>
	const typename Map::mapped_type& map_get(const Map& m, const typename Map::key_type& key, const typename Map::mapped_type& default_)
	{
		auto it = m.find(key);
		if (it == m.end())
			return default_;
		return it->second;
	}
}