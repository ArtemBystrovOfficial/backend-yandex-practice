#pragma once
#include "sdk.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
// #define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <iostream>
#include <variant>

// #include "common.h"

namespace http_server {

// using HttpResponse = http::response<http::string_body>;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;

void ReportError(beast::error_code ec, std::string_view what);

class SessionBase {
   protected:
    using HttpRequest = http::request<http::string_body>;

   public:
    SessionBase(const SessionBase&&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();

   private:
    void Read();

    void LogRead();
    void LogWrite(int status_code, std::string_view content_type);

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void Close();

    virtual void HandleRequest(HttpRequest&& request) = 0;
    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;

   protected:
    explicit SessionBase(tcp::socket&& socket, net::io_context& ioc) : stream_(std::move(socket)), ioc_(ioc) {}
    ~SessionBase() = default;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        // Запись выполняется асинхронно, поэтому response перемещаем в область
        // кучи
        auto content_type = response.at(http::field::content_type);
        LogWrite(response.result_int(), std::string_view(content_type.data(), content_type.size()));

        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response, [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
            self->OnWrite(safe_response->need_eof(), ec, bytes_written);
        });
    }

   private:
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

    // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
    std::chrono::time_point<std::chrono::system_clock> _last_received_time_point;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
    net::io_context& ioc_;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
   public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler, net::io_context& ioc_)
        : SessionBase(std::move(socket), ioc_), request_handler_(std::forward<Handler>(request_handler))  {}

   private:
    std::shared_ptr<SessionBase> GetSharedThis() override { return this->shared_from_this(); }
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            std::visit([self](auto&& arg) { self->Write(std::move(arg)); }, std::move(response));
        });
    }
    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
   public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), request_handler_(std::forward<Handler>(request_handler)) {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() { DoAccept(); }

   private:
    void DoAccept() { acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this())); }

    void OnAccept(sys::error_code ec, tcp::socket socket) {
        using namespace std::literals;

        if (ec) {
            return ReportError(ec, "accept"sv);
        }

        AsyncRunSession(std::move(socket));
        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket) { std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_, ioc_)->Run(); }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
};

}  // namespace http_server