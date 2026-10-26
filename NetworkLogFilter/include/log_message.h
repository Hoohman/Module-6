#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

struct LogMessage {
    std::string message_id;
    std::string source_service;
    std::chrono::system_clock::time_point timestamp_utc;
    std::string payload;
};