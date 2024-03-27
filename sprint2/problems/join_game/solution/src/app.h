#pragma once

#include "players.h"

namespace app {

// Facade
class App {
   public:
    App(const std::filesystem::path& settings_json);

    const Players& GetPlayers() const { return players_; }
    Players& GetMutablePlayers() { return players_; }

    const model::Game& GetGame() const { return game_; }
    model::Game& GetMutableGame() { return game_; }

   private:
    model::Game game_;
    Players players_;
};

}  // namespace app