#include "MessageStore.h"
#include "IncomingMessage.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

bool MessageStore::store(const IncomingMessage& message) {
    pqxx::work tx(conn_);

    auto result =
        tx.exec("INSERT INTO messages (message_id, source_service, "
                "timestamp_utc, payload) "
                "VALUES ($1, $2, $3, $4) "
                "ON CONFLICT (message_id) DO NOTHING "
                "RETURNING id",
                pqxx::params{message.message_id, message.source_service,
                             message.timestamp_utc, message.payload});

    const bool inserted = !result.empty();
    tx.commit();
    return inserted;
}

void MessageStore::print_stats() {
    pqxx::read_transaction tx(conn_);

    auto result = tx.exec("SELECT source_service, COUNT(*) AS message_count "
                          "FROM messages "
                          "GROUP BY source_service "
                          "ORDER BY source_service;");

    uint64_t messagesCount = 0;
    for (const auto& row : result) {
        auto [source, count] = row.as<std::string, uint64_t>();
        messagesCount += count;
    }
    std::cout << "Total messages: " << messagesCount << '\n';

    for (const auto& row : result) {
        auto [source, count] = row.as<std::string, uint64_t>();
        std::cout << "Source: " << source << "| Messages: " << count << '\n';
    }

    tx.commit();
}

void MessageStore::initialize_schema(const std::string& path) {
    std::ifstream file(path);
    std::ostringstream sql;

    if (!file || !(sql << file.rdbuf())) {
        throw std::runtime_error("Cannot read schema file: " + path);
    }

    pqxx::work tx(conn_);
    tx.exec(sql.str());
    tx.commit();
}