#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

#include "json_loader.h"
#include "logger.h"

namespace http_server {
void ReportError(beast::error_code ec, std::string_view what) {
    BOOST_LOG_TRIVIAL(info) << "error"
                            << logging::add_value(additional_data,
                                                  json_loader::CreateTrivialJson({"code", "text", "where"}, ec.value(), ec.what(), what));
}
void SessionBase::Run() { net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis())); }
void SessionBase::Read() {
    using namespace std::literals;
    request_ = {};
    stream_.expires_after(30s);
    http::async_read(stream_, buffer_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}
void SessionBase::LogRead() {
    BOOST_LOG_TRIVIAL(info) << "request received"
                            << logging::add_value(
                                   additional_data,
                                   json_loader::CreateTrivialJson({"ip", "URI", "method"}, stream_.socket().remote_endpoint().address().to_string(),
                                                                  request_.target(), request_.method_string()));
}
void SessionBase::LogWrite(int status_code, std::string_view content_type) {
    BOOST_LOG_TRIVIAL(info)
        << "response sent"
        << logging::add_value(
               additional_data,
               json_loader::CreateTrivialJson(
                   {"response_time", "code", "content_type"},
                   std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() - _last_received_time_point)).count(),
                   status_code, content_type));
}
void SessionBase::OnRead(beast::error_code ec, std::size_t bytes_read) {
    using namespace std::literals;

    if (ec == http::error::end_of_stream) {
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }

    _last_received_time_point = std::chrono::system_clock::now();
    LogRead();

    HandleRequest(std::move(request_));
}
void SessionBase::Close() {
    try {
        stream_.socket().shutdown(tcp::socket::shutdown_send);
    }catch(...) {}
}
void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }
    if (close) {
        return Close();
    }
    Read();
}
}  // namespace http_server