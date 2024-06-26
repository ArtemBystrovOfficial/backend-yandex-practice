#include "common.h"

#include <boost/system.hpp>
#include <map>

#include "error_codes.h"

namespace {
namespace http = boost::beast::http;
}

namespace util {

message_pack_t GetBasicResponse(const StringRequest& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, util::ToBSV(ContentType::TEXT_HTML));
    util::FillBody(response, "");
    response.keep_alive(req.keep_alive());
    message_pack_t resp = response;
    return response;
}

std::string EncodeURL(std::string_view sv) {
    std::string str = std::string(sv.data(), sv.size());
    auto it = str.begin();
    while (true) {
        it = std::find(it + 1, str.end(), '%');
        if (it == str.end()) 
            break;
        str.replace(it, it + 3, std::string(1, char(strtol(std::string((it + 1), (it + 3)).c_str(), 0, 16))));
    }
    it = str.begin();
    while (true) {
        it = std::find(it + 1, str.end(), '+');
        if (it == str.end()) 
            break;
        *it = ' ';
    }
    return str;
}

Args_t SplitUrl(std::string_view url) {
    std::deque<std::string_view> result;

    size_t pos = 0;
    const size_t pos_end = url.npos;
    while (true) {
        size_t space = url.find('/', pos);
        auto str = space == pos_end ? url.substr(pos) : url.substr(pos, space - pos);
        if (!str.empty()) 
            result.push_back(std::move(str));
        if (space == pos_end)
            break;
        else
            pos = space + 1;
    }
    return result;
}

std::string GetUrlByArgs(const Args_t& args) {
    std::string s = "";
    for (auto& arg : args) {
        s += '/';
        s += arg;
    }
    return s;
}

bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string GetMimeContentType(std::string_view file_extension) {
    static const std::map<std::string_view, std::string> mimeTypes = {{".htm", "text/html"},       {".html", "text/html"},
                                                                      {".css", "text/css"},        {".txt", "text/plain"},
                                                                      {".js", "text/javascript"},  {".json", "application/json"},
                                                                      {".xml", "application/xml"}, {".png", "image/png"},
                                                                      {".jpg", "image/jpeg"},      {".jpe", "image/jpeg"},
                                                                      {".jpeg", "image/jpeg"},     {".gif", "image/gif"},
                                                                      {".bmp", "image/bmp"},       {".ico", "image/vnd.microsoft.icon"},
                                                                      {".tiff", "image/tiff"},     {".tif", "image/tiff"},
                                                                      {".svg", "image/svg+xml"},   {".svgz", "image/svg+xml"},
                                                                      {".mp3", "audio/mpeg"}};

    auto it = mimeTypes.find(file_extension);
    if (it != mimeTypes.end()) {
        return it->second;
    } else {
        return "application/octet-stream";
    }
}

void FillBody(StringResponse& resp, std::string_view text) {
    resp.body() = text;
    resp.content_length(text.size());
}

void ReadFileToBuffer(message_pack_t& response, std::string_view path_sv, std::string_view static_folder) {
    using namespace http_handler;
    using namespace boost::beast::http;
    namespace beast = boost::beast;
    namespace sys = boost::system;

    std::filesystem::path path(std::string(static_folder.data(), static_folder.size()) + std::string(path_sv.data(), path_sv.size()));
    if (!std::filesystem::exists(path)) 
        throw ErrorCode::FILE_NOT_EXIST;

    if (!IsSubPath(path, std::filesystem::path(static_folder))) 
        throw ErrorCode::BAD_ACCESS;

    FileResponse res;

    res.version(11);  // HTTP/1.1
    res.result(status::ok);
    res.insert(field::content_type, GetMimeContentType(path.extension().string()));

    file_body::value_type file;

    if (sys::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec)  
        throw ErrorCode::READ_FILE;

    res.body() = std::move(file);
    res.prepare_payload();

    if (std::holds_alternative<FileResponse>(response)) 
        std::get<FileResponse>(response).body().close();

    response = std::move(res);
}

std::string ExecuteAuthorized(HttpResource& res) noexcept(false) {
    std::string_view token_raw;
    try {
        token_raw = util::ToSV(res.req[http::field::authorization]);
    } catch (...) {
        throw http_handler::ErrorCode::AUTHORIZATION_NOT_EXIST;
    }
    return std::string(token_raw.data(), token_raw.size());
}

std::string_view ExtractArg(Args_t& args) {
    if (args.empty()) 
        return ""sv;
    auto arg = args.front();
    args.pop_front();
    return arg;
}

std::map<std::string, std::string> GetPropertiesFromUrl(const std::string& url) {
    std::map<std::string, std::string> properties;

    std::size_t pos = url.find('?');
    if (pos != std::string::npos) {

        std::string queryString = url.substr(pos + 1);

        std::size_t startPos = 0;
        std::size_t endPos;
        while ((endPos = queryString.find('&', startPos)) != std::string::npos) {

            std::string property = queryString.substr(startPos, endPos - startPos);
            std::size_t equalPos = property.find('=');
            if (equalPos != std::string::npos) {
                std::string key = property.substr(0, equalPos);
                std::string value = property.substr(equalPos + 1);
                properties[key] = value;
            }
            startPos = endPos + 1;
        }

        std::string lastProperty = queryString.substr(startPos);
        std::size_t equalPos = lastProperty.find('=');
        if (equalPos != std::string::npos) {
            std::string key = lastProperty.substr(0, equalPos);
            std::string value = lastProperty.substr(equalPos + 1);
            properties[key] = value;
        }
    }

    return properties;
}

}  // namespace util
