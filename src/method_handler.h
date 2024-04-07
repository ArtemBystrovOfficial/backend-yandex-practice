#pragma once

#include "app.h"
#include "error_codes.h"

namespace method_handler {

using ec = http_handler::ErrorCode;

// Cистема Ping нужна чтобы была возможность рекурсивного опроса какие методы поддерживает класс
class MethodHandler {
   public:
    MethodHandler() = delete;
    explicit MethodHandler(app::App &app) : app_(app){};
    virtual bool GetHandler(HttpResource &&, bool) { return false; };
    virtual bool PostHandler(HttpResource &&, bool) { return false; };
    virtual bool PutHandler(HttpResource &&, bool) { return false; };
    virtual bool OptionsHandler(HttpResource &&, bool) { return false; };
    virtual bool HeadHandler(HttpResource &&res, bool is_ping) {
        if (!GetHandler(std::move(res), is_ping)) return false;
        res.resp.body().clear();
        return true;
    };
    virtual bool DeleteHandler(HttpResource &&, bool) { return false; };
    virtual bool PatchHandler(HttpResource &&, bool) { return false; };

    bool RedirectAutomatic(HttpResource &&);

   protected:
    app::App &app_;
};

// Нестандартный код ошибки который может в себе содержать несколько подтипов ошибок
http_handler::ErrorCode MakeAllowError(HttpResource &&res, MethodHandler *method);

#define CALL_WITH_PING(var, func) \
    if (var)                      \
        return true;              \
    else {                        \
        func;                     \
        return true;              \
    }

}  // namespace method_handler