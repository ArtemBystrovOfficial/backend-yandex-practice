#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <atomic>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

#include "seabattle.h"

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()),
               ec);

    return !ec;
}

class SeabattleAgent {
    static constexpr size_t MOVE_HEADER_SIZE = 2;
    static constexpr size_t RESULT_HEADER_SIZE = sizeof(char);
    using ShotResult = SeabattleField::ShotResult;

   public:
    SeabattleAgent(const SeabattleField& field) : my_field_(field) {}

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded()) {
            system("clear");
            PrintFields();
            ShotResult result;
            if (my_initiative) {
                auto [y, x] = InputUser();

                if (!WriteExact(socket, MoveToString({y, x})))
                    throw std::runtime_error("send data");

                result = WaitResult(socket);

                switch (result) {
                    case ShotResult::HIT:
                        other_field_.MarkHit(x, y);
                        break;
                    case ShotResult::KILL:
                        other_field_.MarkKill(x, y);
                        break;
                    case ShotResult::MISS:
                        other_field_.MarkMiss(x, y);
                        break;
                    default:
                        throw std::runtime_error("unknown result");
                }
            } else {
                auto [y, x] = WaitCoords(socket);
                result = my_field_.Shoot(x, y);

                WriteExact(socket, std::string(1, static_cast<char>(result)));
            }
            if (result == ShotResult::MISS) my_initiative = !my_initiative;
        }
    }

   private:
    static std::optional<std::pair<int, int>> ParseMove(
        const std::string_view& sv) {
        if (sv.size() != MOVE_HEADER_SIZE) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A',
                       static_cast<char>(move.second) + '1'};
        return {buff, MOVE_HEADER_SIZE};
    }

    void PrintFields() const { PrintFieldPair(my_field_, other_field_); }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::pair<int, int> InputUser() const {
        std::string buf;
        // flush cin TODO
        while (true) {
            std::cout << "Shooting coords: "sv;
            std::cin >> buf;
            auto coords_opt = ParseMove(buf);
            if (!coords_opt.has_value()) continue;
            if (other_field_(coords_opt->second, coords_opt->first) !=
                SeabattleField::State::UNKNOWN)
                continue;
            return *coords_opt;
        }
    }

    std::pair<int, int> WaitCoords(tcp::socket& socket) {
        std::cout << "Waiting for the opponent move...\n";
        auto result_opt = ReadExact<MOVE_HEADER_SIZE>(socket);
        if (result_opt.has_value()) {
            auto move_opt = ParseMove(*result_opt);
            if (move_opt.has_value()) {
                return *move_opt;
            } else
                throw std::runtime_error("result: parse data");
        } else
            throw std::runtime_error("coords: read data");
    }

    ShotResult WaitResult(tcp::socket& socket) {
        auto result_opt = ReadExact<RESULT_HEADER_SIZE>(socket);
        if (result_opt.has_value()) {
            auto result = static_cast<ShotResult>((*result_opt)[0]);
            if (result < ShotResult::MISS || result > ShotResult::KILL)
                throw std::runtime_error("result: parse data");
            return result;
        } else
            throw std::runtime_error("result: read data");
    }

   private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context context;

    tcp::acceptor acceptor(context, tcp::endpoint(tcp::v4(), port));

    tcp::socket socket{context};
    acceptor.accept(socket);

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str,
                 unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context context;
    tcp::socket socket{context};

    socket.connect(tcp::endpoint(net::ip::make_address(ip_str), port));

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    try {
        if (argc != 3 && argc != 4) {
            std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
            return 1;
        }

        std::mt19937 engine(std::stoi(argv[1]));
        SeabattleField fieldL = SeabattleField::GetRandomField(engine);

        if (argc == 3) {
            StartServer(fieldL, std::stoi(argv[2]));
        } else if (argc == 4) {
            StartClient(fieldL, argv[2], std::stoi(argv[3]));
        }
    } catch (const std::exception& ex) {
        std::cout << ex.what();
        return -1;
    }
    return 0;
}
