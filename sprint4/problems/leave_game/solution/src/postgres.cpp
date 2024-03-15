#include "postgres.h"

#include <thread>
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <pqxx/transaction>

namespace postgres {

using pqxx::operator"" _zv;

Database::Database(const std::string & db_url)  
: pool_{
    std::thread::hardware_concurrency(),
    [db_url] {
        auto conn = std::make_shared<pqxx::connection>(db_url);
        return conn;
    }
} {
    auto connection = pool_.GetConnection();
    pqxx::work work{*connection};

    work.exec(R"(
    CREATE TABLE IF NOT EXISTS retired_players (
    id UUID PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    score int NOT NULL,
    play_time_ms int NOT NULL
    ); )"_zv);

    work.exec("CREATE INDEX IF NOT EXIST sort_retired_players retired_players(score DESC, play_time_ms, name)"_zv);

}

domain::RetiredPlayerRepository::retired_players_t RetiredPlayerRepositoryImpl::GetSortedRetiredPlayersList() {
    pqxx::read_transaction tx{*connection_}; 

    retired_players_t players;
    auto size = std::get<0>(tx.query1<int>("SELECT count(*) FROM retired_players;"));
    players.reserve(size);
    for(auto [name, score, time] : tx.query<std::string, int, int>("SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name;"_zv)) {
        players.push_back(domain::RetiredPlayer(name, score, time));
    }
    return players;
}

void RetiredPlayerRepositoryImpl::AddRetriedPlayer(const domain::RetiredPlayer& player) {
    pqxx::transaction tx{*connection_};

    tx.exec_params("INSERT INTO retired_players VALUES ($1,$2,$3,$4);",
    player.GetId(),player.GetName(),player.GetScore(),player.GetPlayTimeMs()
    );
}

}  // namespace postgres