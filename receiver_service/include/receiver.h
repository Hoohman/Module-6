#pragma once

#include "messageIdCache.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <optional>
#include <string>

std::optional<std::string> validate_message(const boost::json::value& root);

boost::asio::awaitable<bool> send_ack(boost::asio::ip::tcp::socket& socket,
                                      std::string message_id);

boost::asio::awaitable<void> session(boost::asio::ip::tcp::socket socket,
                                     MessageIdCache& processed_ids);

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor,
                                      MessageIdCache& processed_ids);