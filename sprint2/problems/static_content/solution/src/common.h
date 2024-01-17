#pragma once

#include <boost/beast/http.hpp>
#include <filesystem>
#include <variant>

// TODO REFACTOR
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using HttpResponse = http::response<http::string_body>;
using FileRequest = http::request<http::file_body>;
using FileResponse = http::response<http::file_body>;
namespace fs = std::filesystem;

using message_pack_t = std::variant<FileResponse, HttpResponse>;