add_library(log_filter_lib STATIC src/log_filter.cpp)
target_include_directories(log_filter_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(NetworkLogFilter src/main.cpp src/sender.cpp)
target_link_libraries(NetworkLogFilter PRIVATE
    log_filter_lib
    Boost::json
    Boost::headers)

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(NetworkLogFilter PRIVATE ws2_32 wsock32)
endif()