#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "http_server.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);

    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, char* argv[]) {
    InitBoostLogFilter();

    // DEBUG
    //int argc = 3;
    //const char* argv[] = {"", "../../data/config.json", "../../static"};

    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static folder>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        app::App app(argv[1]);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        api::ApiProxyKeeper api_keeper(ioc, app);

        http_handler::RequestHandler handler(api_keeper, argv[2]);

        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
            handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        BOOST_LOG_TRIVIAL(info) << "server started"
                                << logging::add_value(additional_data, json_loader::CreateTrivialJson({"port", "address"}, port, address));

        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(info) << "server exited"
                                << logging::add_value(additional_data,
                                                      json_loader::CreateTrivialJson({"code", "exception"}, EXIT_FAILURE, ex.what()));
        return EXIT_FAILURE;
    }
    BOOST_LOG_TRIVIAL(info) << "server exited" << logging::add_value(additional_data, json_loader::CreateTrivialJson({"code", "exception"}, 0, ""sv));
    return 0;
}
