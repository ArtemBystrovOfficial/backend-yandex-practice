#include "request_redirection.h"

#include "common.h"


namespace http_handler {

namespace {
namespace http = boost::beast::http;
}

ApiRedirection::ApiRedirection(api::ApiProxyKeeper& api_keeper) : api_keeper_(api_keeper) {}

void ApiRedirection::Redirect(Args_t&& args, message_pack_t& resp, const StringRequest& req) {
    auto& api_resp = std::get<StringResponse>(resp);
    auto version = util::ExtractArg(args);
    auto api_ptr = api_keeper_.GetMutableApiByVersion(version);
    api_ptr->HandleApi(HttpResource(req, api_resp, std::move(args)));
}

FilesystemRedirection::FilesystemRedirection(std::string_view static_folder) : static_folder_(static_folder) {}

void FilesystemRedirection::Redirect(Args_t&& args, message_pack_t& resp, const StringRequest& req) {
    auto path = util::GetUrlByArgs(args);
    util::ReadFileToBuffer(resp, path, static_folder_);
}

}  // namespace http_handler
