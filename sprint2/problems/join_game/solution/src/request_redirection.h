#pragma once

#include <string_view>

#include "api_proxy.h"
#include "headers.h"

namespace http_handler {

// Маршрутизатор корневых запросов по облокам программы
// Является фасадом ко всем частям программы API и файловой системе

class BasicRedirection {
   public:
    virtual void RedirectReadableAccess(Args_t&& args, message_pack_t& resp) const = 0;
    virtual void RedirectWritableAccess(Args_t&& args, message_pack_t& resp) = 0;
};

class ApiRedirection : public BasicRedirection {
   public:
    ApiRedirection(api::ApiProxyKeeper& api_keeper);

    void RedirectReadableAccess(Args_t&&, message_pack_t& resp) const override;
    void RedirectWritableAccess(Args_t&&, message_pack_t& resp) override;

   private:
    template <class T>
    void Redirect(Args_t&& args, message_pack_t& resp) const {
        auto& api_resp = std::get<StringResponse>(resp);
        if (!args.empty()) {
            auto version = args.front();
            args.pop_front();

            std::shared_ptr<T> api_ptr;
            if constexpr (std::is_const_v<T>) {
                api_ptr = api_keeper_.GetConstApiByVersion(version);
                api_ptr->GetFormatData(std::move(args), api_resp);
            } else {
                api_ptr = api_keeper_.GetMutableApiByVersion(version);
                api_ptr->PostFormatData(std::move(args), api_resp);
            }
        }
    }
    api::ApiProxyKeeper& api_keeper_;
};

class FilesystemRedirection : public BasicRedirection {
   public:
    FilesystemRedirection(std::string_view static_folder);

    void RedirectReadableAccess(Args_t&& target, message_pack_t& resp) const override;
    void RedirectWritableAccess(Args_t&& target, message_pack_t& resp) override;

   private:
    std::string_view static_folder_;
};

}  // namespace http_handler