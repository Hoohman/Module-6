#include "sender.h"
#include "log_message.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <format>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv,
                const LogMessage& message) {
    std::string time = std::format("{:%Y-%m-%d %H:%M:%S}", message.timestamp_utc);

    jv = {{"source_service", message.source_service},
          {"timestamp_utc", time},
          {"payload", message.payload}};
}

NetworkClient::NetworkClient(const std::string& ip, const std::string& port)
    : socket_(io_context_), ip_(ip), port_(port) {}

bool NetworkClient::connect() {
    boost::system::error_code ec;

    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(ip_, port_, ec);
    if (ec) {
        std::cerr << "Resolver error: " << ec.message() << '\n';
        return false;
    }

    boost::asio::connect(socket_, endpoints, ec);
    if (ec) {
        std::cerr << "Unable to connect (" << ip_ << ":" << port_
                  << "): " << ec.message() << '\n';
        return false;
    }

    std::cout << "Successfully connected (" << ip_ << ":" << port_ << ")\n";
    return true;
}

bool NetworkClient::send_message(const std::string& source,
                                 const std::string& payload) {
    if (!socket_.is_open()) {
        std::cerr << "Error: socket is not opened\n";
        return false;
    }

    LogMessage message{source, std::chrono::system_clock::now(), payload};
    boost::json::value jv(boost::json::value_from(message));
    std::string request_data = boost::json::serialize(jv) + "\n";

    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request_data), ec);
    if (ec) {
        std::cerr << "Error sending data: " << ec.message() << '\n';
        disconnect();
        return false;
    }

    return true;
}

void NetworkClient::disconnect() {
    if (socket_.is_open()) {
        socket_.close();
    }
}