#pragma once

#include "postgres.h"
#include "../unit/unit_of_work.h"

namespace postgres {

class UnitOfWorkImpl : public app::UnitOfWork {
    public:
        UnitOfWorkImpl(pqxx::connection & connection) : connection_(connection) {}

        void Commit() override {
            worker_.commit();
            is_commited_ = true;
        }
        AuthorRepositoryImpl & Authors() override {
            return authors_;
        }
        BookRepositoryImpl & Books() override {
            return books_;
        }
        ~UnitOfWorkImpl() {
            if(!is_commited_)
                Commit();
        }
    private:
        bool is_commited_{false};

        pqxx::connection & connection_;        
        pqxx::work worker_{connection_};

        postgres::AuthorRepositoryImpl authors_{worker_};
        postgres::BookRepositoryImpl books_{worker_};
};

}