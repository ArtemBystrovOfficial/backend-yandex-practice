#include "api_v1.h"

#include <boost/asio/strand.hpp>

#include "common.h"
#include "error_codes.h"
#include "logger.h"

namespace {
namespace http = boost::beast::http;
using namespace std::literals;
namespace ph = std::placeholders;
using ec = http_handler::ErrorCode;
}  // namespace

namespace api_v1 {

Api::Api(strand_t& strand, app::App& app) : ApiCommon(strand) {
    api_classes_["maps"] = std::make_unique<Maps>(app);
    api_classes_["game"] = std::make_unique<Game>(app);
}

int Api::GetVersionCode() { return 0x1; }

void Api::HandleApi(HttpResource&& res) {
    if (res.args.empty()) throw ec::BAD_REQUEST;
    auto val = util::ExtractArg(res.args);

    auto it = api_classes_.find(val);
    if (it == api_classes_.end()) throw ec::BAD_REQUEST;

    auto& redirect = it;

    HttpResource copy = res;
    if (!redirect->second->RedirectAutomatic(std::move(res))) throw method_handler::MakeAllowError(std::move(copy), redirect->second.get());
}

//////// GAME ///////////

bool Game::GetHandler(HttpResource&& res, bool is_ping) {
    if (res.args.size() == 1) {
        auto arg = util::ExtractArg(res.args);
        if (arg == "players"sv) CALL_WITH_PING(is_ping, GetPlayers(std::move(res)))
        if (arg == "state"sv) CALL_WITH_PING(is_ping, GetState(std::move(res)))
    }
    return false;
}

bool Game::PostHandler(HttpResource&& res, bool is_ping) {
    if (res.args.size() == 1) {
        auto arg = util::ExtractArg(res.args);
        if (arg == "join"sv) CALL_WITH_PING(is_ping, AddNewPlayer(std::move(res)));
        if (arg == "tick"sv) CALL_WITH_PING(is_ping, Tick(std::move(res)));
    } else if (res.args.size() == 2) {
        auto arg = util::ExtractArg(res.args);
        if (arg == "player"sv) {
            auto arg = util::ExtractArg(res.args);
            if (arg == "action") CALL_WITH_PING(is_ping, MovePlayer(std::move(res)))
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

    util::FillBody(res.resp, json_loader::JsonObject::GetJson(json, false));
}

void Game::GetPlayers(HttpResource&& res) const {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    auto token_raw = util::ExecuteAuthorized(res);

    auto dogs = app_.GetPlayers().GetListDogInRoom(util::CreateTokenByAuthorizationString(token_raw));
    ptree list_json;
    for (const auto& dog : dogs) {
        ptree dog_json;
        dog_json.put("name", dog->GetName());
        list_json.add_child(std::to_string(*dog->GetId()), dog_json);
    }
    util::FillBody(res.resp, json_loader::JsonObject::GetJson(list_json, false, false));
}

void Game::MovePlayer(HttpResource&& res) {
    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    // TODO обработку по типу запрашиваемого контента пока по умолчанию общение только по json

    auto token_raw = util::ExecuteAuthorized(res);

    std::string_view move;

    try {
        auto tree = json_loader::JsonObject::GetTree(res.req.body());
        move = tree.get<std::string>("move");
    } catch (...) {
        throw ec::BAD_REQUEST;
    }

    app_.GetPlayers().MovePlayer(util::CreateTokenByAuthorizationString(token_raw), move);

    util::FillBody(res.resp, "{}"sv);
}

void Game::Tick(HttpResource&& res) {
    if(!app_.GetTickEditAccess())
        throw ec::BAD_TICK_ACCESS;

    res.resp.set(http::field::content_type, res.req[http::field::content_type]);
    res.resp.set(http::field::cache_control, "no-cache");

    int time_delta;
    try {
        auto tree = json_loader::JsonObject::GetTree(res.req.body());
        time_delta = atoi(tree.get<std::string>("timeDelta").c_str());
    } catch (...) {
        throw ec::BAD_REQUEST_TICK;
    }

    if(time_delta <= 0)
        throw ec::BAD_REQUEST_TICK;

    app_.GetMutableGame().TickFullGame(std::chrono::milliseconds(time_delta));

    util::FillBody(res.resp, "{}"sv);
}

//////// MAPS ///////////

bool Maps::GetHandler(HttpResource&& res, bool is_ping) {
    res.resp.set(http::field::content_type, ToBSV(ContentType::JSON));

    if (res.args.empty()) {
        CALL_WITH_PING(is_ping, util::FillBody(res.resp, GetMapListJson()))
    } else if (res.args.size() == 1) {
        CALL_WITH_PING(is_ping, util::FillBody(res.resp, GetMapDescriptionJson(res.args.back())));
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

    auto token_raw = util::ExecuteAuthorized(res);
    auto dogs = app_.GetPlayers().GetListDogInRoom(util::CreateTokenByAuthorizationString(token_raw));

    //Из-за проблем с Json и точностью на тестах пока что про число с плавающей точкой обрабатываются данным образом
    //В дальнешем найти алтеранативу или совсем убрать округление кординат когда тесты пройдут
    auto ROUND_VALUE_FOR_TEST = [](double value) -> double {
        return std::round(value * std::pow(10, 6)) / std::pow(10, 6);
    };

    auto CreateRoundedNode = [&](boost::property_tree::ptree & node, auto & value) -> boost::property_tree::ptree  {

        if (fmod(value, 1.0) != 0.0) {
            node.push_back({"", boost::property_tree::ptree().put("", ROUND_VALUE_FOR_TEST(value))});
        } else {
            node.push_back({"", boost::property_tree::ptree().put("", std::to_string(int(value)) + "f")});
        }

        return node;
    };


    ptree main_json;  // TODO продумать как кастомные json файлы можно применить к системе модели
    ptree list_json;
    for (const auto& dog : dogs) {
        ptree dog_json;

        boost::property_tree::ptree pos_pt;
        CreateRoundedNode(pos_pt, dog->GetPosition().x);
        CreateRoundedNode(pos_pt, dog->GetPosition().y);

        boost::property_tree::ptree speed_pt;
        CreateRoundedNode(speed_pt, dog->GetSpeed().x);
        CreateRoundedNode(speed_pt, dog->GetSpeed().y);

        dog_json.add_child("pos", pos_pt);
        dog_json.add_child("speed", speed_pt);
        dog_json.put("dir", dog->GetDirectionChar());

        list_json.add_child(std::to_string(*dog->GetId()) + "S"s, dog_json);
    }
    main_json.add_child("players", list_json);

    util::FillBody(res.resp, json_loader::JsonObject::GetJson(main_json, false));
}

}  // namespace api_v1