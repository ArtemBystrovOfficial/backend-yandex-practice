#include "players.h"

namespace auth {
Players::Players(model::Game& game) : game_(game) {}

std::pair<std::shared_ptr<Player>, util::Token> Players::AddPlayer(std::shared_ptr<Dog> dog, std::shared_ptr<model::GameSession> game_session){
    players_} std::shared_ptr<Player> Players::FindByDogAndMapId(Dog::Id dog_id, Map::Id map_id) {}
std::shared_ptr<Player> Players::FindByToken(util::Token) {}
}  // namespace auth