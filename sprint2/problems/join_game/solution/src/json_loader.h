#pragma once

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <regex>

// TODO подумать над рефлекисей JSON моделей

using ptree = boost::property_tree::ptree;

namespace json_loader {

template <typename T>
void ProcessChildNodes(const ptree& tree, const std::string& child_name, std::vector<T>& container) {
    for (const auto& [_, node] : tree.get_child(child_name)) {
        container.push_back({node});
    }
}

template <typename T>
void ProcessChildNodes(ptree& parent, T& object) {
    if constexpr (requires { typename T::value_type::element_type; })
        for (auto& obj : object) parent.push_back({"", obj->GetJsonNode()});
    else
        for (auto& obj : object) parent.push_back({"", obj.GetJsonNode()});
}

template <class... Args>
ptree CreateTrivialJson(const std::array<std::string, sizeof...(Args)> names, Args&&... args) {
    ptree tree;
    size_t i = 0;
    ((tree.put(names[i++], std::forward<decltype(args)>(args))), ...);
    return tree;
}

// DEBUG пока только для тестов, если не устроят записи по типу "y0" : "0", а
// нужны будут "y0" : 0, буду думать
std::string _removeAllQuotesFromNumbers(const std::string& str);

class JsonObject {
   public:
    JsonObject() = default;

    std::string GetJson() const;

    void LoadJson(std::string_view sv);

    static std::string GetJson(const ptree& tree, bool is_format = true);

    void LoadJsonFromFile(const std::filesystem::path& path);

    virtual void LoadJsonNode(const ptree&) = 0;
    virtual ptree GetJsonNode() const = 0;
};

}  // namespace json_loader
