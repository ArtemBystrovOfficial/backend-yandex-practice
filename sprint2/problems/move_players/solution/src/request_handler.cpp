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

BasicRequestTypeHandler::BasicRequestTypeHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder)
    : api_keeper_(keeper), static_folder_(static_folder), default_redirection_(std::make_shared<FilesystemRedirection>(static_folder)) {
    redirection_pack_[ContentType::API_TYPE] = std::make_shared<ApiRedirection>(keeper);
}

message_pack_t BasicRequestTypeHandler::Handle(const StringRequest& req) {
    auto resp = util::GetBasicResponse(req);

    auto args = util::SplitUrl(ToSV(req.target()));
    auto redirection = ExtractRequestRedirection(args);
    redirection->Redirect(std::move(args), resp, req);

    return resp;
}

message_pack_t BadRequestTypeHandler::Handle(const StringRequest& req, ErrorCode status, std::optional<std::string_view> custom_body) {
    auto resp_var = util::GetBasicResponse(req);
    auto& resp = std::get<StringResponse>(resp_var);

    FillInfoError(resp, status, custom_body);

    return resp_var;
}

RequestHandler::RequestHandler(api::ApiProxyKeeper& keeper, std::string_view static_folder)
    : static_folder_(static_folder), bad_request_(keeper, static_folder), basic_request_(keeper, static_folder) {}

message_pack_t RequestHandler::HandleRequest(StringRequest&& req) {
    PreSettings(req);
    try {
        auto resp = basic_request_.Handle(req);
        return resp;
    } catch (const ErrorCode& ec) {
        auto resp = bad_request_.Handle(req, ec);
        // TODO STACK TRACE IN ASYNC CODE
        return resp;
    } catch (const std::exception& ec) {
        auto resp = bad_request_.Handle(req, ErrorCode::UNKNOWN_ERROR, ec.what());
        return resp;
    }
}

void RequestHandler::PreSettings(StringRequest& req) {
    req.target(util::EncodeURL(ToSV(req.target())));
    if (req.target() == "/") req.target(ToBSV(ContentType::INDEX_HTML));
}

}  // namespace http_handler
