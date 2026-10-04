#pragma once

#include <chrono>
#include <string>

struct LogMessage {
    std::string source_service;
    std::chrono::system_clock::time_point timestamp_utc;
    std::string payload;
};