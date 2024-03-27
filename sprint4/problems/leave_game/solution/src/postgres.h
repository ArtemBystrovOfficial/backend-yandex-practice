#pragma once

#include <pqxx/connection>

#include "domain_db.h"
#include "connection_pool.h"

namespace postgres {

using ConnectionUnit =  ConnectionPool::ConnectionWrapper;

//RAII 
//Обратим внимание, что только после разрушения AuthorRepositoryImpl соеденение вернется в pool
class RetiredPlayerRepositoryImpl : public domain::RetiredPlayerRepository {
public:
    explicit RetiredPlayerRepositoryImpl(ConnectionUnit && connection)
        : connection_(std::move(connection)) {
    }

    retired_players_t GetSortedRetiredPlayersList(int offset, int limit) override;
    virtual void AddRetriedPlayer(const domain::RetiredPlayer & player) override;
private:
    ConnectionUnit connection_;
};

class Database {
public:
    explicit Database(const std::string & db_url);

    ConnectionUnit GetConnection() { return pool_.GetConnection(); }
private:
    ConnectionPool pool_;
};

}
