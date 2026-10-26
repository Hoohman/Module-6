#pragma once

#include "log_message.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/uuid.hpp>
#include <string>

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv,
                const LogMessage& message);

class NetworkClient {
public:
    NetworkClient(boost::asio::io_context& io_context, const std::string& ip,
                  const std::string& port);

    boost::asio::awaitable<bool> connect();
    boost::asio::awaitable<bool> send_message(std::string source,
                                              std::string payload);
    void disconnect();

private:
    bool validate_ack(const boost::json::value& root, const std::string& id);
    boost::asio::awaitable<bool> exchange_once(std::string request_data,
                                               std::string message_id);

    static constexpr std::size_t kMaxAttempts = 3;
    boost::uuids::random_generator id_generator_;
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::string ip_;
    std::string port_;
};