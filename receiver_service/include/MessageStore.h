#pragma once

#include "IncomingMessage.h"
#include <pqxx/pqxx>
#include <string>

class MessageStore {
public:
    MessageStore(pqxx::connection& conn) : conn_(conn) {}

    bool store(const IncomingMessage& message);
    void print_stats();
    void initialize_schema(const std::string& path);

private:
    pqxx::connection& conn_;
};