#include "request_handler.h"

namespace http_handler {

namespace {

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view JSON = "application/json"sv;
    constexpr static std::string_view ONLY_READ_ALLOW = "GET, HEAD"sv;
    constexpr static std::string_view INDEX_HTML = "/index.html"sv;
    static constexpr auto API_TYPE = "api"sv;
    static constexpr auto VERSION_1 = "v1"sv;
};

std::string EncodeURL(std::string_view sv) {
    std::string str = std::string(sv.data(), sv.size());
    auto it = str.begin();
    while (true) {
        it = std::find(it + 1, str.end(), '%');
        if (it == str.end()) break;
        str.replace(it, it + 3, std::string(1, char(strtol(std::string((it + 1), (it + 3)).c_str(), 0, 16))));
    }
    it = str.begin();
    while (true) {
        it = std::find(it + 1, str.end(), '+');
        if (it == str.end()) break;
        *it = ' ';
    }
    return str;
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
    std::filesystem::path path(std::string(static_folder.data(), static_folder.size()) + std::string(path_sv.data(), path_sv.size()));
    if (!std::filesystem::exists(path)) throw ErrorCodes::FILE_NOT_EXIST;

    using namespace http;

    if (!IsSubPath(path, std::filesystem::path(static_folder))) throw ErrorCodes::BAD_ACCESS;

    FileResponse res;

    res.version(11);  // HTTP/1.1
    res.result(status::ok);
    res.insert(field::content_type, GetMimeContentType(path.extension().string()));

    file_body::value_type file;

    if (sys::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec) throw ErrorCodes::READ_FILE;

    res.body() = std::move(file);
    res.prepare_payload();

    response = std::move(res);
}

void FillContentType(StringResponse& resp, api::ApiCommon::TypeData type) {
    std::string content;
    switch (type) {
        case api::ApiCommon::TypeData::HTML:
            content = ContentType::TEXT_HTML;
        case api::ApiCommon::TypeData::JSON:
            content = ContentType::JSON;
    }
    resp.set(http::field::content_type, content);
}

template <class Base, class T>
std::unique_ptr<Base> static inline MakeUnique(auto& arg, auto& arg2, auto arg3) {
    return std::unique_ptr<Base>(dynamic_cast<Base*>(new T(arg, arg2, arg3)));
}

// TODO макрос по переводу string view не работает
inline std::string_view ToSV(boost::beast::string_view bsv) { return std::string_view(bsv.data(), bsv.size()); }
inline boost::beast::string_view ToBSV(std::string_view bsv) { return boost::beast::string_view(bsv.data(), bsv.size()); }

void PreSettings(StringRequest& req) {
    req.target(EncodeURL(ToSV(req.target())));
    if (req.target() == "/") req.target(ToBSV(ContentType::INDEX_HTML));
}

std::deque<std::string_view> SplitUrl(std::string_view url) {
    std::deque<std::string_view> result;

    size_t pos = 0;
    const size_t pos_end = url.npos;
    while (true) {
        size_t space = url.find('/', pos);
        auto str = space == pos_end ? url.substr(pos) : url.substr(pos, space - pos);
        if (!str.empty()) result.push_back(std::move(str));
        if (space == pos_end)
            break;
        else
            pos = space + 1;
    }
    return result;
}

}  // namespace

message_pack_t BasicRequestTypeHandler::Handle(const StringRequest& req) {
    StringResponse response(http::status::ok, req.version());
    response.set(http::field::content_type, ToBSV(ContentType::TEXT_HTML));
    FillBody(response, "");
    response.keep_alive(req.keep_alive());
    return response;
}

message_pack_t GetRequestTypeHandler::Handle(const StringRequest& req) {
    auto resp = BasicRequestTypeHandler::Handle(req);
    redirectTarget(ToSV(req.target()), resp);
    return resp;
}

void GetRequestTypeHandler::redirectTarget(std::string_view target, message_pack_t& resp) {
    auto values = SplitUrl(target);

    auto type = values.front();

    // API
    if (type == ContentType::API_TYPE) {
        values.pop_front();
        auto& api_resp = std::get<StringResponse>(resp);
        if (!values.empty()) {
            auto version = values.front();
            values.pop_front();
            auto api_ptr = api_keeper_.GetApiByVersion(version);
            auto [data, content_type] = api_ptr->GetFormatData(std::move(values), api_resp.body());
            FillContentType(api_resp, content_type);
            FillBody(api_resp, data);
        }
        return;
    }

    // FILESYSTEM DEFAULT
    ReadFileToBuffer(resp, target, static_folder_);
    return;

    throw ErrorCodes::BAD_REQUEST;
}

message_pack_t HeadRequestTypeHandler::Handle(const StringRequest& req) {
    auto rep = GetRequestTypeHandler::Handle(req);
    if (std::holds_alternative<FileResponse>(rep)) std::get<FileResponse>(rep).body().close();
    if (std::holds_alternative<StringResponse>(rep)) std::get<StringResponse>(rep).body().clear();
    return rep;
}
message_pack_t BadRequestTypeHandler::Handle(const StringRequest& req, ErrorCodes status, std::optional<std::string_view> custom_body) {
    auto resp_var = BasicRequestTypeHandler::Handle(req);
    auto& resp = std::get<StringResponse>(resp_var);

    resp.set(http::field::content_type, ToBSV(ContentType::JSON));

    std::string_view code = "";
    std::string_view message = "";

    switch (status) {
        case ErrorCodes::BAD_REQUEST:
            resp.result(http::status::bad_request);
            code = "badRequest"sv;
            message = "badRequest"sv;  // TODO Все коды и сообщения ошибок
                                       // вынести в error_codes.h
            break;
        case ErrorCodes::MAP_NOT_FOUNDED:
            resp.result(http::status::not_found);
            code = "mapNotFound"sv;
            message = "mapNotFound"sv;
            break;
        case ErrorCodes::BAD_ACCESS:
            resp.result(http::status::not_found);
            code = "insufficient permissions"sv;
            message = "insufficient permissions"sv;
            break;
        case ErrorCodes::READ_FILE:
            resp.result(http::status::not_found);
            code = "read file error"sv;
            message = "read file error"sv;
            break;
        case ErrorCodes::UNKNOWN_ERROR:
            resp.result(http::status::not_found);
            code = "unknown";
            if (custom_body) message = *custom_body;
            break;
        case ErrorCodes::FILE_NOT_EXIST:
            resp.result(http::status::not_found);
            resp.set(http::field::content_type, "text/plain");
            FillBody(resp, "file not exist");
            return resp_var;
    }

    ptree tree;
    tree.put("code", code);
    tree.put("message", message);

    FillBody(resp, json_loader::JsonObject::GetJson(tree));
    return resp_var;
}
RequestHandler::RequestHandler(model::Game& game, api::ApiProxyKeeper& keeper, std::string_view static_folder)
    : static_folder_(static_folder),
      game_{game},
      bad_request_(MakeUnique<BadRequestTypeHandler, BadRequestTypeHandler>(game, keeper, static_folder)) {
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, HeadRequestTypeHandler>(game, keeper, static_folder));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, GetRequestTypeHandler>(game, keeper, static_folder));
    handlers_variants_.push_back(MakeUnique<BasicRequestTypeHandler, PostRequestTypeHandler>(game, keeper, static_folder));
}
message_pack_t RequestHandler::HandleRequest(StringRequest&& req) {
    PreSettings(req);

    req.target(EncodeURL(ToSV(req.target())));
    auto handler = std::find_if(handlers_variants_.begin(), handlers_variants_.end(),
                                [&req](const auto& handler) { return handler->GetMethodString() == ToSV(req.method_string()); });
    if (handler == handlers_variants_.end()) return bad_request_->Handle(req, ErrorCodes::BAD_REQUEST);

    try {
        auto resp = (*handler)->Handle(req);
        return resp;
    } catch (const ErrorCodes& ec) {
        auto resp = bad_request_->Handle(req, ec);
        return resp;
    } catch (const std::exception& ec) {
        auto resp = bad_request_->Handle(req, ErrorCodes::UNKNOWN_ERROR, ec.what());
        return resp;
    }
}
}  // namespace http_handler
