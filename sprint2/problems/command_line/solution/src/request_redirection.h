#pragma once

#include <string_view>

#include "api_proxy.h"
#include "headers.h"

namespace http_handler {

// Маршрутизатор корневых запросов по облокам программы
// Является фасадом ко всем частям программы API и файловой системе

class BasicRedirection {
   public:
    virtual void Redirect(Args_t&& args, message_pack_t& resp, const StringRequest& req) = 0;
};

class ApiRedirection : public BasicRedirection {
   public:
    explicit ApiRedirection(api::ApiProxyKeeper& api_keeper);

    void Redirect(Args_t&&, message_pack_t& resp, const StringRequest& req) override;

   private:
    api::ApiProxyKeeper& api_keeper_;
};

class FilesystemRedirection : public BasicRedirection {
   public:
    explicit FilesystemRedirection(std::string_view static_folder);

    void Redirect(Args_t&&, message_pack_t& resp, const StringRequest& req) override;

   private:
    std::string_view static_folder_;
};

}  // namespace http_handler