#include "unit_of_work_factory.h"

#include "../postgres/unit_of_work_impl.h"

namespace app {

UnitOfWorkFactory::UnitOfWorkFactory(pqxx::connection& conn) : conn_(conn) {}

std::shared_ptr<UnitOfWork> UnitOfWorkFactory::CreateUnitOfWork() {
    return std::make_shared<postgres::UnitOfWorkImpl>(conn_);
}

}  // namespace app
