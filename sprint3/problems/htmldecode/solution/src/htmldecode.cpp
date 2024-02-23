#include "htmldecode.h"
#include <algorithm>
#include <string_view>

std::string HtmlDecode(std::string_view str_sv) {
    std::string str(str_sv.begin(),str_sv.size());

    // Map для хранения соответствий подстрок и символов
    std::vector<std::pair<std::string, char>> replacements = {{"&lt", '<'},
                                                {"&gt", '>'},
                                                {"&apos", '\'' }, 
                                                {"&quot", '"' },
                                                {"&amp", '&' } //Последний так как может повторно свернуться
                                               };

    for (const auto& replacement : replacements) {
        char symbol = replacement.second;

        for(int i=0;i<2;i++) {
            std::string to_replace = replacement.first + ((i%2) ? "" : ";");
            for(int j=0;j<2;j++) {
                auto it = str.begin();
                while ((it = std::search(it, str.end(), to_replace.begin(), to_replace.end())) != str.end()) {
                    str.replace(it, it + to_replace.size(), 1, symbol);
                    it += to_replace.size();
                }
                transform(to_replace.begin(), to_replace.end(), to_replace.begin(), ::toupper);
            }
        }
    }

    return str;
}
