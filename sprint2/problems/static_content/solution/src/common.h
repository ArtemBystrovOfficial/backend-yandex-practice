#pragma once

#include <deque>
#include <filesystem>
#include <string_view>
#include <variant>

#include "headers.h"

////////////////////////////////////
//// Базовые функции работы с HTTP
////////////////////////////////////

namespace common_pack {

namespace fs = std::filesystem;

std::string EncodeURL(std::string_view sv);

std::deque<std::string_view> SplitUrl(std::string_view url);

bool IsSubPath(fs::path path, fs::path base);

std::string GetMimeContentType(std::string_view file_extension);

void FillBody(StringResponse& resp, std::string_view text);

void ReadFileToBuffer(message_pack_t& response, std::string_view path_sv, std::string_view static_folder);

}  // namespace common_pack