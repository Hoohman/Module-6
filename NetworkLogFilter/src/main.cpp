#include "log_filter.h"
#include "sender.h"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

int main() {
    std::cout << "Starting IPv4 Filter Service...\n";

    NetworkClient client("127.0.0.1", "30303");
    bool connected = true;
    for (std::size_t i = 0; i < 3; i++) {
        if (client.connect()) {
            connected = true;
            break;
        }
        connected = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

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

    if (connected) {
        for (const auto& address : addresses) {
            if (!client.send_message("ipv4_filter", address)) {
                break;
            }
        }
        client.disconnect();
    } else {
        for (const auto& address : addresses) {
            std::cout << address << '\n';
        }
    }

    return 0;
}