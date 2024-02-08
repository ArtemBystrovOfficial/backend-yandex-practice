#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>
#include "http_server.h"
#include "json_loader.h"
#include "logger.h"
#include "request_handler.h"
#include "time.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;
namespace po = boost::program_options;
namespace {

struct Args {
    int tick_period;
    std::string static_path, config_path;
};

[[nodiscard]] std::optional<std::pair<Args, po::variables_map>> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"Allowed options"s};
    std::string_view requires_fields = "Usage ./server --config-file [] --www-root []"sv;
    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_path)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_path)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch(const po::error& e) {
        std::cout << e.what() << "\n" << desc;
        return std::nullopt;
    }
    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("config-file"s)) {
        std::cout << requires_fields;
        return std::nullopt;
    }
    if (!vm.contains("www-root"s)) {
        std::cout << requires_fields;
        return std::nullopt;
    }
    return std::pair<Args, po::variables_map>{args,vm};
}

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

    try {
        if (auto opt = ParseCommandLine(argc, argv)) {
            auto [args, vm] = *opt;

            //INITIAL BOOST ASIO
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                }
            });

            //APP SETTINGS
            app::App app(args.config_path);
            auto & mutable_game = app.GetMutableGame();
            if(vm.contains("randomize-spawn-points")) 
                mutable_game.SetRandomizeStart(true);
            //Пока strand для api глобальный но в будущем можно пересмотреть
            //и блокировать лишь определенные запросы к разделяемым файлам
            auto api_strand = net::make_strand(ioc);

            std::shared_ptr<model::Ticker> ticker(nullptr);
            if(vm.contains("tick-period")) {
                ticker = std::make_shared<model::Ticker>(api_strand, std::chrono::milliseconds(args.tick_period),
                    [&mutable_game](std::chrono::milliseconds delta) { mutable_game.TickFullGame(delta); }
                );
                ticker->Start();
            } else 
                app.SetTickEditAccess(true);
            

            api::ApiProxyKeeper api_keeper(api_strand, app);

            http_handler::RequestHandler handler(api_keeper, args.static_path);

            http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            BOOST_LOG_TRIVIAL(info) << "server started"
                                    << logging::add_value(additional_data, json_loader::CreateTrivialJson({"port", "address"}, port, address));

            RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
        } else 
            exit(1);
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(info) << "server exited"
                                << logging::add_value(additional_data,
                                                      json_loader::CreateTrivialJson({"code", "exception"}, EXIT_FAILURE, ex.what()));
        return EXIT_FAILURE;
    }
    BOOST_LOG_TRIVIAL(info) << "server exited" << logging::add_value(additional_data, json_loader::CreateTrivialJson({"code", "exception"}, 0, ""sv));
    return 0;
}
