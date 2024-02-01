#pragma once

#include <filesystem>
#include <string_view>
#include <variant>

#include "headers.h"

////////////////////////////////////
//// Базовые функции работы с HTTP
////////////////////////////////////

inline std::string_view ToSV(boost::beast::string_view bsv) { return std::string_view(bsv.data(), bsv.size()); }
inline boost::beast::string_view ToBSV(std::string_view bsv) { return boost::beast::string_view(bsv.data(), bsv.size()); }

namespace common_pack {

namespace fs = std::filesystem;

message_pack_t GetBasicResponse(const StringRequest& req);

std::string EncodeURL(std::string_view sv);

Args_t SplitUrl(std::string_view url);
std::string GetUrlByArgs(const Args_t& args);

bool IsSubPath(fs::path path, fs::path base);

std::string GetMimeContentType(std::string_view file_extension);

void FillBody(StringResponse& resp, std::string_view text);

void ReadFileToBuffer(message_pack_t& response, std::string_view path_sv, std::string_view static_folder);

}  // namespace common_pack