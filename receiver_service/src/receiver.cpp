#include "receiver.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

using boost::asio::ip::tcp;

boost::asio::awaitable<void> session(tcp::socket socket) {
    try {
        std::string received_string;
        auto dyn_buf = boost::asio::dynamic_buffer(received_string);

        for (;;) {
            auto [ec, length] = co_await boost::asio::async_read_until(
                socket, dyn_buf, "\n", boost::asio::as_tuple(boost::asio::use_awaitable));

            if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                std::cout << "Client disconnected cleanly.\n";
                break;
            }

            if (ec) {
                throw boost::system::system_error(ec);
            }

            std::string result = received_string.substr(0, length);
            dyn_buf.consume(length);

            try {
                boost::json::value parsed = boost::json::parse(result);
                std::cout << "Received: " << parsed << '\n';

            } catch (const std::exception& e) {
                std::cerr << "Incorrect message format. Error code: "
                          << e.what() << '\n';
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << '\n';
    }
}

boost::asio::awaitable<void> listener(tcp::acceptor acceptor) {
    for (;;) {
        tcp::socket socket =
            co_await acceptor.async_accept(boost::asio::use_awaitable);

        boost::asio::co_spawn(acceptor.get_executor(),
                              session(std::move(socket)),
                              boost::asio::detached);
    }
}