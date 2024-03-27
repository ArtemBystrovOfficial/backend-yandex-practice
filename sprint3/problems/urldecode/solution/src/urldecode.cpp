#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <string_view>

static bool CheckSpecialSymbol(char symbol) {
    return symbol >= '0' && symbol <= '9' || symbol >= 'a' && symbol <='f' || symbol >= 'A' && symbol <='F';
}

std::string UrlDecode(std::string_view sv) {
    std::string str = std::string(sv.data(), sv.size());
    auto it = str.begin();
    while (true) {
        it = std::find(it, str.end(), '%');
        if (it == str.end()) 
            break;
        if(std::distance(it,str.end()) < 3)
            throw std::invalid_argument("count of symbols after %");
        if(!CheckSpecialSymbol(*(it+1)) || !CheckSpecialSymbol(*(it+2)))
            throw std::invalid_argument("invalid symbols after %");
    
        char special = char(strtol(std::string((it + 1), (it + 3)).c_str(), 0, 16)); 
        if(!special)
            throw std::invalid_argument("invalid convert %");
        str.replace(it, it + 3, std::string(1, special));
        it++;
    }
    it = str.begin();
    while (true) {
        it = std::find(it + 1, str.end(), '+');
        if (it == str.end()) break;
        *it = ' ';
    }
    return str;
}
