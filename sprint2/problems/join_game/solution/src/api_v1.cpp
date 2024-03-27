#include "api_v1.h"

#include "common.h"
#include "error_codes.h"

namespace {
namespace http = boost::beast::http;
using namespace std::literals;
namespace ph = std::placeholders;
using ec = http_handler::ErrorCode;
}  // namespace

namespace api_v1 {

Api::Api(app::App& app) {
    api_classes_["maps"] = std::make_unique<Maps>(app);
    api_classes_["game"] = std::make_unique<Game>(app);
}

int Api::GetVersionCode() { return 0x1; }

void Api::HandleApi(HttpResource&& res) {
    if (res.args.empty()) throw ec::BAD_REQUEST;
    auto val = res.args.front();
    res.args.pop_front();

    auto it = api_classes_.find(val);
    if (it == api_classes_.end()) throw ec::BAD_REQUEST;

    HttpResource copy = res;

    if (!it->second->RedirectAutomatic(std::move(res))) throw method_handler::MakeAllowError(std::move(copy), it->second.get());
}

bool Game::GetHandler(HttpResource&& res, bool is_ping) {
    if (res.args.size() == 1) {
        auto arg = res.args.front();
        res.args.pop_front();
        if (arg == "players"sv) {
            CALL_WITH_PING(is_ping, GetPlayers(std::move(res)))
        }
    }
    return false;
}

bool Game::PostHandler(HttpResource&& res, bool is_ping) {
    if (res.args.size() == 1) {
        auto arg = res.args.front();
        res.args.pop_front();
        if (arg == "join"sv) {
            CALL_WITH_PING(is_ping, AddNewPlayer(std::move(res)));
        }
    }
    return false;
}

void Game::AddNewPlayer(HttpResource&& res) {
    if (ToSV(res.req[http::field::content_type]) != ContentType::JSON)
        throw ec::POST_NOT_ALLOWED;  // TODO сделать также автоматический опрос по всем типам ожидаемого контента
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    std::string name, map_id;
    try {
        auto tree = json_loader::JsonObject::GetTree(res.req.body());
        name = tree.get<std::string>("userName");
        map_id = tree.get<std::string>("mapId");
    } catch (...) {
        throw ec::JOIN_PLAYER_UNKNOWN;
    }
    auto [player, token] = app_.GetMutablePlayers().AddPlayer(std::make_shared<model::Dog>(name), model::Map::Id(map_id));

    if (!player) throw ec::JOIN_PLAYER_UNKNOWN;

    auto json = json_loader::CreateTrivialJson({"authToken", "playerId"}, *token, *player->dog_->GetId());

    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(json, false));
}

void Game::GetPlayers(HttpResource&& res) const {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    std::string_view token;

    try {
        token = ToSV(res.req[http::field::authorization]);

    } catch (...) {
        throw ec::AUTHORIZATION_NOT_EXIST;
    }

    if (token.substr(0, 7) == "Bearer "sv) token.remove_prefix(7);

    if (token.size() != util::TOKEN_SIZE) throw ec::AUTHORIZATION_NOT_EXIST;

    auto dogs = app_.GetPlayers().GetListDogInRoom(util::Token(std::string(token.begin(), token.end())));
    ptree list_json;
    for (const auto& dog : dogs) {
        ptree dog_json;
        dog_json.put("name", dog->GetName());
        list_json.add_child(std::to_string(*dog->GetId()), dog_json);
    }
    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(list_json, false, false));
}

bool Maps::GetHandler(HttpResource&& res, bool is_ping) {
    res.resp.set(http::field::content_type, ToBSV(ContentType::JSON));

    if (res.args.empty()) {
        CALL_WITH_PING(is_ping, common_pack::FillBody(res.resp, GetMapListJson()))
    } else if (res.args.size() == 1) {
        CALL_WITH_PING(is_ping, common_pack::FillBody(res.resp, GetMapDescriptionJson(res.args.back())));
    }

    return false;
}

std::string Maps::GetMapListJson() const { return app_.GetGame().GetJsonMaps(); }

std::string Maps::GetMapDescriptionJson(std::string_view id) const {
    auto map = app_.GetGame().FindMap(model::Map::Id(std::string(id.data(), id.size())));
    if (!map) throw ec::MAP_NOT_FOUNDED;
    return map->GetJson();
}

}  // namespace api_v1