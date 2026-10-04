#pragma once

#include <boost/asio.hpp>

boost::asio::awaitable<void> session(boost::asio::ip::tcp::socket socket);

boost::asio::awaitable<void> listener(boost::asio::ip::tcp::acceptor acceptor);