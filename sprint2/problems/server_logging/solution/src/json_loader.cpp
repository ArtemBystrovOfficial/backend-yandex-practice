#include "json_loader.h"

namespace json_loader {
std::string _removeAllQuotesFromNumbers(const std::string& str) {
    std::regex regex("\"(\\d+)\"");
    std::string result = std::regex_replace(str, regex, "$1");
    return result;
}

std::string JsonObject::GetJson() const {
    std::ostringstream oss;
    boost::property_tree::write_json(oss, GetJsonNode());
    return _removeAllQuotesFromNumbers(oss.str());
}

void JsonObject::LoadJson(std::string_view sv) {
    std::istringstream iss(std::move(std::string(sv)));
    ptree tree;
    boost::property_tree::read_json(iss, tree);
    LoadJsonNode(tree);
}

std::string JsonObject::GetJson(const ptree& tree, bool is_format) {
    std::ostringstream oss;
    boost::property_tree::write_json(oss, tree, is_format);
    return oss.str();
}

void JsonObject::LoadJsonFromFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    const auto sz = std::filesystem::file_size(path);
    std::string result(sz, '\0');
    f.read(result.data(), sz);
    LoadJson(result);
}

}  // namespace json_loader
