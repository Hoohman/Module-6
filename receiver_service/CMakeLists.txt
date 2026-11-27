find_package(libpqxx 7.10 CONFIG QUIET)
if(NOT TARGET libpqxx::pqxx)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx>=7.10)
    add_library(libpqxx::pqxx ALIAS PkgConfig::PQXX)
endif()
add_library(receiver_service STATIC src/receiver.cpp src/database/MessageStore.cpp)
target_include_directories(receiver_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(receiver_service PUBLIC
    Boost::json
    Boost::headers
    libpqxx::pqxx)

add_executable(receiver src/main.cpp)
target_link_libraries(receiver PRIVATE receiver_service)

if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(receiver PRIVATE ws2_32 wsock32)
endif()
