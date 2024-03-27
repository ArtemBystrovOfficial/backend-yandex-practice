#include "method_handler.h"

#include "common.h"
#include "error_codes.h"

bool method_handler::MethodHandler::RedirectAutomatic(HttpResource&& res) {
    auto sv = util::ToSV(res.req.method_string());
    if (sv == "GET"sv) return GetHandler(std::move(res), false);
    if (sv == "POST"sv) return PostHandler(std::move(res), false);
    if (sv == "DELETE"sv) return DeleteHandler(std::move(res), false);
    if (sv == "PATCH"sv) return PatchHandler(std::move(res), false);
    if (sv == "PUT"sv) return PutHandler(std::move(res), false);
    if (sv == "OPTIONS"sv) return OptionsHandler(std::move(res), false);
    if (sv == "HEAD"sv) return HeadHandler(std::move(res), false);
    throw http_handler::ErrorCode::BAD_REQUEST;
}

http_handler::ErrorCode method_handler::MakeAllowError(HttpResource&& res, MethodHandler* method) {
    HttpResource copy = res;
    auto er = ec::NOT_ALLOWED;
    ec error = ec::NOT_ALLOWED;

    if (method->GetHandler(std::move(copy), true)) 
        error |= ec::GET_ALLOWED;

    HttpResource copy1 = res;  // Не хочу делать на каждую функцию универсальную ссылку
    if (method->PostHandler(std::move(copy1), true)) 
        error |= ec::POST_NOT_ALLOWED;

    HttpResource copy2 = res;
    if (method->PutHandler(std::move(copy2), true)) 
        error |= ec::PUT_ALLOWED;

    HttpResource copy3 = res;
    if (method->OptionsHandler(std::move(copy3), true)) 
        error |= ec::OPTIONS_ALLOWED;

    HttpResource copy4 = res;
    if (method->HeadHandler(std::move(copy4), true)) 
        error |= ec::HEAD_ALLOWED;

    HttpResource copy5 = res;
    if (method->DeleteHandler(std::move(copy5), true)) 
        error |= ec::DELETE_ALLOWED;

    HttpResource copy6 = res;
    if (method->PatchHandler(std::move(copy6), true)) 
        error |= ec::PATCH_ALLOWED;

    return error;
}
