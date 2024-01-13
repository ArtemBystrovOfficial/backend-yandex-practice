#include "json_loader.h"

namespace json_loader {
std::string _removeAllQuotesFromNumbers(const std::string& str) {
    std::regex regex("\"(\\d+)\"");
    std::string result = std::regex_replace(str, regex, "$1");
    return result;
}
}  // namespace json_loader
