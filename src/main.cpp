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
#include "auto_data_saver.hpp"
#include <optional>

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;
namespace po = boost::program_options;
namespace {
 
struct Args {
    int tick_period, save_state_period;
    std::string static_path, config_path, state_file;
};

namespace {

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

std::string GetDBUrlFromEnv() {
    std::string db_url;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        db_url = url;
    } else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return db_url;
}

}  // namespace

[[nodiscard]] std::optional<std::pair<Args, po::variables_map>> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"Allowed options"s};
    std::string_view requires_fields = "Usage ./server --config-file [] --www-root []"sv;
    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("state-file", po::value(&args.state_file)->value_name("file"s), "make save/load after crash")
        ("save-state-period", po::value(&args.save_state_period)->value_name("milliseconds"s), "make save/load after selected milliseconds")
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
    //Временное решение потом сделать более сложный рандомайзер
    srand(time(0));

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
            app::App app(args.config_path, GetDBUrlFromEnv());

            std::shared_ptr<data_serializer::DataSaverTimeSyncWithGame> time_sync;
            std::optional<data_serializer::DataSaver> data_saver;
            if(vm.contains("state-file"))
                data_saver = data_serializer::DataSaver(&app, args.state_file);


            if(data_saver) {
                if(data_saver->IsSaveExist()) {
                    data_saver->Load();
                }
                if(vm.contains("save-state-period")) {
                    time_sync = std::make_shared<data_serializer::DataSaverTimeSyncWithGame>
                        (data_saver.value(),std::chrono::milliseconds(args.save_state_period));
                    app.GetMutableGame().GetMutableTimeManager().AddSubscribers(time_sync, 0); //Сохранение в приоретете самое последнее
                }
            }
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

            http_handler::RequestHandler handler(api_strand, api_keeper, args.static_path);

            http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

            BOOST_LOG_TRIVIAL(info) << "server started"
                                    << logging::add_value(additional_data, json_loader::CreateTrivialJson({"port", "address"}, port, address));

            RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });
            if(data_saver) 
                data_saver->Save();
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
