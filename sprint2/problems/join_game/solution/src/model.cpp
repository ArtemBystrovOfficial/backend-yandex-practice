#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

namespace lit {
const std::string id = "id"s, name = "name"s, roads = "roads"s, buildings = "buildings"s, offices = "offices"s, maps = "maps"s, x = "x"s, y = "y"s,
                  x0 = "x0"s, y0 = "y0"s, x1 = "x1"s, y1 = "y1"s, w = "w"s, h = "h"s, offsetX = "offsetX"s, offsetY = "offsetY"s;
}  //  namespace lit
void Map::LoadJsonNode(const ptree& tree) {
    *id_ = tree.get<std::string>("id");
    name_ = tree.get<std::string>(lit::name);

    ProcessChildNodes(tree, lit::roads, roads_);
    ProcessChildNodes(tree, lit::buildings, buildings_);
    ProcessChildNodes(tree, lit::offices, offices_);
}

ptree Map::GetJsonNode() const {
    ptree tree;

    tree.put(lit::id, *id_);
    tree.put(lit::name, name_);

    ptree roads;
    ProcessChildNodes(roads, roads_);

    ptree buildings;
    ProcessChildNodes(buildings, buildings_);

    ptree offices;
    ProcessChildNodes(offices, offices_);

    tree.add_child(lit::roads, roads);
    tree.add_child(lit::buildings, buildings);
    tree.add_child(lit::offices, offices);

    return tree;
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second).get();
    }
    return nullptr;
}

void Game::LoadJsonNode(const ptree& tree) {
    for (const auto& [_, map] : tree.get_child(lit::maps)) AddMap({map});
}

ptree Game::GetJsonNode() const {
    ptree tree, maps;
    // TODO Сделать макрос для автоматического заполнения массивов

    ProcessChildNodes(maps, maps_);

    tree.add_child(lit::maps, maps);
    return tree;
}

std::string Game::GetJsonMaps() const {
    using namespace std::literals;
    std::string s = "[";
    for (const auto& map : maps_) {
        s += "{"s + "\""s + lit::id + "\": \""s + *map->GetId() + "\", \""s + lit::name + "\": \""s + map->GetName() + "\""s + "},"s;
    }
    if (!maps_.empty()) s.pop_back();  // last ,
    s += "]";
    return s;
}

void Building::LoadJsonNode(const ptree& tree) {
    bounds_.position.x = tree.get<Dimension>(lit::x);
    bounds_.position.y = tree.get<Dimension>(lit::y);
    bounds_.size.width = tree.get<Dimension>(lit::w);
    bounds_.size.height = tree.get<Dimension>(lit::h);
}

ptree Building::GetJsonNode() const {
    ptree tree;
    boost::property_tree::ptree buildingNode;
    tree.put(lit::x, bounds_.position.x);
    tree.put(lit::y, bounds_.position.y);
    tree.put(lit::w, bounds_.size.width);
    tree.put(lit::h, bounds_.size.height);
    return tree;
}

void Road::LoadJsonNode(const ptree& tree) {
    start_.x = tree.get<Dimension>(lit::x0);
    start_.y = tree.get<Dimension>(lit::y0);
    auto opt = tree.get_optional<Dimension>(lit::y1);
    if (opt) {
        end_.y = *opt;
        end_.x = start_.x;
    } else {
        end_.y = start_.y;
        end_.x = tree.get<Dimension>(lit::x1);
    }
}

ptree Road::GetJsonNode() const {
    ptree tree;
    tree.put(lit::x0, start_.x);  // only for te
    tree.put(lit::y0, start_.y);
    if (IsHorizontal())
        tree.put(lit::x1, end_.x);
    else
        tree.put(lit::y1, end_.y);
    return tree;
}

void Office::LoadJsonNode(const ptree& tree) {
    *id_ = tree.get<std::string>(lit::id);
    position_.x = tree.get<Dimension>(lit::x);
    position_.y = tree.get<Dimension>(lit::y);
    offset_.dx = tree.get<Dimension>(lit::offsetX);
    offset_.dy = tree.get<Dimension>(lit::offsetY);
}

ptree Office::GetJsonNode() const {
    ptree tree;
    tree.put(lit::id, *id_);
    tree.put(lit::x, position_.x);
    tree.put(lit::y, position_.y);
    tree.put(lit::offsetX, offset_.dx);
    tree.put(lit::offsetY, offset_.dy);
    return tree;
}

}  // namespace model
