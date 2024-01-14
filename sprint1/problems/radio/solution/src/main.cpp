#include <iostream>

#include "client-server.h"

using namespace std::literals;
static constexpr auto kRadioPort = 21112;

void printHelp(bool is_client = false) {
    std::cout << (is_client ? "radio [client] [255.255.255.255]\n"sv
                            : "radio [client] or [server]\n"sv);
}

int main(int argc, char** argv) {
    try {
        net::io_context context;
        if (argc == 2) {
            if (strcmp(argv[1], "server")) {
                std::cout << "mega test\n";
                printHelp();
                return -1;
            }

            Server server(context, kRadioPort);
            while (true) {
                server.ReceiveAndPlayPart();
                std::cout << "AudioPlayed" << std::endl;
            }
        }

        if (argc == 3) {
            if (strcmp(argv[1], "client")) {
                printHelp(true);
                return -1;
            }
            Client client(context, argv[2], kRadioPort);
            std::string _;
            std::cout << "Press Enter to start radio.." << std::endl;
            std::getline(std::cin, _);
            // Сделал, чтобы поток радио был постоянным и записывался порциями
            while (true) {
                client.RecordAndSendPart();
            }
        }

        printHelp();
        return -1;
    } catch (const std::exception& er) {
        std::cout << er.what();
        return -1;
    }
}
