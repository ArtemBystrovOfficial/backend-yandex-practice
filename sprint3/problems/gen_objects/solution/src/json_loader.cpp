#include "json_loader.h"

#include <regex>

namespace json_loader {
std::string _removeAllQuotesFromNumbers(const std::string& str) {
    std::string jsonString;
    std::regex numberRegex("\"(-?\\d*\\.?\\d+)\"");  // Все числа будут без кавычек, если хотим число как строку добавляем в конце S
    std::regex regex_pattern("\"(-?\\d+)f\"");
    std::regex numberSRegex("\"(\\d*\\.?\\d+)S\"");
    jsonString = std::regex_replace(str, numberRegex, "$1");
    jsonString = std::regex_replace(jsonString, regex_pattern, "$1.0");
    jsonString = std::regex_replace(jsonString, numberSRegex, "\"$1\"");
    return jsonString;
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

ptree JsonObject::GetTree(std::string_view json) {
    ptree tree;
    std::istringstream iss(std::move(std::string(json)));
    boost::property_tree::read_json(iss, tree);
    return tree;
}

std::string JsonObject::GetJson(const ptree& tree, bool is_format, bool is_remove_q_) {
    std::ostringstream oss;
    boost::property_tree::write_json(oss, tree, is_format);
    return is_remove_q_ ? _removeAllQuotesFromNumbers(oss.str()) : oss.str();
}

void JsonObject::LoadJsonFromFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    const auto sz = std::filesystem::file_size(path);
    std::string result(sz, '\0');
    f.read(result.data(), sz);
    LoadJson(result);
}

}  // namespace json_loader
