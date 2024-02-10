#pragma once
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <optional>

#include "api.h"
#include "error_codes.h"
#include "headers.h"
#include "request_redirection.h"
#include "common.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace net = boost::asio;
using RedirectionPack = std::map<std::string_view, std::shared_ptr<BasicRedirection>>;

template <class Base, class T, class... Args>
std::unique_ptr<Base> static inline MakeUnique(Args&&... args) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(std::forward<Args>(args)...)));
}

class RequestHandler {
   public:
    explicit RequestHandler(strand_t & strand, api::ApiProxyKeeper& keeper, std::string_view static_folder = "");

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        //send(HandleRequest(std::forward<decltype(req)>(req)));

        PreSettings(req);
        auto args = util::SplitUrl(util::ToSV(req.target()));
        auto resp_var = util::GetBasicResponse(req);
        //Здесь должен был полифорфизм переадресации корневого сегмента,
        //но strand немного поменял правила, позже вернуть 
        auto redirection = ExtractRequestRedirection(args);
        if(redirection) {
            net::dispatch(api_strand_, [args = std::move(args), redirection, send = std::move(send), req = std::forward<decltype(req)>(req),
                                         resp = std::forward<decltype(resp_var)>(resp_var)]() mutable {
                try {
                    redirection->Redirect(std::move(args), resp, req);
                } catch (const ErrorCode& ec) {
                    // TODO STACK TRACE IN ASYNC CODE
                    FillInfoError(std::get<StringResponse>(resp), ec);
                } catch (const std::exception& ec) {
                    FillInfoError(std::get<StringResponse>(resp), ErrorCode::UNKNOWN_ERROR, ec.what());
                }
                send(resp);     
            });
        } else {
            try {
                file_system_redirection_.Redirect(std::move(args), resp_var, req);
            } catch (const ErrorCode& ec) {
                FillInfoError(std::get<StringResponse>(resp_var), ec);
            } catch (const std::exception& ec) {
                FillInfoError(std::get<StringResponse>(resp_var), ErrorCode::UNKNOWN_ERROR, ec.what());
            }
            send(resp_var);
        }
    }

   private:
    void PreSettings(StringRequest& req);

    strand_t & api_strand_;

    std::shared_ptr<BasicRedirection> ExtractRequestRedirection(Args_t& args);
    FilesystemRedirection file_system_redirection_;
    RedirectionPack redirection_pack_;
    std::string_view static_folder_;
};

}  // namespace http_handler
