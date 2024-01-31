#include "api_v1.h"

#include "common.h"
#include "error_codes.h"

namespace {
namespace http = boost::beast::http;
using namespace std::literals;
namespace ph = std::placeholders;
using ec = http_handler::ErrorCodes;
}  // namespace

namespace api {

ApiV1::ApiV1(app::App& app) : app_(app) {
    // Нужна помощь как разграничить const от обычнынх обработчиков void() и void()const для post_handlers_ и get_handlers_
    get_handlers_.insert({"maps"sv, std::bind(&ApiV1::GetMapHandler, this, ph::_1, ph::_2)});
    // et_handlers_.insert({"maps"sv, std::bind(&ApiV1::PostMapHandler, this, ph::_1, ph::_2)});

    post_handlers_.insert({"game"sv, std::bind(&ApiV1::PostGameHandler, this, ph::_1, ph::_2)});
}

int ApiV1::GetVersionCode() { return 0x1; }

void ApiV1::GetMapHandler(Args_t&& args, StringResponse& resp) const {
    resp.set(http::field::content_type, ToBSV(ContentType::JSON));

    if (args.empty()) {
        common_pack::FillBody(resp, GetMapListJson());
    } else if (args.size() == 1) {
        common_pack::FillBody(resp, GetMapDescriptionJson(args.back()));
    } else
        throw ec::BAD_REQUEST;
}

void ApiV1::PostMapHandler(Args_t&& args, StringResponse& body) {
    if (args.empty())  // Version Category
        throw ec::BAD_REQUEST;
}

std::string api::ApiV1::GetMapListJson() const { return app_.GetGame().GetJsonMaps(); }

std::string api::ApiV1::GetMapDescriptionJson(std::string_view id) const {
    auto map = app_.GetGame().FindMap(model::Map::Id(std::string(id.data(), id.size())));
    if (!map) throw ec::MAP_NOT_FOUNDED;

    return map->GetJson();
}

void ApiV1::PostGameHandler(Args_t&& args, StringResponse& resp) {
    if (args.size() == 1) {
        auto arg = args.front();
        args.pop_front();
        if (arg == "join"sv) {
            AddNewPlayer(std::move(args), resp);
        }
    } else
        throw ec::BAD_REQUEST;
}

void ApiV1::AddNewPlayer(Args_t&& args, StringResponse& resp) {
    resp.set(http::field::content_type, ToBSV(ContentType::JSON));
    resp.set(http::field::cache_control, "no-cache");

    auto tree = json_loader::JsonObject::GetTree(resp.body());
    std::string name, map_id;
    try {
        name = tree.get<std::string>("userName");
        map_id = tree.get<std::string>("mapId");
    } catch (...) {
        throw ec::JOIN_PLAYER;
    }
    auto [player, token] = app_.GetMutablePlayers().AddPlayer(std::make_shared<model::Dog>(model::Dog::Id(name)), model::Map::Id(map_id));

    if (!player) throw ec::JOIN_PLAYER;

    auto json = json_loader::CreateTrivialJson({"authToken", "playerId"}, *token, *player->dog_->GetId());

    common_pack::FillBody(resp, json_loader::JsonObject::GetJson(json, false));
}

}  // namespace api