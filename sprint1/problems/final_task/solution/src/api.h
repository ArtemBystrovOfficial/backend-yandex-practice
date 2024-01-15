#pragma once

#include <map>
#include <memory>

#include "model.h"
namespace api {

using version_code_t = size_t;
using Args_t = std::deque<std::string_view>;
// SINGLETON

class ApiCommon {
   public:
    enum class TypeData { JSON, HTML, PLAIN };
    using api_handler_t = std::function<std::pair<std::string, TypeData>(Args_t &&, std::string_view)>;

    ApiCommon() = default;

    std::pair<std::string, TypeData> GetFormatData(Args_t &&, std::string_view) const;
    // GetFormatData
    virtual int GetVersionCode() = 0;

   protected:
    std::map<std::string_view, api_handler_t> get_handlers_;
};

class ApiV1 : public ApiCommon {
   public:
    ApiV1(model::Game &game);

    int GetVersionCode() override;

   private:
    // MAPS
    std::pair<std::string, TypeData> MapHandler(Args_t &&, std::string_view) const;
    std::string GetMapListJson() const;
    std::string GetMapDescriptionJson(std::string_view) const;

    std::map<std::string_view, std::function<std::string(Args_t &&, std::string_view)> > get_json_handlers_;
    model::Game &game_;
};

// Хранит все версии API и подгружает если версия API понадобилась
// Возможен сброс кэша по таймауту если API давно не использовалось
// TODO Пока что сразу все загружает, по мере расширения добавить
class ApiProxyKeeper {
   public:
    ApiProxyKeeper(model::Game &game);

    std::shared_ptr<ApiCommon> GetApiByVersion(std::string_view);
    std::shared_ptr<ApiCommon> getApiByCode(version_code_t code) noexcept;

   private:
    std::map<version_code_t, std::shared_ptr<ApiCommon> > apis_;
};

}  // namespace api