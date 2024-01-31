#pragma once

#include "api.h"
#include "app.h"
#include "model.h"

namespace api {

class ApiV1 : public ApiCommon {
   public:
    ApiV1(app::App &app);

    int GetVersionCode() override;

   private:
    // Пока api небольшое все методы в один класс, добавить обработчики классы для всех типов запросов

    ///////////////////////
    // MAPS
    ///////////////////////
    void GetMapHandler(Args_t &&, StringResponse &) const;
    void PostMapHandler(Args_t &&, StringResponse &);

    std::string GetMapListJson() const;
    std::string GetMapDescriptionJson(std::string_view) const;

    ///////////////////////
    // GAME
    ///////////////////////

    void GetGameHandler(Args_t &&, StringResponse &) const {};
    void PostGameHandler(Args_t &&, StringResponse &);

    void AddNewPlayer(Args_t &&, StringResponse &);

    std::map<std::string_view, api_handler_t> get_json_handlers_;
    app::App &app_;
};

}  // namespace api