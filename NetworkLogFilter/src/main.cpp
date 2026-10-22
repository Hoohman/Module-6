#include "log_filter.h"
#include "sender.h"
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

boost::asio::awaitable<bool> run_client(NetworkClient& client,
                                        std::vector<std::string> addresses) {

    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::steady_timer retry_timer(executor);

    bool connected = false;

    for (std::size_t attempt = 0; attempt < 3; attempt++) {
        if (co_await client.connect()) {
            connected = true;
            break;
        }

        client.disconnect();

        if (attempt + 1 < 3) {
            retry_timer.expires_after(std::chrono::milliseconds(1500));
            co_await retry_timer.async_wait(boost::asio::use_awaitable);
        }
    }

    if (!connected) {
        for (const auto& address : addresses) {
            std::cout << address << '\n';
        }
        co_return false;
    }

    for (const auto& address : addresses) {
        if (!(co_await client.send_message("ipv4_filter", address))) {
            client.disconnect();
            co_return false;
        }
    }

    client.disconnect();
    co_return true;
}

int main() {
    std::cout << "Starting IPv4 Filter Service...\n";

    boost::asio::io_context io_context;
    NetworkClient client(io_context, "127.0.0.1", "30303");

    std::string input_data = "192.168.0.5 - GET /\n"
                             "10.0.0.1 - POST /login\n"
                             "192.168.1.256 - INVALID\n"
                             "invalid.ip - CORRUPTED\n"
                             " - EMPTY\n"
                             "192.168.0.200 - GET /api\n"
                             "192.168.0.50 - GET /\n"
                             "192.168.0.31 - GET /\n"
                             "192.168.0.42 - GET /\n"
                             "192.168.0.156 - GET /\n";

    std::istringstream input(input_data);
    std::ostringstream output;

    auto filter = create_filter({{"subnet", "192.168.0.0/24"}});
    process_stream(input, output, *filter);

    std::vector<std::string> addresses;
    std::string address;
    std::istringstream parse_input(output.str());

    while (std::getline(parse_input, address)) {
        addresses.push_back(address);
    }

    try {
        auto result = boost::asio::co_spawn(
            io_context, run_client(client, std::move(addresses)),
            boost::asio::use_future);

        io_context.run();
        return result.get() ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Sender error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}