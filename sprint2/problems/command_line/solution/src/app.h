#pragma once

#include "players.h"

namespace app {

// Facade
class App {
   public:
    explicit App(const std::filesystem::path& settings_json);

    const Players& GetPlayers() const { return players_; }
    Players& GetMutablePlayers() { return players_; }

    const model::Game& GetGame() const { return game_; }
    model::Game& GetMutableGame() { return game_; }

    void SetTickEditAccess(bool is_open) { tick_edit_access_ = is_open; }
    bool GetTickEditAccess() { return tick_edit_access_; }

   private:
    model::Game game_;
    Players players_;

    bool tick_edit_access_;
};

}  // namespace app