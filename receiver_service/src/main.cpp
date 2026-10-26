#include "messageIdCache.h"
#include "receiver.h"
#include <boost/asio.hpp>
#include <exception>
#include <iostream>
#include <utility>


using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 30303));
        MessageIdCache processed_ids;
        std::cout << "Receiver launched on 30303 port\n";

        boost::asio::co_spawn(io_context,
                              listener(std::move(acceptor), processed_ids),
                              boost::asio::detached);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Receiver error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}