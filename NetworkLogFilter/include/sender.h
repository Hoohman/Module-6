#pragma once

#include "log_message.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <string>

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv,
                const LogMessage& message);

class NetworkClient {
public:
    NetworkClient(const std::string& ip, const std::string& port);

    bool connect();
    bool send_message(const std::string& source, const std::string& payload);
    void disconnect();

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::string ip_;
    std::string port_;
};