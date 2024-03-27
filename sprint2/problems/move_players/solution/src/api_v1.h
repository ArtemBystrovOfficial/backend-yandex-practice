#pragma once

#include "api.h"
#include "app.h"
#include "method_handler.h"
#include "model.h"

namespace api_v1 {

namespace net = boost::asio;
using Methods = method_handler::MethodHandler;

class Game : public Methods {
   public:
    Game(app::App &app) : Methods(app) {}

    bool GetHandler(HttpResource &&, bool is_ping = false) override;
    bool PostHandler(HttpResource &&, bool is_ping = false) override;

   private:
    void AddNewPlayer(HttpResource &&);
    void GetPlayers(HttpResource &&) const;
    void MovePlayer(HttpResource &&);

    void GetState(HttpResource &&res) const;
};

class Maps : public Methods {
   public:
    Maps(app::App &app) : Methods(app) {}

    bool GetHandler(HttpResource &&, bool is_ping = false) override;

   private:
    std::string GetMapListJson() const;
    std::string GetMapDescriptionJson(std::string_view) const;
};

class Api : public api::ApiCommon {
   public:
    Api(net::io_context &io, app::App &app);

    int GetVersionCode() override;
    void HandleApi(HttpResource &&resp) override;

   private:
    std::map<std::string, std::unique_ptr<Methods>, std::less<> > api_classes_;
};

}  // namespace api_v1