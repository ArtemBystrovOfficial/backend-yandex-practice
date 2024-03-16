#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "geometry.h"
#include "json_loader.h"
#include "tagged.h"
#include "time.h"
#include "loot_generator.h"
#include "collision_detector.h"
#include <boost/signals2/signal.hpp>

namespace model {

using Dimension = int;
using Coord = Dimension;

namespace gl = geometry_local;

struct Point {
    Coord x, y;
};

struct SpeedF {
    Real x, y;
};

enum class Direction : char { NORTH = 'U', WEST = 'R', SOUTH = 'D', EAST = 'L' };

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road : public json_loader::JsonObject {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

   public:
    static constexpr Real WidthRoad = 0.4;

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    explicit Road(const ptree& tree) { LoadJsonNode(tree); }
    Road(HorizontalTag, Point start, Coord end_x) noexcept : start_{start}, end_{end_x, start.y} {}
    Road(VerticalTag, Point start, Coord end_y) noexcept : start_{start}, end_{start.x, end_y} {}

    bool IsHorizontal() const noexcept { return start_.y == end_.y; }
    bool IsVertical() const noexcept { return start_.x == end_.x; }

    Point GetStart() const noexcept { return start_; }
    Point GetEnd() const noexcept { return end_; }

   private:
    Point start_;
    Point end_;
};

class Building : public json_loader::JsonObject {
   public:
    explicit Building(Rectangle bounds) noexcept : bounds_{bounds} {}

    explicit Building(const ptree& tree) { LoadJsonNode(tree); }

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

    const Rectangle& GetBounds() const noexcept { return bounds_; }

   private:
    Rectangle bounds_;
};

class Office : public json_loader::JsonObject {
   public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept : id_{std::move(id)}, position_{position}, offset_{offset} {}

    explicit Office(const ptree& tree) : id_("") { LoadJsonNode(tree); }

    const Id& GetId() const noexcept { return id_; }

    Point GetPosition() const noexcept { return position_; }
    Offset GetOffset() const noexcept { return offset_; }

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

   private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map : public json_loader::JsonObject {
   public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

    explicit Map(const ptree& tree) : id_("") { LoadJsonNode(tree); }

    const Id& GetId() const { return id_; }
    const std::string& GetName() const  { return name_; }

    const Buildings& GetBuildings() const { return buildings_; }
    const Roads& GetRoads() const { return roads_; }
    const Offices& GetOffices() const { return offices_; }
    const ptree & GetSpecialLootInformation(int index) { return special_information_loots_.at(index); }
    size_t GetSizeObjectLoots() const { return special_information_loots_.size(); } 

    int GetScoreByLoot(int loot_type) const;

    void AddRoad(const Road& road) { roads_.emplace_back(road); }
    void AddBuilding(const Building& building) { buildings_.emplace_back(building); }

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

    void AddOffice(Office office);

    bool HasCustomSpeed() { return dog_custom_speed_.has_value(); }
    Real CustomSpeed() noexcept(false) { return *dog_custom_speed_; }

    bool HasCustomBagCapacity() { return bag_custom_capacity_.has_value(); }
    int CustomBagCapacity() noexcept(false) { return *bag_custom_capacity_; }

    PointF GetRandomCordinates();
    PointF GetMovePositionWithCollisions(const PointF&, const PointF&);

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    std::vector<gl::Box> GetRectsByRoads();

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    std::optional<Real> dog_custom_speed_;
    std::optional<int> bag_custom_capacity_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::vector<ptree> special_information_loots_;

    std::shared_ptr<std::vector<geometry_local::Box>> boxes_list_;
};

struct Bag {
    //id, type
    std::vector<std::pair<int,int>> items;
    int max_count;
};

class Dog : public TimeObject {
   public:

    using Id = util::Tagged<size_t, Dog>;

    Dog() = default;

    Dog(Id id, std::string_view name, PointF position, Real map_speed, std::shared_ptr<Map> current_map, Bag bag, int dog_retirement_time)
        : id_(id),
          name_(name.data(), name.size()),
          position_(position),
          speed_({0.0, 0.0}),
          direction_(Direction::NORTH),
          map_speed_(map_speed),
          current_map_(current_map), 
          bag_(bag),
          score_(0),
          dog_retirement_time_(dog_retirement_time) {
            position_before_ = position;
            entered_time_ = std::chrono::steady_clock::now();
            StopDog();
          };

    bool IsValid() { return !name_.empty(); }

    void SetId(const Id&);
    void AddScorePoints(size_t points) { score_ += points; }

    const Id& GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const PointF& GetPosition() const { return position_; }
    const PointF& GetPositionBefore() const { return position_before_; }
    const Direction& GetDirection() const { return direction_; }
    const SpeedF& GetSpeed() const { return speed_; }
    const Real& GetMapSpeed() const { return map_speed_; }
    std::shared_ptr<Map> GetMap() const { return current_map_; }
    Bag & GetMutableBag() { return bag_; }
    const Bag & GetBag() const { return bag_; }
    size_t GetScore() const { return score_; }
    bool IsExited() const { return is_exited_; }
    bool IsStopped() const { return (speed_.x == 0.0 && speed_.y == 0.0); }

    void SetName(const std::string& new_name) { name_ = new_name;}
    void SetPosition(const PointF& new_position) { position_ = new_position; position_before_ = position_;}
    void SetDirection(const Direction& new_direction) { direction_ = new_direction; }
    void SetSpeed(const SpeedF& new_speed) { speed_ = new_speed; }
    void SetMapSpeed(const Real& map_speed) { map_speed_ = map_speed; }
    void SetMap(std::shared_ptr<Map> new_map) { current_map_ = new_map; }
    void SetBag(const Bag& new_bag) { bag_ = new_bag; }
    void SetScore(size_t new_score) { score_ = new_score;}
    void SetIsExited(bool is_exited) { is_exited_ = is_exited; }

    bool MoveDog(Direction);
    void StopDog();

    char GetDirectionChar() { return static_cast<char>(direction_); }

    // TIME SUPPORT
    void Tick(const std::chrono::milliseconds& ms) override;

   public:
    //name, score, play_time_ms
    boost::signals2::signal<void(std::string, int, int)> request_to_save_retired_player_s;

   private:
    Id id_ = Id(0);
    std::string name_;
    std::shared_ptr<Map> current_map_;

    size_t score_;
    Bag bag_;

    PointF position_;
    PointF position_before_;
    SpeedF speed_;
    Real map_speed_;
    Direction direction_;

    //Exit System
    Real dog_retirement_time_;
    bool is_exited_{false};
    std::chrono::steady_clock::time_point entered_time_;
    std::optional<std::chrono::steady_clock::time_point> exited_time_;
};

class LootObject {
public:
    LootObject() = default;

    LootObject(std::shared_ptr<Map> current_map, int id)
        : current_map_(current_map)
        , id_(id) {
        position_ = current_map_->GetRandomCordinates();
        type_ = rand() % current_map_->GetSizeObjectLoots();
    }

    int GetType() const { return type_; }
    int GetId() const { return id_; }
    const PointF& GetPosition() const { return position_; }

    void SetType(int new_type) { type_ = new_type; }
    void SetId(int new_id) { id_ = new_id; }
    void SetPosition(const PointF& new_position) { position_ = new_position; }

    void SetMap(std::shared_ptr<Map> map) { current_map_ = map; }

   private:
    std::shared_ptr<Map> current_map_;
    PointF position_;
    int type_;
    int id_;
};

class GameSession : public TimeObject {
   public:
    GameSession() = default;

    using Dogs = std::vector<std::shared_ptr<Dog>>;
    using LootObjects = std::vector<std::shared_ptr<LootObject>>;

    GameSession(TimeManager& time_manager, std::shared_ptr<loot_gen::LootGenerator> generator) 
        : time_manager_(time_manager),
          loot_generator_(generator){} 

    GameSession(std::shared_ptr<Map> map,
                TimeManager& time_manager,
                Real default_speed,
                int default_bag_capacity,
                bool is_game_randomize_start_cordinate,
                int dog_retirement_time, 
                std::shared_ptr<loot_gen::LootGenerator>);

    std::shared_ptr<Dog> AddDog(std::string_view dog_name);
    void AddDog(std::shared_ptr<Dog> dog);
    std::shared_ptr<Dog> FindDogByID(Dog::Id);

    std::shared_ptr<Map> GetMap();
    const Map * GetMap() const;
    const Dogs& GetDogs() const { return dogs_; }
    const LootObjects& GetLootObjects() const { return loot_objects_; }
    void SetLootObjects(const LootObjects & loots);
    int GetLastDogId() const { return _last_dog_id;}
    Real GetDefaultSpeed() const { return default_speed_;}
    int GetBagCapacity() const { return default_bag_capacity_;}
    int GetLastIdObject() const { return last_id_object_; }
    bool GetIsGameRandomizeStartCoords() const { return is_game_randomize_start_cordinate_;}

    void setMap(std::shared_ptr<Map> map);
    void SetLastDogId(int new_last_dog_id) {_last_dog_id = new_last_dog_id;}
    void SetDefaultSpeed(Real new_default_speed) {default_speed_ = new_default_speed;}
    void SetBagCapacity(int new_default_bag_capacity) {default_bag_capacity_ = new_default_bag_capacity;}
    void SetLastIdObject(int new_last_id_object) {last_id_object_ = new_last_id_object;}
    void SetGameRandomizeStartCoords(bool new_is_game_randomize_start_coords) {is_game_randomize_start_cordinate_ = new_is_game_randomize_start_coords;}

    void Tick(const std::chrono::milliseconds& ms) override;

    // Сделаем систему создания комнат или автоматическое распределение по картам, но сейчас одна сессия одна карта
    size_t GetCountDogs() { return dogs_.size(); }
   public:
    boost::signals2::signal<void(std::string, int, int)> request_to_save_retired_player_s;

   private:

    //if if bag full return false
    bool TakeLoot(int id_dog, int id_loot);
    //return score
    void PutLootsToOffice(int id_dog);

    int _last_dog_id;
    Real default_speed_;
    int default_bag_capacity_;
    Real dog_retirement_time_;
    std::shared_ptr<Map> map_;
    Dogs dogs_;
    LootObjects loot_objects_;

    std::shared_ptr<loot_gen::LootGenerator> loot_generator_;
    TimeManager& time_manager_;

    int last_id_object_;

    bool is_game_randomize_start_cordinate_;
};

class CollisionManager : public collision_detector::ItemGathererProvider {
    public:
    using items_list_t = std::vector<collision_detector::Item>;
    using gatherers_list_t = std::vector<collision_detector::Gatherer>;

    static constexpr double k_dog_width = 0.3;
    static constexpr double k_office_width = 0.25;
    static constexpr double k_item_width = 0.0;

    CollisionManager(const GameSession &session)  {
        int offset = 0;
        for(const auto & dog : session.GetDogs()) {
            //Гарантировано что собаки уже отработали свой путь и переместились, благодаря приоретету
            gatherers_.push_back({dog->GetPositionBefore(),dog->GetPosition(),k_dog_width});
        }

        for(const auto & office : session.GetMap()->GetOffices()) {
            items_.push_back({{double(office.GetPosition().x),double(office.GetPosition().y)},k_office_width});
            offset++;
        }
        offset_ = offset;
        for(const auto & loot_object : session.GetLootObjects()) {
            items_.push_back({loot_object->GetPosition(),k_item_width});
        }
    }

    /*
        Возвращает сдвиг индекса с которого начинаются колизиции для Loots
        До этого индекса идут офисы
    */
    int GetOffsetLoots() {
        return offset_;
    }

    size_t ItemsCount() const override {
        return items_.size();
    }
    collision_detector::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    size_t GatherersCount() const override{
        return gatherers_.size();
    }
    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }
private:
    int offset_;

    items_list_t items_;
    gatherers_list_t gatherers_;
};

class Game : public json_loader::JsonObject {
   public:
    using Maps = std::vector<std::shared_ptr<Map>>;
    using GameSessions = std::vector<std::shared_ptr<GameSession>>;

    Game() = default;
    explicit Game(const std::filesystem::path& path) : 
        is_game_randomize_start_cordinate_(false) {
            LoadJsonFromFile(path); 
        }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;
    std::string GetJsonMaps() const;

    std::shared_ptr<GameSession> AddSession(Map::Id);
    void AddSession(std::shared_ptr<GameSession>);
    bool IsSessionStarted(Map::Id);
    std::shared_ptr<GameSession> GetSession(Map::Id);

    const GameSessions & GetSessions() const { return sessions_; }

    void TickFullGame(const std::chrono::milliseconds& ms);

    void SetRandomizeStart(bool is);

    TimeManager & GetMutableTimeManager() { return time_manager_; }
    std::shared_ptr<loot_gen::LootGenerator> GetMutableLootGenerator() { return loot_generator_; }
   public:
    boost::signals2::signal<void(std::string, int, int)> request_to_save_retired_player_s;
   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Maps maps_;
    GameSessions sessions_;
    MapIdToIndex map_id_to_index_;
    Real dog_speed_default_; 
    int default_bag_capacity_;
    Real dog_retirement_time_;

    std::shared_ptr<loot_gen::LootGenerator> loot_generator_;

    bool is_game_randomize_start_cordinate_;

    TimeManager time_manager_;
    
};

}  // namespace model
