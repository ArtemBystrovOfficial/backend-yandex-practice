#include "auto_data_saver.hpp"

namespace data_serializer {

DataSaver::DataSaver(const std::string & path) : path_(path) {}

void DataSaver::Save(const app::App & app) {
    std::ofstream out(path_, std::ios_base::binary);
    if(!out.is_open())
        throw std::invalid_argument("path to save data file");
    boost::archive::polymorphic_text_oarchive oa{out};
    auto size_sessions = app.GetGame().GetSessions().size();
    oa << size_sessions;
    for(const auto &session : app.GetGame().GetSessions()) {
        auto count_dogs = session->GetCountDogs();
        oa << count_dogs;
        for(const auto & dog : session->GetDogs()) {
            DogRepr dog_repr(dog);
            oa << dog_repr;
        }
        auto count_sessions = session->GetLootObjects().size();
        oa << count_sessions;
        for(const auto & loot : session->GetLootObjects()) {
            LootObjectRepr loot_repr(loot);
            oa << loot_repr;
        }
        GameSessionRepr session_repr(session);
        oa << session_repr;
    }

    auto players_count = app.GetPlayers().GetPlayersList().size();
    oa << players_count;
    for(const auto &[token, player] : app.GetPlayers().GetPlayersList()) {
        PlayerRepr player_repr(*token,player);
        oa << player_repr;
    }

    out.close();
}

void DataSaver::Load(app::App & app) {
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
        ia >> count_dogs;
        for(int i=0;i<count_dogs;i++) {
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

        auto & mutable_game = app.GetMutableGame();        

        auto [map_id, session_pack] = session_repr.GetGameSession(dogs_list,loot_list,
                                                                        mutable_game.GetMutableTimeManager(),
                                                                        mutable_game.GetMutableLootGenerator());
        auto [session_uuid, session] = session_pack;

        auto map = app.GetGame().FindMap(model::Map::Id(map_id));
        session->setMap(map);

        app.GetMutableGame().AddSession(session);

        dogs.assign(dogs.begin(),dogs.end());
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
    app.GetMutablePlayers().SetPlayersList(players);
}

std::string GetUuidByPointer(void *ptr)  {
    std::ostringstream out;
    out << ptr;
    return out.str();
}

}  // namespace data_serializer