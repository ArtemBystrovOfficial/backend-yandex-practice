#pragma once

#include <map>

#include "model.h"

namespace auth {

struct Player {
    std::shared_ptr<model::GameSession> session_;
    std::shared_ptr<model::Dog> dog_;
};

class Players {
   public:
    using Players_t = std::map<util::Token, std::shared_ptr<model::Player>>;
    Players(model::Game& game);

    std::pair<std::shared_ptr<Player>, util::Token> AddPlayer(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game);
    std::shared_ptr<Player> FindByDogAndMapId(Dog::Id dog_id, Map::Id map_id);
    std::shared_ptr<Player> FindByToken(util::Token);

   private:
    Players_t players_;
    model::Game& game_;
};

}  // namespace auth