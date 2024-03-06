#include "auto_data_saver.hpp"
#include <filesystem>
#include "logger.h"

namespace data_serializer {

namespace fs = std::filesystem;
using namespace std::literals;

DataSaver::DataSaver(app::App * app, const std::string & path) : path_(path), app_(app) {}

void DataSaver::Save() {
    std::ofstream out(path_ + "_temp"s, std::ios_base::binary);
    if(!out.is_open())
        throw std::invalid_argument("path to save data file");
    boost::archive::polymorphic_text_oarchive oa{out};
    auto size_sessions = app_->GetGame().GetSessions().size();
    oa << size_sessions;
    for(const auto &session : app_->GetGame().GetSessions()) {
        auto count_dogs = session->GetCountDogs();
        oa << count_dogs;
        for(const auto & dog : session->GetDogs()) {
            DogRepr dog_repr(dog);
            oa << dog_repr;
        }
        auto count_loot = session->GetLootObjects().size();
        oa << count_loot;
        for(const auto & loot : session->GetLootObjects()) {
            LootObjectRepr loot_repr(loot);
            oa << loot_repr;
        }
        GameSessionRepr session_repr(session);
        oa << session_repr;
    }

    auto players_count = app_->GetPlayers().GetPlayersList().size();
    oa << players_count;
    for(const auto &[token, player] : app_->GetPlayers().GetPlayersList()) {
        PlayerRepr player_repr(*token,player);
        oa << player_repr;
    }

    out.close();

    std::filesystem::rename(path_ + "_temp"s, path_);
}

void DataSaver::Load() {
    std::ifstream in(path_, std::ios_base::binary);
    boost::archive::polymorphic_text_iarchive ia{in};

    int size_sessions;
    ia >> size_sessions;

    std::vector<std::pair<std::string,std::shared_ptr<model::GameSession>>> sessions_all;
    std::vector<std::pair<std::string,std::shared_ptr<model::Dog>>> dogs_all;

    for(int i=0;i<size_sessions;i++) {
        int count_dogs;
        ia >> count_dogs;
        std::vector<std::pair<std::string,std::shared_ptr<model::Dog>>> dogs;
        for(int i=0;i<count_dogs;i++) {
            DogRepr dog_repr;
            ia >> dog_repr;
            dogs.push_back(dog_repr.GetDog());
        }

        std::vector<std::pair<std::string,std::shared_ptr<model::LootObject>>> loots;
        int loot_count;
        ia >> loot_count;
        for(int i=0;i<loot_count;i++) {
            LootObjectRepr loot_repr;
            ia >> loot_repr;
            loots.push_back(loot_repr.GetLoot());
        }

        GameSessionRepr session_repr;
        ia >> session_repr;
        std::vector<std::shared_ptr<model::Dog>> dogs_list;
        for(const auto & [_,dog] : dogs)
            dogs_list.push_back(dog);
        std::vector<std::shared_ptr<model::LootObject>> loot_list;
        for(const auto & [_,loot] : loots)
            loot_list.push_back(loot);

        auto & mutable_game = app_->GetMutableGame();        

        auto [map_id, session_pack] = session_repr.GetGameSession(dogs_list,loot_list,
                                                                        mutable_game.GetMutableTimeManager(),
                                                                        mutable_game.GetMutableLootGenerator());
        auto [session_uuid, session] = session_pack;

        auto map = app_->GetGame().FindMap(model::Map::Id(map_id));
        session->setMap(map);

        app_->GetMutableGame().AddSession(session);

        dogs_all.insert(dogs_all.end(),dogs.begin(),dogs.end());
        sessions_all.push_back(std::make_pair(session_uuid, session));
    }
    
    int players_size;
    ia >> players_size;

    app::Players::Players_t players;

    for(int i=0;i<players_size;i++) {
        PlayerRepr player_repr;
        ia >> player_repr;
        auto [token, player] = player_repr.GetPlayersMap(sessions_all, dogs_all);
        players[util::Token(token)] = player;
    }
    app_->GetMutablePlayers().SetPlayersList(players);
}

bool DataSaver::IsSaveExist() { 
    return fs::exists(path_); 
}

std::string GetUuidByPointer(void *ptr)  {
    std::ostringstream out;
    out << ptr;
    return out.str();
}


void DataSaverTimeSyncWithGame::Tick(const std::chrono::milliseconds &ms) {
    using namespace std::literals::chrono_literals;
    ms_passed_ += ms;
    if(ms_passed_ >= ms_maximum_) {
        data_saver_.Save();
        ms_passed_ = 0ms;
        //BOOST_LOG_TRIVIAL(debug) << "saved!";
    }
}

}  // namespace data_serializer