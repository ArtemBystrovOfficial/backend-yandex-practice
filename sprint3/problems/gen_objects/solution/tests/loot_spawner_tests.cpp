#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"

using namespace std::literals;

SCENARIO("spawn loot") {
    //Не использовали моки, не нашел способ протестировать распределение без интеграционных ценностей
    GIVEN("a loot, a map") {
        auto map = std::make_shared<model::Map>(model::Map::Id(""),"");
        map->LoadJsonFromFile("../../data/test_config.json");
        auto loot_generator = std::make_shared<loot_gen::LootGenerator>(std::chrono::milliseconds(1000), 1.0f);
        model::TimeManager time_manager;
        auto session = std::make_shared<model::GameSession>(map, time_manager, 1.0, false, loot_generator);
        time_manager.AddSubscribers(session);

        WHEN("Map without loots") {
            THEN("Add loot 100% -> 1 times 1 dog") {
                session->AddDog("");
                time_manager.GlobalTick(100ms);
                REQUIRE(session->GetLootObjects().size() == 1);
                auto type = session->GetLootObjects().at(0)->GetType();
                CHECK((type == 0 || type == 1));
            }
            THEN("Add loot 100% -> 1 times 10 dog") {
                for(int i=0;i<10;i++)
                    session->AddDog("");
                time_manager.GlobalTick(100ms);   
                REQUIRE(session->GetLootObjects().size() == 10);
                for(int i=0;i<10;i++) {
                    auto type = session->GetLootObjects().at(i)->GetType();
                    CHECK((type == 0 || type == 1));
                }
                WHEN("Session have 10 loots and 10 dogs") {
                    THEN("Add 100% one more dog") {
                        session->AddDog("");
                        time_manager.GlobalTick(1ms);
                        CHECK(session->GetLootObjects().size() == 11);
                    }
                }
            }
        }
    }
}