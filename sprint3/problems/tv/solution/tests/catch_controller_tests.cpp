#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/controller.h"

SCENARIO("Controller", "[Controller]") {
    using namespace std::literals;

    //TODO Mocs on integration tests
    GIVEN("Controller and TV") {
        TV tv;
        std::istringstream input;
        std::ostringstream output;
        Menu menu{input, output};
        Controller controller{tv, menu};

        auto run_menu_command = [&menu, &input](std::string command) {
            input.str(std::move(command));
            input.clear();
            menu.Run();
        };
        auto expect_output = [&output](std::string_view expected) {
            // В g++ 10.3 не реализован метод ostringstream::view(), поэтому приходится
            // использовать метод str()
            // Также в conan есть баг, из-за которого Catch2 не подхватывает поддержку string_view:
            // https://github.com/conan-io/conan-center-index/issues/13993
            // Поэтому expected преобразуется к строке, чтобы обойти ошибку компиляции
            CHECK(output.str() == std::string{expected});
        };
        auto expect_output_any_error = [&output]() {
            CHECK(output.str().substr(0,7) == "Error: ");
        };
        auto expect_extra_arguments_error = [&expect_output](std::string_view command) {
            expect_output("Error: the "s.append(command).append(
                " command does not require any arguments\n"sv));
        };
        auto expect_empty_output = [&expect_output] {
            expect_output({});
        };

        WHEN("The TV is turned off") {
            AND_WHEN("Info command is entered without arguments") {
                run_menu_command("Info"s);

                THEN("output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("Info command is entered with some arguments") {
                run_menu_command("Info some extra arguments");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("Info"s);
                }
            }

            AND_WHEN("Info command has trailing spaces") {
                run_menu_command("Info  "s);

                THEN("output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("TurnOn command is entered without arguments") {
                run_menu_command("TurnOn"s);

                THEN("TV is turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOn command is entered with some arguments") {
                run_menu_command("TurnOn some args"s);

                THEN("the error message is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with some arguments") {
                run_menu_command("SelectChannel 5"s);

                THEN("SelectChannel output") {
                    expect_output_any_error(); //Error from tv
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered without arguments") {
                run_menu_command("SelectPreviousChannel"s);

                THEN("SelectPreviousChannel output") {
                    expect_output_any_error(); //Error from tv
                }
            }
        }

        WHEN("The TV is turned on") {
            tv.TurnOn();
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);

                THEN("TV is turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOff command is entered with some arguments") {
                run_menu_command("TurnOff some args");

                THEN("the error message is printed and TV is not turned off") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }

            AND_WHEN("Info command is entered without arguments") {
                tv.SelectChannel(12);
                run_menu_command("Info"s);

                THEN("current channel is printed") {
                    expect_output("TV is turned on\nChannel number is 12\n"s);
                }
            }

            AND_WHEN("Select command is entered with arguments") {
                run_menu_command("SelectChannel 5"s);

                THEN("Channel is setted") {
                    expect_empty_output();
                }
            }

            AND_WHEN("SelectChannel command is entered without arguments") {
                run_menu_command("SelectChannel"s);

                THEN("SelectChannel output") {
                    expect_output("Error: Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with none valid channel") {
                run_menu_command("SelectChannel 120"s);

                THEN("SelectChannel output") {
                    expect_output_any_error(); //Error from tv
                }
            }

            AND_WHEN("SelectChannel command is entered with none valid channel") {
                run_menu_command("SelectChannel 120"s);

                THEN("SelectChannel output") {
                    expect_output_any_error(); //Error from tv
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered with some arguments") {
                run_menu_command("SelectPreviousChannel 5"s);

                THEN("SelectPreviousChannel output") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered without arguments") {
                run_menu_command("SelectPreviousChannel"s);

                THEN("SelectPreviousChannel output") {
                    expect_empty_output();
                }
            }
        }
    }
}
