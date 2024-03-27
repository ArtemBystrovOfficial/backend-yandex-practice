#include "tagged.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "error_codes.h"

namespace {
using namespace std::literals;
using ec = http_handler::ErrorCode;
}  // namespace

namespace util {

Token GenerateRandomToken() {
    auto uuid = boost::uuids::random_generator()();
    auto uuid_without_space = boost::lexical_cast<std::string>(uuid);
    uuid_without_space.erase(std::remove_if(uuid_without_space.begin(), uuid_without_space.end(), [](char c) { return c == '-'; }),
                             uuid_without_space.end());
    return Token(uuid_without_space);
}

Token CreateTokenByAuthorizationString(std::string_view token_raw) noexcept(false) {
    if (token_raw.substr(0, 7) == "Bearer "sv) token_raw.remove_prefix(7);
    if (token_raw.size() != util::TOKEN_SIZE) throw ec::AUTHORIZATION_NOT_EXIST;
    return Token{std::string(token_raw.data(), token_raw.size())};
}

}  // namespace util
