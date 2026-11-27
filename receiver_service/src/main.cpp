#include "MessageStore.h"
#include "receiver.h"
#include <boost/asio.hpp>
#include <exception>
#include <iostream>
#include <pqxx/pqxx>
#include <utility>

using boost::asio::ip::tcp;

int main() {
    try {
        std::cout << std::unitbuf;
        pqxx::connection conn;
        MessageStore messageStore(conn);
        messageStore.initialize_schema("receiver_service/schema.sql");
        std::cout << "DB connected successfully!\n";

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 30303));
        std::cout << "Receiver launched on 30303 port\n";

        boost::asio::co_spawn(io_context,
                              listener(std::move(acceptor), messageStore),
                              boost::asio::detached);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Receiver error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}