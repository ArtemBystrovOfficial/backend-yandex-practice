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

ApiV1::ApiV1(model::Game& game) : game_(game) {
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

std::string api::ApiV1::GetMapListJson() const { return game_.GetJsonMaps(); }

std::string api::ApiV1::GetMapDescriptionJson(std::string_view id) const {
    auto* map = game_.FindMap(model::Map::Id(std::string(id.data(), id.size())));
    if (!map) throw ec::MAP_NOT_FOUNDED;

    return map->GetJson();
}

void ApiV1::PostGameHandler(Args_t&& args, StringResponse& body) {
    if (args.size() == 1) {
        if (args.back() == "join"sv) {
            body.set(http::field::content_type, ToBSV(ContentType::JSON));
            body.set(http::field::cache_control, "no-cache");
                }
    }
    throw ec::BAD_REQUEST;
}

}  // namespace api