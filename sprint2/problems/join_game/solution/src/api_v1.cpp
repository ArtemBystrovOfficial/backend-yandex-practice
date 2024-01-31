#include "api_v1.h"

#include "common.h"
#include "error_codes.h"

namespace {
namespace http = boost::beast::http;
using namespace std::literals;
namespace ph = std::placeholders;
using ec = http_handler::ErrorCode;
}  // namespace

namespace api {

ApiV1::ApiV1(app::App& app) : app_(app) {
    // Нужна помощь как разграничить const от обычнынх обработчиков void() и void()const для post_handlers_ и get_handlers_
    get_handlers_.insert({"maps"sv, std::bind(&ApiV1::GetMapHandler, this, ph::_1)});
    get_handlers_.insert({"game"sv, std::bind(&ApiV1::GetGameHandler, this, ph::_1)});
    // et_handlers_.insert({"maps"sv, std::bind(&ApiV1::PostMapHandler, this, ph::_1, ph::_2)});

    post_handlers_.insert({"game"sv, std::bind(&ApiV1::PostGameHandler, this, ph::_1)});
}

int ApiV1::GetVersionCode() { return 0x1; }

void ApiV1::GetMapHandler(HttpResource&& res) const {
    res.resp.set(http::field::content_type, ToBSV(ContentType::JSON));

    if (res.args.empty()) {
        return common_pack::FillBody(res.resp, GetMapListJson());
    } else if (res.args.size() == 1) {
        return common_pack::FillBody(res.resp, GetMapDescriptionJson(res.args.back()));
    }

    throw ec::GET_NOT_ALLOWED;
}

void ApiV1::PostMapHandler(HttpResource&& res) {
    if (res.args.empty())  // Version Category
        throw ec::POST_NOT_ALLOWED;
}

std::string api::ApiV1::GetMapListJson() const { return app_.GetGame().GetJsonMaps(); }

std::string api::ApiV1::GetMapDescriptionJson(std::string_view id) const {
    auto map = app_.GetGame().FindMap(model::Map::Id(std::string(id.data(), id.size())));
    if (!map) throw ec::MAP_NOT_FOUNDED;
    return map->GetJson();
}

void ApiV1::GetGameHandler(HttpResource&& res) const {
    if (res.args.size() == 1) {
        auto arg = res.args.front();
        res.args.pop_front();
        if (arg == "players"sv) return GetPlayers(std::move(res));
    }

    throw ec::GET_NOT_ALLOWED;
}

void ApiV1::PostGameHandler(HttpResource&& res) {
    if (res.args.size() == 1) {
        auto arg = res.args.front();
        res.args.pop_front();
        if (arg == "join"sv) return AddNewPlayer(std::move(res));
    }
    throw ec::POST_NOT_ALLOWED;
}

void ApiV1::AddNewPlayer(HttpResource&& res) {
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

void ApiV1::GetPlayers(HttpResource&& res) const {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    std::string_view token;

    try {
        auto token = ToSV(res.req[http::field::authorization]);

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
    common_pack::FillBody(res.resp, json_loader::JsonObject::GetJson(list_json, false));
}

}  // namespace api