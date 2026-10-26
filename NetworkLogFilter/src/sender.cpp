#include "sender.h"
#include "log_message.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/uuid.hpp>
#include <chrono>
#include <format>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

static constexpr std::chrono::seconds kMaxTimeLimit{5};

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv,
                const LogMessage& message) {
    std::string time =
        std::format("{:%Y-%m-%d %H:%M:%S}", message.timestamp_utc);

    jv = {{"message_id", message.message_id},
          {"source_service", message.source_service},
          {"timestamp_utc", time},
          {"payload", message.payload}};
}

NetworkClient::NetworkClient(boost::asio::io_context& io_context,
                             const std::string& ip, const std::string& port)
    : io_context_(io_context), socket_(io_context_), ip_(ip), port_(port) {}

boost::asio::awaitable<bool> NetworkClient::connect() {
    tcp::resolver resolver(io_context_);

    auto [ec_resolver, endpoints] = co_await resolver.async_resolve(
        ip_, port_, boost::asio::as_tuple(boost::asio::use_awaitable));

    if (ec_resolver) {
        std::cerr << "Resolver error: " << ec_resolver.message() << '\n';
        co_return false;
    }

    auto [ec_connect, _] = co_await boost::asio::async_connect(
        socket_, endpoints,
        boost::asio::as_tuple(boost::asio::cancel_after(
            kMaxTimeLimit, boost::asio::use_awaitable)));

    if (ec_connect) {
        std::cerr << "Unable to connect (" << ip_ << ":" << port_
                  << "): " << ec_connect.message() << '\n';
        co_return false;
    }

    std::cout << "Successfully connected (" << ip_ << ":" << port_ << ")\n";
    co_return true;
}

boost::asio::awaitable<bool> NetworkClient::send_message(std::string source,
                                                         std::string payload) {
    auto message_id = boost::uuids::to_string(id_generator_());
    LogMessage message{message_id, source, std::chrono::system_clock::now(),
                       payload};
    boost::json::value jv(boost::json::value_from(message));
    std::string request_data = boost::json::serialize(jv) + "\n";
    boost::asio::steady_timer retry_timer(io_context_);

    for (std::size_t attempt = 0; attempt < kMaxAttempts; attempt++) {
        bool connected = socket_.is_open();

        if (!connected) {
            connected = co_await connect();
        }

        if (connected &&
            (co_await exchange_once(request_data, message.message_id))) {
            co_return true;
        }

        disconnect();

        if (attempt + 1 < kMaxAttempts) {
            retry_timer.expires_after(std::chrono::milliseconds(1500));
            co_await retry_timer.async_wait(boost::asio::use_awaitable);
        }
    }

    co_return false;
}

void NetworkClient::disconnect() {
    if (socket_.is_open()) {
        socket_.close();
    }
}

boost::asio::awaitable<bool>
NetworkClient::exchange_once(std::string request_data, std::string message_id) {
    std::string received_string;
    auto dyn_buf = boost::asio::dynamic_buffer(received_string, 1024);

    auto [ec_write, bytes_written] = co_await boost::asio::async_write(
        socket_, boost::asio::buffer(request_data),
        boost::asio::as_tuple(boost::asio::cancel_after(
            kMaxTimeLimit, boost::asio::use_awaitable)));
    if (ec_write) {
        std::cerr << "Error sending data: " << ec_write.message() << '\n';
        co_return false;
    }

    auto [ec_read, bytes_read] = co_await boost::asio::async_read_until(
        socket_, dyn_buf, '\n',
        boost::asio::as_tuple(boost::asio::cancel_after(
            kMaxTimeLimit, boost::asio::use_awaitable)));
    if (ec_read) {
        std::cerr << "Error reading ACK: " << ec_read.message() << '\n';
        co_return false;
    }
    std::string result = received_string.substr(0, bytes_read);
    dyn_buf.consume(bytes_read);

    boost::system::error_code parse_ec;
    auto ack = boost::json::parse(result, parse_ec);
    if (parse_ec) {
        std::cerr << "Invalid ACK JSON: " << parse_ec.message() << '\n';
        co_return false;
    }
    if (!validate_ack(ack, message_id)) {
        std::cerr << "Invalid ACK: unexpected type or message_id\n";
        co_return false;
    }

    co_return true;
}

bool NetworkClient::validate_ack(const boost::json::value& root,
                                 const std::string& id) {
    if (!root.is_object()) {
        return false;
    }

    const auto& obj = root.get_object();
    const auto* value_type = obj.if_contains("type");
    if (!value_type || !value_type->is_string()) {
        return false;
    }
    if (value_type->get_string() != "ack") {
        return false;
    }

    const auto& value_id = obj.if_contains("message_id");
    if (!value_id || !value_id->is_string()) {
        return false;
    }

    return value_id->get_string() == id;
}