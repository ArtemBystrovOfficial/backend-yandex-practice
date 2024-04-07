#pragma once
#include <string_view>
#include "app.h"
#include <memory>
#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

namespace model {
template<class Archive>
void serialize(Archive & ar, SpeedF & speed, [[maybe_unused]] const unsigned version) {
    ar& speed.x;
    ar& speed.y;
}
}

namespace std {
template<class Archive>
void serialize(Archive & ar, pair<int,int> & pair, [[maybe_unused]] const unsigned version) {
    ar& pair.first;
    ar& pair.second;
}
}

template<class Archive>
void serialize(Archive & ar, PointF & point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

namespace data_serializer {

std::string GetUuidByPointer(void * ptr);

class LootObjectRepr {
public:

    LootObjectRepr() = default;

    explicit LootObjectRepr(std::shared_ptr<model::LootObject> loot) 
    : uuid_(GetUuidByPointer(loot.get())),
    position_(loot->GetPosition()),
    type_(loot->GetType()),
    id_(loot->GetId()){
    }

    std::pair<std::string, std::shared_ptr<model::LootObject> >GetLoot() {
        auto loot = std::make_shared<model::LootObject>();
        loot->SetId(id_);
        loot->SetType(type_);
        loot->SetPosition(position_);
        return {uuid_, loot};
    }

    template<class Archive>
    void serialize(Archive & ar, [[maybe_unused]] const unsigned version) {
        ar& uuid_;
        ar& position_;
        ar& type_;
        ar& id_;
    }

private:
    std::string uuid_;

    std::string map_id_;

    PointF position_;
    int type_;
    int id_;
};

//Обертка над сериализацией
class DogRepr {
public:

    DogRepr() = default;

    explicit DogRepr(std::shared_ptr<model::Dog> dog)
        : uuid_(GetUuidByPointer(dog.get())),   
          id_(*dog->GetId()),
          name_(dog->GetName()),
          position_(dog->GetPosition()),
          bag_capacity_(dog->GetBag().max_count),
          speed_(dog->GetSpeed()),
          map_speed_(dog->GetMapSpeed()),
          direction_(dog->GetDirection()),
          score_(dog->GetScore()),
          bag_content_(dog->GetBag().items) {
    }

    std::pair<std::string, std::shared_ptr<model::Dog> >GetDog() {
        auto dog = std::make_shared<model::Dog>();
        dog->SetId(model::Dog::Id(id_));
        dog->SetName(name_);
        dog->SetPosition(position_);
        dog->SetScore(score_);
        dog->SetSpeed(speed_);
        dog->SetMapSpeed(map_speed_);
        dog->SetDirection(direction_);
        dog->SetBag(model::Bag{bag_content_, bag_capacity_});
        return {uuid_, dog};
    }

    template<class Archive>
    void serialize(Archive & ar, [[maybe_unused]] const unsigned version) {
        ar& uuid_;
        ar& id_;
        ar& name_;
        ar& position_;
        ar& bag_capacity_;
        ar& map_speed_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:

    std::string uuid_;

    int id_;
    std::string name_;
    PointF position_;
    //PointF position_before_;
    int bag_capacity_ = 0;
    Real map_speed_;
    model::SpeedF speed_;
    model::Direction direction_ = model::Direction::NORTH;
    int score_ = 0;
    std::vector<std::pair<int,int>> bag_content_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(std::shared_ptr<model::GameSession> session)
        : uuid_(GetUuidByPointer(session.get())),
          map_id_(*session->GetMap()->GetId()),
          default_speed_(session->GetDefaultSpeed()),
          default_bag_capacity_(session->GetBagCapacity()),
          last_id_object_(session->GetLastIdObject()),
          is_game_randomize_start_cordinate_(session->GetIsGameRandomizeStartCoords()) {
    }

    std::pair<std::string, std::pair<std::string, std::shared_ptr<model::GameSession>>> GetGameSession(
        const std::vector<std::shared_ptr<model::Dog>> & dogs, const std::vector<std::shared_ptr<model::LootObject>> & loots,
        model::TimeManager & manager, std::shared_ptr<loot_gen::LootGenerator> loot_generator ) const {
        auto session = std::make_shared<model::GameSession>(manager, loot_generator);
        for(const auto & dog : dogs) 
            session->AddDog(dog);
        session->SetLootObjects(loots);
        session->SetDefaultSpeed(default_speed_);
        session->SetLastIdObject(last_id_object_);
        session->SetGameRandomizeStartCoords(is_game_randomize_start_cordinate_);
        session->SetBagCapacity(default_bag_capacity_);
        
        return {map_id_, {uuid_, session}};
    }
    template<class Archive>
    void serialize(Archive & ar, [[maybe_unused]] const unsigned version) {
        ar& uuid_;
        ar& map_id_;
        ar& _last_dog_id;
        ar& default_speed_;
        ar& default_bag_capacity_;
        ar& last_id_object_;
        ar& is_game_randomize_start_cordinate_;
    }

private:
    std::string uuid_;

    std::string map_id_;

    int _last_dog_id;
    Real default_speed_;
    int default_bag_capacity_;
    int last_id_object_;
    bool is_game_randomize_start_cordinate_;
};

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const std::string & token, std::shared_ptr<app::Player> player)
        : token_(token),
          uuid_dog_(GetUuidByPointer(player->dog_.get())),
          uuid_session_(GetUuidByPointer(player->session_.get())) {
    }

    std::pair<std::string, std::shared_ptr<app::Player>> GetPlayersMap(const std::vector<std::pair<std::string,std::shared_ptr<model::GameSession> > > & sessions,
                                          const std::vector<std::pair<std::string, std::shared_ptr<model::Dog> > > & dogs) {
        app::Players::Players_t players_map;
        for(const auto & [ses_uuid, session] : sessions) {
            for(const auto & [dog_uuid, dog] : dogs) {
                if(ses_uuid == uuid_session_ && uuid_dog_ == dog_uuid)
                    return {token_, std::shared_ptr<app::Player>(new app::Player{session,dog})};
            }
        }

        //Собаки без владельца будут уничтожены
        return {};
    }

    template<class Archive>
    void serialize(Archive & ar, [[maybe_unused]] const unsigned version) {
        ar& token_;
        ar& uuid_dog_;
        ar& uuid_session_;
    }

private:
    std::string token_;
    std::string uuid_dog_;
    std::string uuid_session_;
};

class DataSaver {
    public:
        explicit DataSaver(app::App * app, const std::string & path);

        void Save();
        void Load();
        bool IsSaveExist();

    private:
        std::string path_;
        app::App * app_;
};

class DataSaverTimeSyncWithGame : public model::TimeObject {
    public:
        DataSaverTimeSyncWithGame(DataSaver & data_saver, std::chrono::milliseconds ms_maximum) 
            : data_saver_(data_saver) , ms_passed_(0), ms_maximum_(ms_maximum) {};

        void Tick(const std::chrono::milliseconds& ms) override;
    private:
        std::chrono::milliseconds ms_passed_;
        std::chrono::milliseconds ms_maximum_;
        DataSaver & data_saver_;
};

}