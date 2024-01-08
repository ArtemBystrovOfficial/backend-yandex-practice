#pragma once

#include <boost/asio.hpp>
#include <iostream>

#include "audio.h"

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

static constexpr auto kRecordSeconds = 1.5s;
static constexpr auto kBufferLength = 65000;
#define MA_FORMAT_U8_1 ma_format_u8, 1

void dumpChar(const char* str, int size) {
    for (int i = 0; i < size; i++) std::cout << int(str[i]) << " ";
}

class Client {
   public:
    Client(net::io_context& context, const std::string_view& ip, int port)
        : m_recorder(MA_FORMAT_U8_1),
          m_context(context),
          m_socket(context, udp::v4()),
          m_end_point(net::ip::make_address(ip), port) {}

    void RecordAndSendPart() {
        auto result = m_recorder.Record(kBufferLength, kRecordSeconds);
        dumpChar(result.data.data(), result.data.size());
        m_socket.send_to(net::buffer(result.data.data(), result.data.size()),
                         m_end_point);
    }

   private:
    Recorder m_recorder;
    net::io_context& m_context;
    udp::socket m_socket;
    udp::endpoint m_end_point;
};

class Server {
   public:
    Server(net::io_context& context, int port)
        : m_player(MA_FORMAT_U8_1),
          m_context(context),
          m_socket(context, udp::endpoint(udp::v4(), port)) {}

    void ReceiveAndPlayPart() {
        std::array<char, kBufferLength> recv_buf;
        udp::endpoint remote_endpoint;
        m_socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);
        dumpChar(recv_buf.data(), recv_buf.size());
        m_player.PlayBuffer(recv_buf.data(), recv_buf.size(), kRecordSeconds);
    }

   private:
    Player m_player;
    net::io_context& m_context;
    udp::socket m_socket;
};