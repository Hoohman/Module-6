#pragma once

#include <string>

struct IncomingMessage {
    std::string message_id;
    std::string source_service;
    std::string timestamp_utc;
    std::string payload;
};
