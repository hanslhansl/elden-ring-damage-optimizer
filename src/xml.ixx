module;
#include <pugixml.hpp>
export module erdo:xml;

import std;


namespace erdo::xml
{
    pugi::xml_document load_file(const std::filesystem::path &file_path) {
        pugi::xml_document data;
        auto result = data.load_file(file_path.c_str(), pugi::parse_default, pugi::encoding_utf8);
        if (!result)
            throw std::runtime_error(std::format("could not load xml file: {}", result.description()));
        return data;
    }

    template<typename T>
    auto get_value(const auto& attr, T def = {}) {
        if constexpr (std::same_as<T, double>)
            return attr.as_double(def);
        else if constexpr (std::same_as<T, long long>)
            return attr.as_llong(def);
        else if constexpr (std::same_as<T, std::string>)
            return std::string(attr.as_string(def.c_str()));
        else
            static_assert(false, "unsupported type");
    }

    template<typename T>
    T get_element_value(const pugi::xml_document& doc, const std::vector<std::string>& path, T def = {}) {
        pugi::xml_node node{};
        for (auto&& element : path)
        {
            if (node)
                node = node.child(element.c_str());
            else
                node = doc.child(element.c_str());

            if (!node)
                throw std::runtime_error(std::format("could not find element {}", element));
        }

        return get_value<T>(node.text(), def);
    }

    template<typename T>
        requires (std::same_as<T, double> || std::same_as<T, long long>)
    std::map<long long, std::map<std::string, T>> read_param_file(const std::filesystem::path &file_path) {
        pugi::xml_document data = load_file(file_path);

        auto field_nodes = data.child("param").child("fields").children("field");

        std::map<std::string, T> default_values{};
        for (auto &&field_node : field_nodes)
        {
            auto name = field_node.attribute("name").as_string();
            auto defaultValue = get_value<T>(field_node.attribute("defaultValue"), std::numeric_limits<T>::max());

            if (defaultValue != std::numeric_limits<T>::max())
                default_values.emplace(name, defaultValue);
        }

        auto row_nodes = data.child("param").child("rows").children("row");

        std::map<long long, std::map<std::string, T>> ret{};
        for (auto &&row_node : row_nodes)
        {
            auto name = row_node.attribute("name").as_string();

            std::map<std::string, T> row_data = default_values;
            for (auto &&row_attribute : row_node.attributes())
            {
                row_data[row_attribute.name()] = get_value<T>(row_attribute);
            }

            auto id = row_node.attribute("id").as_llong();
            ret.emplace(id, std::move(row_data));
        }

        return ret;
    }
    std::map<long long, std::string> read_fmg_file(const std::filesystem::path &file_path) {
        pugi::xml_document data = load_file(file_path);

        auto text_nodes = data.child("fmg").child("entries").children("text");

        std::map<long long, std::string> ret{};
        for (auto &&text_node : text_nodes)
        {
            auto id = text_node.attribute("id").as_llong();
            auto text = text_node.child_value();
            ret.emplace(id, text);
        }

        return ret;
    }
}

