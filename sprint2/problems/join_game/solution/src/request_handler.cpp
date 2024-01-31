#include "request_handler.h"

#include "common.h"

using namespace std::literals;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace http_handler {

std::shared_ptr<BasicRedirection> BasicRequestTypeHandler::ExtractRequestRedirection(Args_t& args) {
    auto it = redirection_pack_.end();
    if (!args.empty()) {
        it = redirection_pack_.find(args.front());
    }
    if (it != redirection_pack_.end()) {
        args.pop_front();
        return it->second;
    }
    return default_redirection_;
}

message_pack_t BasicRequestTypeHandler::Handle(const StringRequest& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, ToBSV(ContentType::TEXT_HTML));
    common_pack::FillBody(response, "");
    response.keep_alive(req.keep_alive());
    return response;
}

message_pack_t GetRequestTypeHandler::Handle(const StringRequest& req) {
    auto resp = BasicRequestTypeHandler::Handle(req);
    auto args = common_pack::SplitUrl(ToSV(req.target()));
    auto redirection = ExtractRequestRedirection(args);
    redirection->RedirectReadableAccess(std::move(args), resp, req);  // ONLY GET
    return resp;
}

message_pack_t HeadRequestTypeHandler::Handle(const StringRequest& req) {
    auto rep = GetRequestTypeHandler::Handle(req);
    if (std::holds_alternative<FileResponse>(rep)) std::get<FileResponse>(rep).body().close();
    if (std::holds_alternative<StringResponse>(rep)) std::get<StringResponse>(rep).body().clear();
    return rep;
}

message_pack_t PostRequestTypeHandler::Handle(const StringRequest& req) {
    auto resp = BasicRequestTypeHandler::Handle(req);
    auto args = common_pack::SplitUrl(ToSV(req.target()));
    auto redirection = ExtractRequestRedirection(args);
    redirection->RedirectWritableAccess(std::move(args), resp, req);  // ONLY GET
    return resp;
}

message_pack_t BadRequestTypeHandler::Handle(const StringRequest& req, ErrorCode status, std::optional<std::string_view> custom_body) {
    auto resp_var = BasicRequestTypeHandler::Handle(req);
    auto& resp = std::get<StringResponse>(resp_var);

    FillInfoError(resp, status, custom_body);

    return resp_var;
}

RequestHandler::RequestHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder)
    : static_folder_(static_folder),
      bad_request_(MakeUnique<BadRequestTypeHandler, BadRequestTypeHandler>(keeper, static_folder, handlers_redirection_)) {
    handlers_redirection_[ContentType::API_TYPE] = std::make_shared<ApiRedirection>(keeper);

    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, HeadRequestTypeHandler>(keeper, static_folder, handlers_redirection_));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, GetRequestTypeHandler>(keeper, static_folder, handlers_redirection_));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, PostRequestTypeHandler>(keeper, static_folder, handlers_redirection_));
}

message_pack_t RequestHandler::HandleRequest(StringRequest&& req) {
    PreSettings(req);
    auto handler = std::find_if(handlers_variants_.begin(), handlers_variants_.end(),
                                [&req](const auto& handler) { return handler->GetMethodString() == ToSV(req.method_string()); });
    if (handler == handlers_variants_.end()) return bad_request_->Handle(req, ErrorCode::BAD_REQUEST);

    try {
        auto resp = (*handler)->Handle(req);
        return resp;
    } catch (const ErrorCode& ec) {
        auto resp = bad_request_->Handle(req, ec);
        // TODO STACK TRACE IN ASYNC CODE
        return resp;
    } catch (const std::exception& ec) {
        auto resp = bad_request_->Handle(req, ErrorCode::UNKNOWN_ERROR, ec.what());
        return resp;
    }
}

void RequestHandler::PreSettings(StringRequest& req) {
    req.target(common_pack::EncodeURL(ToSV(req.target())));
    if (req.target() == "/") req.target(ToBSV(ContentType::INDEX_HTML));
}

}  // namespace http_handler
