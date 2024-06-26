#pragma once

///////////////////////////////////////////////////
//// Все пользовательские типы данных и строки
///////////////////////////////////////////////////

#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <deque>
#include <string_view>
#include <variant>

using namespace std::literals;

struct ContentType {
    ContentType() = delete;
    static constexpr auto TEXT_HTML = "text/html"sv;
    static constexpr auto JSON = "application/json"sv;
    static constexpr auto ONLY_READ_ALLOW = "GET, HEAD"sv;
    static constexpr auto INDEX_HTML = "/index.html"sv;
    static constexpr auto API_TYPE = "api"sv;
    static constexpr auto VERSION_1 = "v1"sv;
};

using Args_t = std::deque<std::string_view>;

using StringRequest = boost::beast::http::request<boost::beast::http::string_body>;
using StringResponse = boost::beast::http::response<boost::beast::http::string_body>;
using FileRequest = boost::beast::http::request<boost::beast::http::file_body>;
using FileResponse = boost::beast::http::response<boost::beast::http::file_body>;
using message_pack_t = std::variant<FileResponse, StringResponse>;

using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;

// Совмещает в себе ответ аргументы и запрос, для экономии места
struct HttpResource {
    HttpResource(HttpResource&&) = default;
    HttpResource& operator=(HttpResource&&) = default;
    HttpResource(const HttpResource&) = default;
    HttpResource& operator=(HttpResource&) = default;

    HttpResource(const StringRequest& request, StringResponse& response, Args_t&& arguments)
        : req(request), resp(response), args(std::move(arguments)) {}
    HttpResource() = delete;

    const StringRequest& req;
    StringResponse& resp;
    Args_t args;
};