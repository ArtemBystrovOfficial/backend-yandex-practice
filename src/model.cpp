#include "model.h"

#include <boost/geometry/geometries/point.hpp>
#include <stdexcept>

#include "error_codes.h"
#include "logger.h"
#include <ratio>

namespace model {
using namespace std::literals;

namespace lit {
const std::string id = "id"s;
const std::string name = "name"s; 
const std::string roads = "roads"s; 
const std::string buildings = "buildings"s;
const std::string offices = "offices"s; 
const std::string maps = "maps"s; 
const std::string x = "x"s; 
const std::string y = "y"s;
const std::string x0 = "x0"s; 
const std::string y0 = "y0"s; 
const std::string x1 = "x1"s; 
const std::string y1 = "y1"s; 
const std::string w = "w"s; 
const std::string h = "h"s; 
const std::string offsetX = "offsetX"s; 
const std::string offsetY = "offsetY"s;
const std::string dog_speed_default = "defaultDogSpeed"; 
const std::string dog_speed = "dogSpeed"; 
const std::string period = "period"; 
const std::string probability = "probability"; 
const std::string loot_type = "lootType";
const std::string loot_generator_config="lootGeneratorConfig";
const std::string default_bag_capacity =  "defaultBagCapacity"; 
const std::string bag_capacity = "bagCapacity"; 
const std::string value = "value";
const std::string dog_retirement_time = "dogRetirementTime";
}
int Map::GetScoreByLoot(int loot_type) const { 
    return special_information_loots_[loot_type].get<int>(lit::value); 
}
//  namespace lit
void Map::LoadJsonNode(const ptree& tree) {
    *id_ = tree.get<std::string>("id");
    name_ = tree.get<std::string>(lit::name);

    try {
        dog_custom_speed_ = tree.get<Real>(lit::dog_speed);
    } catch (...) {
        dog_custom_speed_ = std::nullopt;
    }

    try {
        bag_custom_capacity_ = tree.get<int>(lit::bag_capacity);
    } catch (...) {
        bag_custom_capacity_ = std::nullopt;
    }

    ProcessChildNodes(tree, lit::roads, roads_);
    ProcessChildNodes(tree, lit::buildings, buildings_);
    ProcessChildNodes(tree, lit::offices, offices_);

    for (const auto& [_, node] : tree.get_child(lit::loot_type +"s")) 
        special_information_loots_.push_back(node);
    
    boxes_list_ = std::make_shared<std::vector<gl::Box>>(GetRectsByRoads());
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

    ptree loots;
    for (auto& loot : special_information_loots_) 
        loots.push_back({"", loot});

    tree.add_child(lit::loot_type + "s", loots);
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

PointF Map::GetRandomCordinates() {
    auto road = roads_[rand() % roads_.size()]; //random road
    auto start = road.GetStart();
    auto end = road.GetEnd();
    return PointF(std::min(start.x,end.x) + rand() % (abs(start.x - end.x) + 1), std::min(start.y,end.y) + rand() % (abs(start.y - end.y) + 1));
}

PointF Map::GetMovePositionWithCollisions(const PointF &from, const PointF &to)
{
    if (!boxes_list_) 
        throw "rtree not implemented!";

    Real inaccuracy = 0.005;

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
            if (bx.CheckContains(point)) 
                has_out_bound = false;

        return has_out_bound;
    };

    for (auto point : list_points) {
        bool is = false;
        if (from.x != to.x) {
            if (from.x > to.x)
                is = CheckOutBounds({point.x - inaccuracy, point.y});
            else
                is = CheckOutBounds({point.x + inaccuracy, point.y});
        } else {
            if (from.y > to.y)
                is = CheckOutBounds({point.x, point.y - inaccuracy});
            else
                is = CheckOutBounds({point.x, point.y + inaccuracy});
        }
        if (is) {
            // OutOfBound
            if (from.x != to.x) 
                return PointF{point.x, from.y};
            if (from.y != to.y) 
                return PointF{from.x, point.y};
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

        auto width_road = Road::WidthRoad;
        Real x1 = point_start.x, x2 = point_end.x, y1 = point_start.y, y2 = point_end.y;

        if (road.IsHorizontal()) {
            //--->
            if (point_start.x < point_end.x) {
                x1 -= width_road;
                y1 -= width_road;
                x2 += width_road;
                y2 += width_road;
            } else {  //<---
                x1 += width_road;
                y1 += width_road;
                x2 -= width_road;
                y2 -= width_road;
            }
        } else {
            // | ^
            // v |
            if (point_start.y < point_end.y) {
                x1 -= width_road;
                y1 -= width_road;
                x2 += width_road;
                y2 += width_road;
            } else {
                x1 += width_road;
                y1 += width_road;
                x2 -= width_road;
                y2 -= width_road;
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
        dog_speed_default_ = 1.0;
    }

    try {
        default_bag_capacity_ = tree.get<int>(lit::default_bag_capacity);
    } catch (...) {
        default_bag_capacity_ = 3;
    }

    try {
        dog_retirement_time_ = tree.get<float>(lit::dog_retirement_time);
    } catch (...) {
        dog_retirement_time_ = 60.0; //1min
    }

    auto gen_conf = tree.get_child(lit::loot_generator_config);

    auto period = gen_conf.get<Real>(lit::period);
    auto probability = gen_conf.get<Real>(lit::probability);

    loot_generator_ = std::make_unique<loot_gen::LootGenerator>(std::chrono::milliseconds(int(period*1000)), probability);

    for (const auto& [_, map] : tree.get_child(lit::maps)) AddMap(Map(map));
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
    if (!maps_.empty()) 
        s.pop_back();  // last ,
    s += "]";
    return s;
}

std::shared_ptr<GameSession> Game::AddSession(Map::Id id) {
    auto map = FindMap(id);
    if (map) {
        auto session = std::make_shared<GameSession>(std::move(map), time_manager_, dog_speed_default_,default_bag_capacity_, is_game_randomize_start_cordinate_,dog_retirement_time_, loot_generator_);
        session->request_to_save_retired_player_s.connect([this](std::string a1, int a2, int a3) { 
            request_to_save_retired_player_s(std::move(a1),a2,a3); 
        });
        time_manager_.AddSubscribers(session, 10);
        sessions_.push_back(session);
        return session;
    }
    return nullptr;
}

void Game::AddSession(std::shared_ptr<GameSession> game_session) {
    sessions_.push_back(game_session);
    time_manager_.AddSubscribers(game_session, 10);
}

bool Game::IsSessionStarted(Map::Id id) { return GetSession(id) != nullptr; }

std::shared_ptr<GameSession> Game::GetSession(Map::Id id) {
    auto it = std::find_if(sessions_.begin(), sessions_.end(), [id](auto& session) { return session->GetMap()->GetId() == id; });
    return it != sessions_.end() ? *it : nullptr;
}

void Game::TickFullGame(const std::chrono::milliseconds& ms) { time_manager_.GlobalTick(ms); }

void Game::SetRandomizeStart(bool is) {
    is_game_randomize_start_cordinate_ = is;
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

GameSession::GameSession(std::shared_ptr<Map> map,
                         TimeManager& time_manager,
                         Real default_speed,
                         int default_bag_capacity,
                         bool is_game_randomize_start_cordinate,
                         int dog_retirement_time,
                         std::shared_ptr<loot_gen::LootGenerator> gen)
    : map_(std::move(map)),
     _last_dog_id(0),
     default_speed_(default_speed),
     time_manager_(time_manager), 
     default_bag_capacity_(default_bag_capacity),
     dog_retirement_time_(dog_retirement_time), 
     is_game_randomize_start_cordinate_(is_game_randomize_start_cordinate),
     loot_generator_(gen) {}

std::shared_ptr<Dog> GameSession::AddDog(std::string_view dog_name) {
    Real map_speed;

    if (map_->HasCustomSpeed())
        map_speed = map_->CustomSpeed();
    else
        map_speed = default_speed_;

    int bag_capacity;
    if (map_->HasCustomBagCapacity())
        bag_capacity = map_->CustomBagCapacity();
    else
        bag_capacity = default_bag_capacity_;

    auto ptr = dogs_.emplace_back(std::make_shared<Dog>(
        Dog::Id(_last_dog_id++), 
        dog_name, 
        is_game_randomize_start_cordinate_ ? map_->GetRandomCordinates() : PointF{0.0, 0.0},
        map_speed,
        map_,
        Bag{{},bag_capacity},
        dog_retirement_time_
        ));

    ptr->request_to_save_retired_player_s.connect([this, ptr](std::string a1, int a2, int a3){ 
        auto elem = std::find(dogs_.begin(), dogs_.end(), ptr); //TODO HASH MAP 
        if(elem == dogs_.end())
            assert(false);
        (*elem)->SetIsExited(true);
        dogs_.erase(elem);
        request_to_save_retired_player_s(std::move(a1),a2,a3); 
    });
    time_manager_.AddSubscribers(ptr, 20);
    return ptr;
}

void GameSession::AddDog(std::shared_ptr<Dog> dog) {
    dogs_.push_back(dog);
    time_manager_.AddSubscribers(dog,20);
}

std::shared_ptr<Dog> GameSession::FindDogByID(Dog::Id id) {
    // TODO сделать HashMap как и для карт
    auto it = std::find_if(dogs_.begin(), dogs_.end(), [id](auto& dog1) { return dog1->GetId() == id; });
    return it == dogs_.end() ? nullptr : *it;
}

std::shared_ptr<Map> GameSession::GetMap() { return map_; }

const Map* GameSession::GetMap() const { return map_.get(); }

void GameSession::SetLootObjects(const LootObjects& loots) {
    for(const auto & loot: loots) 
        loot_objects_.push_back(loot);
    last_id_object_ = 0;
}

void GameSession::setMap(std::shared_ptr<Map> map) {
    map_ = map;
    for(auto & dog : dogs_) {
        dog->SetMap(map);
    }
    for(auto & loot : loot_objects_) {
        loot->SetMap(map);
    }
}

void GameSession::Tick(const std::chrono::milliseconds& ms) {
    //Генерация нового лута
    auto count_to_generate = loot_generator_->Generate(ms,loot_objects_.size(),dogs_.size());
    for(int i=0;i<count_to_generate;i++) 
        loot_objects_.push_back(std::make_shared<LootObject>(map_,last_id_object_++));
    
    //Сбор лута и складирование на базу
    CollisionManager manager(*this);
    auto events = collision_detector::FindGatherEvents(manager);

    int offset_loots = manager.GetOffsetLoots();
    for(const auto & event : events) {
        
        if(event.item_id < offset_loots){ //Offices
            PutLootsToOffice(event.gatherer_id);
        } else { //Loots
            int id_loot = event.item_id - offset_loots;
            TakeLoot(event.gatherer_id,id_loot);
        }
    }
}

bool GameSession::TakeLoot(int id_dog, int id_loot) {
    if(id_loot >= loot_objects_.size())
        return false;

    if(id_dog >= dogs_.size())
        return false;

    auto & bag = dogs_[id_dog]->GetMutableBag();

    if(bag.items.size() >= bag.max_count) {
        return false;
    }

    bag.items.push_back({loot_objects_[id_loot]->GetId(), loot_objects_[id_loot]->GetType()});
    loot_objects_.erase(loot_objects_.begin()+id_loot);

    return true;
}
 
void GameSession::PutLootsToOffice(int id_dog) {
    if(id_dog >= dogs_.size())
        throw std::invalid_argument("id_dog ");
    auto & bag = dogs_[id_dog]->GetMutableBag();
    int score = 0; 

    for(const auto [_,loot_type]: bag.items) {
        score += map_->GetScoreByLoot(loot_type);
    }
    bag.items.clear();
    dogs_[id_dog]->AddScorePoints(score);
}

void Dog::SetId(const Id& id) { id_ = id; }

bool Dog::MoveDog(Direction direction) {
    switch (direction) {
        case Direction::NORTH:
            speed_ = {0.0, -map_speed_};
            break;
        case Direction::WEST:
            speed_ = {map_speed_, 0.0};
            break;
        case Direction::EAST:
            speed_ = {-map_speed_, 0.0};
            break;
        case Direction::SOUTH:
            speed_ = {0.0, map_speed_};
            break;
        default:
            return false;
    }
    direction_ = direction;
    return true;
}

void Dog::StopDog() { 
    speed_ = {0.0, 0.0}; 
    BOOST_LOG_TRIVIAL(debug) << "DOG STOPPED!";
    exited_time_ = current_absolute_time_;
}

void Dog::Tick(const std::chrono::milliseconds& ms) {
    current_absolute_time_ += ms;

    if(IsStopped() && exited_time_ && !is_exited_) {
        auto time_after_stopped = std::chrono::duration_cast<std::chrono::milliseconds>(current_absolute_time_ - *exited_time_).count() / 1000.0;
        BOOST_LOG_TRIVIAL(debug) << time_after_stopped << " " << dog_retirement_time_;
        if(time_after_stopped  >= dog_retirement_time_) {
            auto deleted_time = std::chrono::steady_clock::now();
            auto ms_after = std::chrono::duration_cast<std::chrono::milliseconds>(current_absolute_time_ -
             std::chrono::steady_clock::time_point(std::chrono::milliseconds(0))).count();
            request_to_save_retired_player_s(name_, score_, ms_after);
        }
    }

    if (!speed_.x && !speed_.y) 
        return;
    static constexpr float millisecond_in_second = 1000.0;

    Real new_x = position_.x + (speed_.x * ms.count() / millisecond_in_second), 
         new_y = position_.y + (speed_.y * ms.count() / millisecond_in_second);

    PointF position_target{new_x, new_y};

    auto position = current_map_->GetMovePositionWithCollisions(position_, {new_x, new_y});
    if (position != position_target) 
        StopDog();
    position_before_ = position_;
    position_ = position;
}

}  // namespace model
