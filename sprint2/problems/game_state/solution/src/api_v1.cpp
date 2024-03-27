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

//////// GAME ///////////

bool Game::GetHandler(HttpResource&& res, bool is_ping) {
    if (res.args.size() == 1) {
        auto arg = res.args.front();
        res.args.pop_front();
        if (arg == "players"sv) {
            CALL_WITH_PING(is_ping, GetPlayers(std::move(res)))
        }
        if (arg == "state"sv) {
            CALL_WITH_PING(is_ping, GetState(std::move(res)))
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
    auto [player, token] = app_.GetMutablePlayers().AddPlayer(name, model::Map::Id(map_id));

    if (!player) throw ec::JOIN_PLAYER_UNKNOWN;

    auto json = json_loader::CreateTrivialJson({"authToken", "playerId"}, *token, *player->dog_->GetId());

    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(json, false));
}

void Game::GetPlayers(HttpResource&& res) const {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    std::string_view token_raw;
    try {
        token_raw = ToSV(res.req[http::field::authorization]);
    } catch (...) {
        throw ec::AUTHORIZATION_NOT_EXIST;
    }

    auto dogs = app_.GetPlayers().GetListDogInRoom(util::CreateTokenByAuthorizationString(token_raw));
    ptree list_json;
    for (const auto& dog : dogs) {
        ptree dog_json;
        dog_json.put("name", dog->GetName());
        list_json.add_child(std::to_string(*dog->GetId()), dog_json);
    }
    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(list_json, false, false));
}

//////// MAPS ///////////

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

void Game::GetState(HttpResource&& res) const {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    std::string_view token_raw;
    try {
        token_raw = ToSV(res.req[http::field::authorization]);
    } catch (...) {
        throw ec::AUTHORIZATION_NOT_EXIST;
    }

    auto dogs = app_.GetPlayers().GetListDogInRoom(util::CreateTokenByAuthorizationString(token_raw));

    ptree main_json;  // TODO продумать как кастомные json файлы можно применить к системе модели
    ptree list_json;
    for (const auto& dog : dogs) {
        ptree dog_json;

        boost::property_tree::ptree pos_pt;
        pos_pt.push_back({"", ptree().put("", dog->GetPosition().x)});
        pos_pt.push_back({"", ptree().put("", dog->GetPosition().y)});
        boost::property_tree::ptree speed_pt;
        speed_pt.push_back({"", ptree().put("", dog->GetSpeed().x)});
        speed_pt.push_back({"", ptree().put("", dog->GetSpeed().y)});

        dog_json.add_child("pos", pos_pt);
        dog_json.add_child("speed", speed_pt);
        dog_json.put("dir", dog->GetDirectionChar());

        list_json.add_child(std::to_string(*dog->GetId()) + "S"s, dog_json);
    }
    main_json.add_child("players", list_json);

    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(main_json, false));
}

}  // namespace api_v1