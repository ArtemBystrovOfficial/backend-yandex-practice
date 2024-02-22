#include "urlencode.h"
#include <string_view>
#include <sstream>
#include <iomanip>

inline static auto CheckSymbol = [](char c) {
    using namespace std::literals;
    static constexpr auto  extra_symbols = "!#$&'()*+,/:;=?@[]"sv;
    return ( extra_symbols.find(c) == std::string_view::npos && c >= 32 && c < 128);
};

std::string UrlEncode(std::string_view str) {
    std::ostringstream ss;

    for (char sch : str) {
        uint8_t ch = static_cast<uint8_t>(sch);
        if(ch == ' ') {
            ss << '+';
            continue;
        }
        if(CheckSymbol(ch))
            ss << ch;
        else
            ss << '%' << std::setfill('0') << std::setw(2) << std::hex << short(ch);
    }
    return ss.str();
}
