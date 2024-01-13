#pragma once
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include "json_loader.h"
#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

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
    virtual void LoadJsonNode(const ptree& tree) override {
        start_.x = tree.get<Dimension>("x0");
        start_.y = tree.get<Dimension>("y0");

        auto opt = tree.get_optional<Dimension>("y1");

        if (opt) {
            end_.y = *opt;
            end_.x = start_.x;
        } else {
            end_.y = start_.y;
            end_.x = tree.get<Dimension>("x1");
        }
    }
    virtual ptree GetJsonNode() const override {
        ptree tree;
        tree.put("x0", start_.x);  // only for te
        tree.put("y0", start_.y);

        if (IsHorizontal())
            tree.put("x1", end_.x);
        else
            tree.put("y1", end_.y);
        return tree;
    }

    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(const ptree& tree) { LoadJsonNode(tree); }

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}, end_{end_x, start.y} {}

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}, end_{start.x, end_y} {}

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

    virtual void LoadJsonNode(const ptree& tree) override {
        bounds_.position.x = tree.get<Dimension>("x");
        bounds_.position.y = tree.get<Dimension>("y");
        bounds_.size.width = tree.get<Dimension>("w");
        bounds_.size.height = tree.get<Dimension>("h");
    }
    virtual ptree GetJsonNode() const override {
        ptree tree;
        boost::property_tree::ptree buildingNode;
        buildingNode.put_value<int>(bounds_.position.x);
        tree.add_child("x", buildingNode);

        // tree.put("x", bounds_.position.x);
        tree.put("y", bounds_.position.y);
        tree.put("w", bounds_.size.width);
        tree.put("h", bounds_.size.height);
        return tree;
    }

    const Rectangle& GetBounds() const noexcept { return bounds_; }

   private:
    Rectangle bounds_;
};

class Office : public json_loader::JsonObject {
   public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}, position_{position}, offset_{offset} {}

    Office(const ptree& tree) : id_("") { LoadJsonNode(tree); }

    const Id& GetId() const noexcept { return id_; }

    Point GetPosition() const noexcept { return position_; }
    Offset GetOffset() const noexcept { return offset_; }

    virtual void LoadJsonNode(const ptree& tree) override {
        *id_ = tree.get<std::string>("id");
        position_.x = tree.get<Dimension>("x");
        position_.y = tree.get<Dimension>("y");
        offset_.dx = tree.get<Dimension>("offsetX");
        offset_.dy = tree.get<Dimension>("offsetY");
    }
    virtual ptree GetJsonNode() const override {
        ptree tree;
        tree.put("id", *id_);
        tree.put("x", position_.x);
        tree.put("y", position_.y);
        tree.put("offsetX", offset_.dx);
        tree.put("offsetY", offset_.dy);
        return tree;
    }

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

    Map(Id id, std::string name) noexcept
        : id_(std::move(id)), name_(std::move(name)) {}

    Map(const ptree& tree) : id_("") { LoadJsonNode(tree); }

    const Id& GetId() const noexcept { return id_; }

    const std::string& GetName() const noexcept { return name_; }

    const Buildings& GetBuildings() const noexcept { return buildings_; }

    const Roads& GetRoads() const noexcept { return roads_; }

    const Offices& GetOffices() const noexcept { return offices_; }

    void AddRoad(const Road& road) { roads_.emplace_back(road); }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    virtual void LoadJsonNode(const ptree& tree) override {
        *id_ = tree.get<std::string>("id");
        name_ = tree.get<std::string>("name");

        for (const auto& [_, road] : tree.get_child("roads"))
            roads_.push_back({road});

        for (const auto& [_, building] : tree.get_child("buildings"))
            buildings_.push_back({building});

        for (const auto& [_, offices] : tree.get_child("offices"))
            offices_.push_back({offices});
    }
    virtual ptree GetJsonNode() const override {
        ptree tree;
        tree.put("id", *id_);
        tree.put("name", name_);

        ptree roads;
        for (auto& road : roads_) roads.push_back({"", road.GetJsonNode()});

        ptree buildings;
        for (auto& building : buildings_)
            buildings.push_back({"", building.GetJsonNode()});

        ptree offices;
        for (auto& office : offices_)
            offices.push_back({"", office.GetJsonNode()});

        tree.add_child("roads", roads);
        tree.add_child("buildings", buildings);
        tree.add_child("offices", offices);

        return tree;
    }

    void AddOffice(Office office);

   private:
    using OfficeIdToIndex =
        std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Game : public json_loader::JsonObject {
   public:
    using Maps = std::vector<Map>;

    Game() = default;
    Game(const std::filesystem::path& path) { LoadJsonFromFile(path); }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept { return maps_; }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    virtual void LoadJsonNode(const ptree& tree) override {
        for (const auto& [_, map] : tree.get_child("maps")) AddMap({map});
    }
    virtual ptree GetJsonNode() const override {
        ptree tree, maps;

        // TODO Сделать макрос для автоматического заполнения массивов
        for (const auto& map : maps_) maps.push_back({"", map.GetJsonNode()});
        tree.add_child("maps", maps);

        return tree;
    }
    std::string GetJsonMaps() const {
        using namespace std::literals;
        std::string s = "[";
        for (const auto& map : maps_) {
            s += "{"s + "\"id\": \""s + *map.GetId() + "\", \"name\": \""s +
                 map.GetName() + "\""s + "},"s;
        }
        if (!maps_.empty()) s.pop_back();  // last ,
        s += "]";
        return s;
    }

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
};

}  // namespace model
