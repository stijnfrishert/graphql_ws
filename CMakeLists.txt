cmake_minimum_required(VERSION 3.1)

project(graphql_ws VERSION 0.1.0 LANGUAGES CXX)

add_library(graphql_ws
    ${CMAKE_CURRENT_SOURCE_DIR}/src/connection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/socket.cpp
)
add_library(stijnfrishert::graphql_ws ALIAS graphql_ws)

target_compile_features(graphql_ws PUBLIC cxx_std_17)

target_include_directories(graphql_ws
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(graphql_ws
PUBLIC
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
)