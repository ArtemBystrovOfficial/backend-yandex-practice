#define _USE_MATH_DEFINES

#include <catch2/catch_all.hpp>
#include <boost/range/combine.hpp>
using Catch::Matchers::WithinAbs;
#include "../src/collision_detector.h"

namespace collision_detector {

class TestGathererProvider : public ItemGathererProvider {
    public:
    using items_list_t = std::vector<Item>;
    using gatherers_list_t = std::vector<Gatherer>;

    TestGathererProvider(const items_list_t & items, const gatherers_list_t & gatherers) : items_(items), gatherers_(gatherers) {}

    size_t ItemsCount() const override {
        return items_.size();
    }
    Item GetItem(size_t idx) const override {
        return items_[idx];
    }
    size_t GatherersCount() const override{
        return gatherers_.size();
    }
    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }
private:
    items_list_t items_;
    gatherers_list_t gatherers_;
};

using events_t = std::vector<GatheringEvent>;

}

namespace Catch {

template <>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(const collision_detector::GatheringEvent & event) {
        using namespace std::literals;
        return "Gatherer id: "s + std::to_string(event.gatherer_id) + 
               " Item id: "s + std::to_string(event.item_id) +
               " Sq distance: "s + std::to_string(event.sq_distance) +
               " Time: "s + std::to_string(event.time);
    }
};

}  // namespace Catch 

struct IsSortedEventsMatcher : Catch::Matchers::MatcherGenericBase {
    IsSortedEventsMatcher() = default;
    IsSortedEventsMatcher(IsSortedEventsMatcher&&) = default;

    bool match(const collision_detector::events_t & range) const {
        using std::begin;
        using std::end;

        return is_sorted(begin(range), end(range),[](const auto & value1, const auto & value2) { 
            return value1.time < value2.time;
        });
    }

    std::string describe() const override {
        using namespace std::literals;
        return "None sorted"s;
    }

}; 

void TestSorted(const collision_detector::events_t & events) {
    CHECK_THAT(events, IsSortedEventsMatcher());
}

void TestDataEq(const collision_detector::events_t & events, const collision_detector::events_t & answer) {
    using namespace collision_detector;
    REQUIRE( events.size() == answer.size() );
    std::equal(begin(events),end(events),begin(answer),[](const GatheringEvent & event1, const GatheringEvent & event2){
        CHECK(event1.gatherer_id == event2.gatherer_id);
        CHECK(event1.item_id == event2.item_id);
        CHECK_THAT(event1.sq_distance, WithinAbs(event2.sq_distance,1e-10));
        return true;
    });
}

void TestEvents(const collision_detector::events_t & events, const collision_detector::events_t & answer) {
    TestSorted(events);
    TestDataEq(events, answer);
}

TEST_CASE("Collision detector 1i 1g") {
    using namespace collision_detector;
    auto provider = TestGathererProvider(
                                         {
                                           { {0, 0.5}, 0.1 }
                                         },
                                         {
                                           { {0,0}, {0,1}, 0.2 }
                                         }
                                        );
    auto answer = events_t ({
        { 0 , 0 , pow(0.1 + 0.2, 2), 0}
    });

    auto events = FindGatherEvents(provider);

    TestEvents(events, answer);
}

void ExecuteSquareTest(double R, double r, double gather_x_distance) {
    using namespace collision_detector;
    auto CalculateItems = [](double R, double r, double gath_x_distanse) {
        REQUIRE((R+r*2)<gath_x_distanse*sqrt(2));
        TestGathererProvider::items_list_t items;
        double delta = R*r*sqrt(2);
        double inaccuracy = 1e-3;
        items.push_back(Item{{delta,-inaccuracy},r});
        items.push_back(Item{{-1+delta,-1+inaccuracy},r});
        return items;
    };

    auto provider = TestGathererProvider(
                                         {
                                           CalculateItems(R,r,gather_x_distance)
                                         },
                                         {
                                           { {0,0}, {-gather_x_distance, -gather_x_distance}, R }
                                         }
                                        );
    auto answer = events_t ({
        {0 ,0 ,pow(R + r, 2), 0},
        {1 ,0 ,pow(R + r, 2), 0},
    });

    
    auto events = FindGatherEvents(provider);
    TestEvents(events, answer);
}

TEST_CASE("Collision detector 2i 1g") {
    ExecuteSquareTest(0.3,0.1,1);
    ExecuteSquareTest(15,14,100);
    ExecuteSquareTest(200,100,10000);
}

TEST_CASE("Collision detector 1i 2g") {
    using namespace collision_detector;
    auto provider = TestGathererProvider(
                                         {
                                           { {2./3.,1./3.}, 1e-5 }
                                         },
                                         {
                                           { {0,0}, {2,1}, 1e-5 },
                                           { {2,0}, {1,1}, 1e-5 }
                                         }
                                        );
    auto answer = events_t ({
        {0 ,1 ,pow(1e-5 + 1e-5, 2), 0},
        {0 ,0 ,pow(1e-5 + 1e-5, 2), 0}
    });

    auto events = FindGatherEvents(provider);

    TestEvents(events, answer);
}

// Напишите здесь тесты для функции collision_detector::FindGatherEvents