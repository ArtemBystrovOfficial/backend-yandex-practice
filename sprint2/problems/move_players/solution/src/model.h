#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "json_loader.h"
#include "tagged.h"

namespace model {

using Real = float;
using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct PointF {
    Real x, y;
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
    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(const ptree& tree) { LoadJsonNode(tree); }

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

    Building(const ptree& tree) { LoadJsonNode(tree); }

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

    Office(const ptree& tree) : id_("") { LoadJsonNode(tree); }

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

    Map(Id id, std::string name) noexcept : id_(std::move(id)), name_(std::move(name)) {}

    Map(const ptree& tree) : id_("") { LoadJsonNode(tree); }

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }

    bool HasCustomSpeed() { return dog_custom_speed_.has_value(); }
    Real CustomSpeed() noexcept(false) { return *dog_custom_speed_; }

    const Buildings& GetBuildings() const noexcept { return buildings_; }
    const Roads& GetRoads() const noexcept { return roads_; }
    const Offices& GetOffices() const noexcept { return offices_; }

    void AddRoad(const Road& road) { roads_.emplace_back(road); }
    void AddBuilding(const Building& building) { buildings_.emplace_back(building); }

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;

    void AddOffice(Office office);

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    std::optional<Real> dog_custom_speed_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
   public:
    using Id = util::Tagged<size_t, Dog>;

    Dog(Id id, std::string_view name, PointF position, Real map_speed)
        : id_(id), name_(name.data(), name.size()), position_(position), speed_({0.0f, 0.0f}), direction_(Direction::NORTH), map_speed_(map_speed){};

    bool IsValid() { return !name_.empty(); }

    void SetId(const Id&);

    const Id& GetId();
    const std::string& GetName();
    const PointF& GetPosition() { return position_; }
    const Direction& GetDirection() { return direction_; }
    const SpeedF& GetSpeed() { return speed_; }

    bool MoveDog(Direction);
    void StopDog();

    char GetDirectionChar() { return static_cast<char>(direction_); }

   private:
    Id id_;
    std::string name_;

    PointF position_;
    SpeedF speed_;
    Real map_speed_;
    Direction direction_;
};

class GameSession {
   public:
    using Dogs = std::vector<std::shared_ptr<Dog>>;

    GameSession(std::shared_ptr<Map> map, Real default_speed);

    std::shared_ptr<Dog> AddDog(std::string_view dog_name);
    std::shared_ptr<Dog> FindDogByID(Dog::Id);

    std::shared_ptr<Map> GetMap();

    const Dogs& GetDogs() { return dogs_; }

    // Сделаем систему создания комнат или автоматическое распределение по картам, но сейчас одна сессия одна карта
    size_t GetCountDogs() { return dogs_.size(); }

   private:
    int _last_dog_id;
    Real default_speed_;
    std::shared_ptr<Map> map_;
    Dogs dogs_;
};

class Game : public json_loader::JsonObject {
   public:
    using Maps = std::vector<std::shared_ptr<Map>>;
    using GameSessions = std::vector<std::shared_ptr<GameSession>>;

    Game() = default;
    Game(const std::filesystem::path& path) { LoadJsonFromFile(path); }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    void LoadJsonNode(const ptree& tree) override;
    ptree GetJsonNode() const override;
    std::string GetJsonMaps() const;

    std::shared_ptr<GameSession> AddSession(Map::Id);
    bool IsSessionStarted(Map::Id);
    std::shared_ptr<GameSession> GetSession(Map::Id);

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Maps maps_;
    GameSessions sessions_;
    MapIdToIndex map_id_to_index_;
    Real dog_speed_default_;
};

}  // namespace model
