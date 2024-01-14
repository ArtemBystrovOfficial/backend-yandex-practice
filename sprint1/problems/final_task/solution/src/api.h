#pragma once

#include <memory>

#include "model.h"
namespace api {

// SINGLETON
class API {
   public:
    static std::shared_ptr<API> GetAPI(std::string_view version_code);

    virtual std::string getMapListJson(const model::Game &) const = 0;
    virtual std::string getMapDescriptionJson(const model::Game &,
                                              const std::string &id) const = 0;

   protected:
    API() {}
};

class API_V1 : public API {
    virtual std::string getMapListJson(const model::Game &) const override;
    virtual std::string getMapDescriptionJson(
        const model::Game &, const std::string &id) const override;
};
}  // namespace api