#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/tv.h"

namespace Catch {

template <>
struct StringMaker<std::nullopt_t> {
    static std::string convert(std::nullopt_t) {
        using namespace std::literals;
        return "nullopt"s;
    }
};

template <typename T>
struct StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& opt_value) {
        if (opt_value) {
            return StringMaker<T>::convert(*opt_value);
        } else {
            return StringMaker<std::nullopt_t>::convert(std::nullopt);
        }
    }
};

}  // namespace Catch

SCENARIO("TV", "[TV]") {
    GIVEN("A TV") {  // Дано: Телевизор
        TV tv;

        // Изначально он выключен и не показывает никаких каналов
        SECTION("Initially it is off and doesn't show any channel") {
            CHECK(!tv.IsTurnedOn());
            CHECK(!tv.GetChannel().has_value());
        }

        // Когда он выключен,
        WHEN("it is turned off") {
            REQUIRE(!tv.IsTurnedOn());
            // он не может переключать каналы
            THEN("it can't select any channel") {
                CHECK_THROWS_AS(tv.SelectChannel(10), std::logic_error);
                CHECK(tv.GetChannel() == std::nullopt);
                tv.TurnOn();
                CHECK(tv.GetChannel() == 1);
            }
        }

        WHEN("it is turned on first time") {  // Когда его включают в первый раз,
            tv.TurnOn();

            // то он включается и показывает канал #1
            THEN("it is turned on and shows channel #1") {
                CHECK(tv.IsTurnedOn());
                CHECK(tv.GetChannel() == 1);

                // А когда его выключают,
                AND_WHEN("it is turned off") {
                    tv.TurnOff();

                    // то он выключается и не показывает никаких каналов
                    THEN("it is turned off and doesn't show any channel") {
                        CHECK(!tv.IsTurnedOn());
                        CHECK(tv.GetChannel() == std::nullopt);
                    }
                }
            }
            // И затем может выбирать канал с 1 по 99
            AND_THEN("it can select channel from 1 to 99") {
                for(int i=1;i<=99;i++) {
                    tv.SelectChannel(i);
                    CHECK(tv.GetChannel().has_value());
                    CHECK(tv.GetChannel().value() == i);
                }
                AND_WHEN("it selected #99 channel and we select last viewed channel") {
                    tv.SelectLastViewedChannel();
                    THEN("now channel is #98") {
                        CHECK(tv.GetChannel().has_value());
                        CHECK(tv.GetChannel() == 98);
                        AND_WHEN("repeated it, channel is #99 yet ") {
                            tv.SelectLastViewedChannel();
                            THEN("now channel is #99") {
                                CHECK(tv.GetChannel().has_value());
                                CHECK(tv.GetChannel() == 99);
                            }
                        }
                    } 
                }
            }
            AND_THEN("it can't select channel lower then 1 and higher then #99") {
                CHECK_THROWS_AS(tv.SelectChannel(-1), std::out_of_range);
                CHECK_THROWS_AS(tv.SelectChannel(0), std::out_of_range);
                CHECK_THROWS_AS(tv.SelectChannel(100), std::out_of_range);
            }
        }
    }
}
