#pragma once

#include "IncomingMessage.h"
#include "MessageStore.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <optional>
#include <pqxx/pqxx>
#include <string>

IncomingMessage tag_invoke(boost::json::value_to_tag<IncomingMessage>,
                           const boost::json::value& jv);

std::optional<std::string> validate_message(const boost::json::value& root);

boost::asio::awaitable<bool> send_ack(boost::asio::ip::tcp::socket& socket,
                                      std::string message_id);

boost::asio::awaitable<void> session(boost::asio::ip::tcp::socket socket,
                                     MessageStore& messageStore);

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor,
                                      MessageStore& messageStore);