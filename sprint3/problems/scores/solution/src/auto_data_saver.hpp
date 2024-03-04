#pragma once
#include <string_view>
#include "model.h"

namespace data_serializer {

class DataSaver {
    public:
        DataSaver(std::string_view path);

        void Save(const model::Game &);
        void Load(model::Game &);

    private:
        std::string_view path_;
};

}