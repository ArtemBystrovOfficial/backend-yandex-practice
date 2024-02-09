#include "request_handler.h"

using namespace std::literals;
namespace beast = boost::beast;
namespace sys = boost::system;
namespace fs = std::filesystem;

namespace http_handler {

RequestHandler::RequestHandler(strand_t & api_strand, api::ApiProxyKeeper& keeper, std::string_view static_folder)
    : static_folder_(static_folder), file_system_redirection_(static_folder), api_strand_(api_strand) {
        redirection_pack_[ContentType::API_TYPE] = std::make_shared<ApiRedirection>(keeper);
    }

void RequestHandler::PreSettings(StringRequest& req) {
    req.target(util::EncodeURL(ToSV(req.target())));
    if (req.target() == "/") req.target(ToBSV(ContentType::INDEX_HTML));
}

std::shared_ptr<BasicRedirection> RequestHandler::ExtractRequestRedirection(Args_t &args) {
    auto it = redirection_pack_.end();
    if (!args.empty()) {
        it = redirection_pack_.find(args.front());
    }
    if (it != redirection_pack_.end()) {
        args.pop_front();
        return it->second;
    }
    return nullptr;
}

}  // namespace http_handler
