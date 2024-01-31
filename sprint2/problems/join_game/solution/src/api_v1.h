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
    void GetMapHandler(HttpResource &&) const;
    void PostMapHandler(HttpResource &&);

    std::string GetMapListJson() const;
    std::string GetMapDescriptionJson(std::string_view) const;

    ///////////////////////
    // GAME
    ///////////////////////

    void GetGameHandler(HttpResource &&) const;
    void PostGameHandler(HttpResource &&);

    void AddNewPlayer(HttpResource &&);
    void GetPlayers(HttpResource &&) const;

    app::App &app_;
};

}  // namespace api