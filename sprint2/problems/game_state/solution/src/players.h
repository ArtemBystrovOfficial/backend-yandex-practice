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
    Players(model::Game& game);

    const std::vector<std::shared_ptr<model::Dog>>& GetListDogInRoom(const util::Token&) const;
    std::pair<std::shared_ptr<Player>, util::Token> AddPlayer(std::string_view dog_name, const model::Map::Id& map_id);
    std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, const model::Map::Id& map_id);
    std::shared_ptr<Player> FindByToken(const util::Token&) const;

   private:
    std::shared_ptr<Player> GetPlayerWithCheck(const util::Token&) const noexcept(false);

    Players_t players_;
    model::Game& game_;
};

}  // namespace app