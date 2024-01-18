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
    for (auto& road : object) parent.push_back({"", road.GetJsonNode()});
}

// DEBUG пока только для тестов, если не устроят записи по типу "y0" : "0", а
// нужны будут "y0" : 0, буду думать
std::string _removeAllQuotesFromNumbers(const std::string& str);

class JsonObject {
   public:
    JsonObject() = default;

    std::string GetJson() const {
        std::ostringstream oss;
        boost::property_tree::write_json(oss, GetJsonNode());
        return _removeAllQuotesFromNumbers(oss.str());
    }

    void LoadJson(std::string_view sv) {
        std::istringstream iss(std::move(std::string(sv)));
        ptree tree;
        boost::property_tree::read_json(iss, tree);
        LoadJsonNode(tree);
    }

    static std::string GetJson(const ptree& tree) {
        std::ostringstream oss;
        boost::property_tree::write_json(oss, tree);
        return oss.str();
    }

    void LoadJsonFromFile(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::in | std::ios::binary);
        const auto sz = std::filesystem::file_size(path);
        std::string result(sz, '\0');
        f.read(result.data(), sz);
        LoadJson(result);
    }

    virtual void LoadJsonNode(const ptree&) = 0;
    virtual ptree GetJsonNode() const = 0;
};

}  // namespace json_loader
