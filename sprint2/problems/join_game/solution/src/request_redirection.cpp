#include "request_redirection.h"

#include "common.h"

namespace {
namespace http = boost::beast::http;
}

namespace http_handler {
ApiRedirection::ApiRedirection(api::ApiProxyKeeper& api_keeper) : api_keeper_(api_keeper) {}

void ApiRedirection::RedirectReadableAccess(Args_t&& args, message_pack_t& resp, const StringRequest& req) const {
    Redirect<const api::ApiCommon>(std::move(args), resp, req);
}
void ApiRedirection::RedirectWritableAccess(Args_t&& args, message_pack_t& resp, const StringRequest& req) {
    Redirect<api::ApiCommon>(std::move(args), resp, req);
}

FilesystemRedirection::FilesystemRedirection(std::string_view static_folder) : static_folder_(static_folder) {}

void FilesystemRedirection::RedirectReadableAccess(Args_t&& args, message_pack_t& resp, const StringRequest& req) const {
    auto path = common_pack::GetUrlByArgs(args);
    common_pack::ReadFileToBuffer(resp, path, static_folder_);
}
void FilesystemRedirection::RedirectWritableAccess(Args_t&& args, message_pack_t& resp, const StringRequest& req) {}

}  // namespace http_handler
