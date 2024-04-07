#pragma once

#include "players.h"

#include "use_case_impl_db.h"
#include "postgres.h"

namespace app {

// Facade
class App {
   public:
    explicit App(const std::filesystem::path& settings_json, const std::string & db_url);

    const Players& GetPlayers() const { return players_; }
    Players& GetMutablePlayers() { return players_; }

    const model::Game& GetGame() const { return game_; }
    model::Game& GetMutableGame() { return game_; }

    UseCases & GetUseCaseDB() { return use_case_db; }

    void SetTickEditAccess(bool is_open) { tick_edit_access_ = is_open; }
    bool GetTickEditAccess() { return tick_edit_access_; }

   private:

    postgres::Database database_;
    UseCasesImpl use_case_db{database_};

    model::Game game_;
    Players players_;

    bool tick_edit_access_;
};

}  // namespace app