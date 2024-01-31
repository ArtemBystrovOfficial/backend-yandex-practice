#include "players.h"

#include "error_codes.h"

namespace {
using ec = http_handler::ErrorCodes;
}

namespace app {
Players::Players(model::Game& game) : game_(game) {}

std::pair<std::shared_ptr<Player>, util::Token> Players::AddPlayer(std::shared_ptr<model::Dog> dog, model::Map::Id map_id) {
    auto token = util::GenerateRandomToken();

    std::shared_ptr<model::GameSession> game_session;

    if (!game_.IsSessionStarted(map_id))
        game_session = game_.AddSession(map_id);
    else
        game_session = game_.GetSession(map_id);

    if (!game_session) throw ec::JOIN_PLAYER;

    game_session->AddDog(dog);

    auto player = std::make_shared<Player>(game_session, dog);
    players_[token] = player;
    return {player, token};
}
std::shared_ptr<Player> Players::FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) { return nullptr; }
std::shared_ptr<Player> Players::FindByToken(util::Token token) {
    auto it = players_.find(token);
    return it == players_.end() ? nullptr : it->second;
}
}  // namespace app