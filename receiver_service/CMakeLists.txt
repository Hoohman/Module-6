add_library(receiver_service STATIC src/receiver.cpp)
target_include_directories(receiver_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(receiver_service PUBLIC
    Boost::json
    Boost::headers)

add_executable(receiver src/main.cpp)
target_link_libraries(receiver PRIVATE receiver_service)

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(receiver PRIVATE ws2_32 wsock32)
endif()