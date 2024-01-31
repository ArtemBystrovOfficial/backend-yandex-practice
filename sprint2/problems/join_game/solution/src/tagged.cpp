#include "tagged.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
namespace util {

Token GenerateRandomToken() {
    auto uuid = boost::uuids::random_generator()();
    auto uuid_without_space = boost::lexical_cast<std::string>(uuid);
    uuid_without_space.erase(std::remove_if(uuid_without_space.begin(), uuid_without_space.end(), [](char c) { return c == '-'; }),
                             uuid_without_space.end());
    return Token(uuid_without_space);
}

}  // namespace util
