#pragma once

#include <map>

#include "model.h"

namespace app {

struct Player {
    std::shared_ptr<model::GameSession> session_;
    std::shared_ptr<model::Dog> dog_;
};

class Players {
   public:
    using Players_t = std::map<util::Token, std::shared_ptr<Player>>;
    explicit Players(model::Game& game);

    const std::vector<std::shared_ptr<model::Dog>>& GetListDogInRoom(const util::Token&) const noexcept(false);
    std::pair<std::shared_ptr<Player>, util::Token> AddPlayer(std::string_view dog_name, const model::Map::Id& map_id) noexcept(false);
    std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, const model::Map::Id& map_id);
    std::shared_ptr<Player> FindByToken(const util::Token&) const;

    void MovePlayer(const util::Token&, std::string_view direction) const noexcept(false);
 
    const auto & GetPlayersList() const { return players_; }

    void SetPlayersList(const Players_t & players) {
        players_ = players;
    }

   private:
    std::shared_ptr<Player> GetPlayerWithCheck(const util::Token&) const noexcept(false);

    Players_t players_;
    model::Game& game_;
};

}  // namespace app