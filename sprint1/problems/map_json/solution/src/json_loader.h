#pragma once

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <regex>

using ptree = boost::property_tree::ptree;

namespace json_loader {

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
