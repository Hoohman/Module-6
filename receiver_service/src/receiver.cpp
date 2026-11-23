#include "receiver.h"
#include "IncomingMessage.h"
#include "MessageStore.h"
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <utility>

using boost::asio::ip::tcp;
static constexpr std::size_t kMaxMessageSize = 1024;
static constexpr std::chrono::seconds kMaxTimeLimit{10};

IncomingMessage tag_invoke(boost::json::value_to_tag<IncomingMessage>,
                           const boost::json::value& jv) {
    const auto& obj = jv.as_object();

    IncomingMessage im;
    im.message_id = boost::json::value_to<std::string>(obj.at("message_id"));
    im.source_service =
        boost::json::value_to<std::string>(obj.at("source_service"));
    im.timestamp_utc =
        boost::json::value_to<std::string>(obj.at("timestamp_utc"));
    im.payload = boost::json::value_to<std::string>(obj.at("payload"));

    return im;
}

std::optional<std::string> validate_message(const boost::json::value& root) {
    if (!root.is_object()) {
        return "JSON root must be an object";
    }

    const auto& obj = root.get_object();

    struct FieldRule {
        const char* name;
        std::size_t max_length;
    };

    static constexpr FieldRule rules[] = {
        {"message_id", 36},
        {"source_service", 32},
        {"timestamp_utc", 32},
        {"payload", 768},
    };

    for (const auto& rule : rules) {
        const auto* value = obj.if_contains(rule.name);
        const std::string field = rule.name;

        if (!value) {
            return "Missing field '" + field + "'";
        }

        if (!value->is_string()) {
            return "Field '" + field + "' must be a string";
        }

        if (value->get_string().size() > rule.max_length) {
            return "Field '" + field + "' is too long";
        }
    }

    const auto& message_id = obj.at("message_id").get_string();
    if (message_id.size() != 36) {
        return "Field 'message_id' must be a UUID in 8-4-4-4-12 format";
    }

    for (std::size_t i = 0; i < message_id.size(); ++i) {
        const char c = message_id[i];
        const bool is_separator = i == 8 || i == 13 || i == 18 || i == 23;
        const bool valid_character =
            is_separator ? c == '-'
                         : (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                               (c >= 'A' && c <= 'F');

        if (!valid_character) {
            return "Field 'message_id' must be a UUID in 8-4-4-4-12 format";
        }
    }

    return std::nullopt;
}

boost::asio::awaitable<bool> send_ack(tcp::socket& socket,
                                      std::string message_id) {
    boost::json::object ack{{"type", "ack"}, {"message_id", message_id}};
    std::string response = boost::json::serialize(ack) + '\n';

    auto [ec, bytes_written] = co_await boost::asio::async_write(
        socket, boost::asio::buffer(response),
        boost::asio::as_tuple(boost::asio::cancel_after(
            kMaxTimeLimit, boost::asio::use_awaitable)));

    if (ec) {
        std::cerr << "Error sending ACK: " << ec.message() << '\n';
        co_return false;
    }

    co_return true;
}

boost::asio::awaitable<void> session(tcp::socket socket,
                                     MessageStore& messageStore) {
    try {
        std::string received_string;
        auto dyn_buf =
            boost::asio::dynamic_buffer(received_string, kMaxMessageSize);

        for (;;) {
            auto [ec, length] = co_await boost::asio::async_read_until(
                socket, dyn_buf, "\n",
                boost::asio::as_tuple(boost::asio::cancel_after(
                    kMaxTimeLimit, boost::asio::use_awaitable)));

            if (ec == boost::asio::error::eof ||
                ec == boost::asio::error::connection_reset) {
                std::cout << "Client disconnected cleanly.\n";
                break;
            }

            if (ec == boost::asio::error::operation_aborted) {
                std::cerr << "Session timeout\n";
                break;
            }

            if (ec == boost::asio::error::not_found) {
                std::cerr << "Message is too long\n";
                break;
            }

            if (ec) {
                throw boost::system::system_error(ec);
            }

            std::string result = received_string.substr(0, length);
            dyn_buf.consume(length);

            boost::json::value parsed;
            IncomingMessage message;
            try {
                parsed = boost::json::parse(result);
                if (auto error = validate_message(parsed)) {
                    std::cerr << "Protocol error: " << *error << '\n';
                    continue;
                }
                message = boost::json::value_to<IncomingMessage>(parsed);
            } catch (const std::exception& e) {
                std::cerr << "Incorrect message format. Error code: "
                          << e.what() << '\n';
                continue;
            }

            if (message.source_service == "control") {
                if (message.payload == "stats") {
                    messageStore.print_stats();
                    if (!(co_await send_ack(socket, message.message_id))) {
                        break;
                    }
                    continue;
                }
                std::cout << "Unknown control command\n";
                continue;
            }

            try {
                if (messageStore.store(message)) {
                    std::cout << "[DB] Stored message id=" << message.message_id
                              << '\n';
                }
            } catch (const std::exception& e) {
                std::cerr << "[DB] Error storing message: " << e.what() << '\n';
                break;
            }

            if (!(co_await send_ack(socket, message.message_id))) {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << '\n';
    }
}

boost::asio::awaitable<void> listener(tcp::acceptor acceptor,
                                      MessageStore& messageStore) {
    for (;;) {
        tcp::socket socket =
            co_await acceptor.async_accept(boost::asio::use_awaitable);

        boost::asio::co_spawn(acceptor.get_executor(),
                              session(std::move(socket), messageStore),
                              boost::asio::detached);
    }
}
