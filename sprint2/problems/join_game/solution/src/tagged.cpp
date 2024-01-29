#include "tagged.h"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
namespace util {

Token GenerateRandomToken() {
    auto uuid = boost::uuids::random_generator()();
    return Token(boost::lexical_cast<std::string>(uuid));
}

}  // namespace util
