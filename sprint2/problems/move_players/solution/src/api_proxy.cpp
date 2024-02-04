#include "api_proxy.h"

#include "error_codes.h"

namespace {
using ec = http_handler::ErrorCode;
}  // namespace

namespace api {

ApiProxyKeeper::ApiProxyKeeper(net::io_context &io, app::App &app) {
    // V1
    auto v1 = std::make_shared<api_v1::Api>(io, app);
    apis_.insert({v1->GetVersionCode(), std::move(v1)});
}

std::shared_ptr<ApiCommon> ApiProxyKeeper::GetMutableApiByVersion(std::string_view version_code) const {
    if (version_code.size() != 2) throw ec::BAD_REQUEST;
    if (version_code.front() != 'v' || version_code.back() < '0' || version_code.back() > '9')  // v1 пока диапазон допускает
                                                                                                // лишь 1-9
        throw ec::BAD_REQUEST;

    auto api_impl = getApiByCode(version_code.back() - '0');
    if (!api_impl) throw ec::BAD_REQUEST;
    return api_impl;
}

std::shared_ptr<const ApiCommon> ApiProxyKeeper::GetConstApiByVersion(std::string_view version_code) const {
    return GetMutableApiByVersion(version_code);
}

std::shared_ptr<ApiCommon> ApiProxyKeeper::getApiByCode(version_code_t code) const noexcept {
    auto it = apis_.find(code);
    return (it == apis_.end() ? nullptr : it->second);
}

}  // namespace api