#include "model.h"

#include <boost/geometry/geometries/point.hpp>
#include <stdexcept>

#include "error_codes.h"

namespace model {
using namespace std::literals;

namespace lit {
const std::string id = "id"s, name = "name"s, roads = "roads"s, buildings = "buildings"s, offices = "offices"s, maps = "maps"s, x = "x"s, y = "y"s,
                  x0 = "x0"s, y0 = "y0"s, x1 = "x1"s, y1 = "y1"s, w = "w"s, h = "h"s, offsetX = "offsetX"s, offsetY = "offsetY"s,
                  dog_speed_default = "defaultDogSpeed", dog_speed = "dogSpeed";
}  //  namespace lit
void Map::LoadJsonNode(const ptree& tree) {
    *id_ = tree.get<std::string>("id");
    name_ = tree.get<std::string>(lit::name);

    try {
        dog_custom_speed_ = tree.get<Real>(lit::dog_speed);
    } catch (...) {
        dog_custom_speed_ = std::nullopt;
    }

    ProcessChildNodes(tree, lit::roads, roads_);
    ProcessChildNodes(tree, lit::buildings, buildings_);
    ProcessChildNodes(tree, lit::offices, offices_);

    boxes_list_.reset(new std::vector<gl::Box>(GetRectsByRoads()));
}

ptree Map::GetJsonNode() const {
    ptree tree;

    tree.put(lit::id, *id_);
    tree.put(lit::name, name_);
    // if (dog_custom_speed_) tree.put(lit::dog_speed, *dog_custom_speed_); В тестах это не поддерживается

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

PointF Map::GetMovePositionWithCollisions(const PointF& from, const PointF& to) {
    if (!boxes_list_) throw "rtree not implemented!";

    Real kof_direction = 0.005;

    // Начальная и конечная точки луча
    geometry_local::Line ray_local{{from.x, from.y}, {to.x, to.y}};

    geometry_local::ListPoints list_points;

    for (const auto& bx : *boxes_list_) {
        bx.FillIntersects(list_points, ray_local);
    }

    geometry_local::Direction direction_sort;

    if (from.x != to.x) {
        if (from.x > to.x)
            direction_sort = geometry_local::Direction::Left;
        else
            direction_sort = geometry_local::Direction::Right;
    } else {
        if (from.y > to.y)
            direction_sort = geometry_local::Direction::Down;
        else
            direction_sort = geometry_local::Direction::Up;
    }

    geometry_local::SortLinePoints(list_points, direction_sort);

    auto CheckOutBounds = [&](const PointF& point) -> bool {
        bool has_out_bound = true;
        for (const auto& bx : *boxes_list_)
            if (bx.CheckContains(point)) has_out_bound = false;

        return has_out_bound;
    };

    for (auto point : list_points) {
        bool is = false;
        if (from.x != to.x) {
            if (from.x > to.x)
                is = CheckOutBounds({point.x - kof_direction, point.y});
            else
                is = CheckOutBounds({point.x + kof_direction, point.y});
        } else {
            if (from.y > to.y)
                is = CheckOutBounds({point.x, point.y - kof_direction});
            else
                is = CheckOutBounds({point.x, point.y + kof_direction});
        }
        if (is) {
            // OutBound
            if (from.x != to.x) return PointF{point.x, from.y};
            if (from.y != to.y) return PointF{from.x, point.y};
        }
    }

    // Дорога не прирывна
    return to;
}

std::vector<gl::Box> Map::GetRectsByRoads() {
    std::vector<gl::Box> vec;
    for (auto& road : roads_) {
        auto point_start = road.GetStart();
        auto point_end = road.GetEnd();

        auto kof = Road::WidthRoad;
        Real x1 = point_start.x, x2 = point_end.x, y1 = point_start.y, y2 = point_end.y;

        if (road.IsHorizontal()) {
            //--->
            if (point_start.x < point_end.x) {
                x1 -= kof;
                y1 -= kof;
                x2 += kof;
                y2 += kof;
            } else {  //<---
                x1 += kof;
                y1 += kof;
                x2 -= kof;
                y2 -= kof;
            }
        } else {
            // | ^
            // v |
            if (point_start.y < point_end.y) {
                x1 -= kof;
                y1 -= kof;
                x2 += kof;
                y2 += kof;
            } else {
                x1 += kof;
                y1 += kof;
                x2 -= kof;
                y2 -= kof;
            }
        }

        vec.push_back({{std::min(x1,x2), std::min(y1,y2)}, {std::max(x1, x2), std::max(y1,y2)}});
    }
    return vec;
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::make_shared<Map>(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second);
    }
    return nullptr;
}

void Game::LoadJsonNode(const ptree& tree) {
    try {
        dog_speed_default_ = tree.get<Real>(lit::dog_speed_default);
    } catch (...) {
        dog_speed_default_ = 1.0f;
    }
    for (const auto& [_, map] : tree.get_child(lit::maps)) AddMap({map});
}

ptree Game::GetJsonNode() const {
    ptree tree, maps;
    tree.put("defaultDogSpeed", dog_speed_default_);

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

std::shared_ptr<GameSession> Game::AddSession(Map::Id id) {
    auto map = FindMap(id);
    if (map) {
        auto session = std::make_shared<GameSession>(std::move(map), time_manager_, dog_speed_default_);
        sessions_.push_back(session);
        return session;
    }
    return nullptr;
}

bool Game::IsSessionStarted(Map::Id id) { return GetSession(id) != nullptr; }

std::shared_ptr<GameSession> Game::GetSession(Map::Id id) {
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [id](auto& session) { return session->GetMap()->GetId() == id; });
    return it != sessions_.end() ? *it : nullptr;
}

void Game::TickFullGame(const std::chrono::milliseconds& ms) { time_manager_.GlobalTick(ms); }

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

GameSession::GameSession(std::shared_ptr<Map> map, TimeManager& time_manager, Real default_speed)
    : map_(std::move(map)), _last_dog_id(0), default_speed_(default_speed), time_manager_(time_manager) {}

std::shared_ptr<Dog> GameSession::AddDog(std::string_view dog_name) {
    Real map_speed;

    if (map_->HasCustomSpeed())
        map_speed = map_->CustomSpeed();
    else
        map_speed = default_speed_;
    auto ptr = dogs_.emplace_back(std::make_shared<Dog>(
        Dog::Id(_last_dog_id++), dog_name, PointF{/*(rand() % 10000) / 100.0f, (rand() % 10000) / 100.0f*/ 0.0f, 0.0f}, map_speed, map_));

    time_manager_.AddSubscribers(ptr);
    return ptr;
}

std::shared_ptr<Dog> GameSession::FindDogByID(Dog::Id id) {
    // TODO сделать HashMap как и для карт
    auto it = std::find_if(dogs_.begin(), dogs_.end(), [id](auto& dog1) { return dog1->GetId() == id; });
    return it == dogs_.end() ? nullptr : *it;
}

std::shared_ptr<Map> GameSession::GetMap() { return map_; }

void Dog::SetId(const Id& id) { id_ = id; }

const Dog::Id& Dog::GetId() { return id_; }

const std::string& Dog::GetName() { return name_; }

bool Dog::MoveDog(Direction direction) {
    switch (direction) {
        case Direction::NORTH:
            speed_ = {0.0f, -map_speed_};
            break;
        case Direction::WEST:
            speed_ = {map_speed_, 0.0f};
            break;
        case Direction::EAST:
            speed_ = {-map_speed_, 0.0f};
            break;
        case Direction::SOUTH:
            speed_ = {0.0f, map_speed_};
            break;
        default:
            return false;
    }
    direction_ = direction;
    return true;
}

void Dog::StopDog() { speed_ = {0.0f, 0.0f}; }

void Dog::Tick(const std::chrono::milliseconds& ms) {
    if (!speed_.x && !speed_.y) return;
    Real new_x = position_.x + (speed_.x * ms.count() / 1000.0f), new_y = position_.y + (speed_.y * ms.count() / 1000.0f);

    PointF position_target{new_x, new_y};

    auto position = current_map_->GetMovePositionWithCollisions(position_, {new_x, new_y});
    if (position != position_target) StopDog();
    position_ = position;
}

}  // namespace model
