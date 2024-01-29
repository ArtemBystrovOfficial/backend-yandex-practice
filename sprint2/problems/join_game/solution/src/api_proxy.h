#pragma once

#include "api_v1.h"

// Хранит все версии API и подгружает если версия API понадобилась
// Возможен сброс кэша по таймауту если API давно не использовалось
// TODO Пока что сразу все загружает, по мере расширения добавить
namespace api {

class ApiProxyKeeper {
   public:
    ApiProxyKeeper(model::Game &game);

    std::shared_ptr<const ApiCommon> GetConstApiByVersion(std::string_view) const;
    std::shared_ptr<ApiCommon> GetMutableApiByVersion(std::string_view) const;

   private:
    std::shared_ptr<ApiCommon> getApiByCode(version_code_t code) const noexcept;

    std::map<version_code_t, std::shared_ptr<ApiCommon> > apis_;
};

}  // namespace api