#include "auto_data_saver.hpp"

namespace data_serializer {

DataSaver::DataSaver(std::string_view path) : path_(path) {}

void DataSaver::Save(const model::Game &) {

}

void DataSaver::Load(model::Game &) {

}

}