#include "domain_db.h"

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/lexical_cast.hpp>

std::string domain::RetiredPlayer::GenerateUuid() { 
    auto uuid_impl = boost::uuids::random_generator()();
    auto uuid = boost::lexical_cast<std::string>(uuid_impl);
    return uuid;
}
