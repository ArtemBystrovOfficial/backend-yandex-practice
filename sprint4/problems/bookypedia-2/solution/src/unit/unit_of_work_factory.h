#pragma once

#include "../postgres/unit_of_work_impl.h"
#include <memory>
#include <pqxx/pqxx>

namespace app {

class UnitOfWorkFactory {
public:
    UnitOfWorkFactory(pqxx::connection & conn);

    std::shared_ptr<UnitOfWork> CreateUnitOfWork();
private:
    pqxx::connection & conn_;
};

}
