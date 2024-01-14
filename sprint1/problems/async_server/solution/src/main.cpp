#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view ONLY_READ = "GET, HEAD"sv;
};

inline auto prepareTarget(const boost::beast::string_view& bsv) {
    return std::string(bsv.data() + 1, bsv.size() - 1);
}

class ThreadChecker {
   public:
    explicit ThreadChecker(std::atomic_int& counter) : counter_{counter} {}

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

   private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

StringResponse MakeStringResponse(
    http::status status, std::string_view body, unsigned http_version,
    bool keep_alive, std::string_view allow = "",
    std::string_view content_type = ContentType::TEXT_HTML) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    if (!allow.empty()) response.set(http::field::allow, allow);
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) {
    const auto text_response = [&req](http::status status,
                                      std::string_view text) {
        return MakeStringResponse(status, text, req.version(),
                                  req.keep_alive());
    };

    if (req.method_string() == "GET")
        return text_response(http::status::ok,
                             "Hello, "s + prepareTarget(req.target()));

    if (req.method_string() == "HEAD")
        return text_response(http::status::ok, "");

    return MakeStringResponse(http::status::method_not_allowed,
                              "Invalid method"sv, req.version(),
                              req.keep_alive(), ContentType::ONLY_READ);
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(
        ioc, {address, port}, [](StringRequest&& req, auto&& sender) {
            sender(HandleRequest(std::forward<decltype(req)>(req)));
        });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов
    // обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] { ioc.run(); });

    std::cout << "Server closed\n";
}
