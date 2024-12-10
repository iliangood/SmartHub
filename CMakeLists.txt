cmake_minimum_required(VERSION 3.22.1)

project(SQLiteTest)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SQLite3 REQUIRED)

include_directories(headers)

add_executable(test src/SQL/TgSQL.cpp)

add_library(BaseSQL STATIC src/SQL/BaseSQL.cpp headers/SQL/BaseSQL.h)

target_link_libraries(BaseSQL PRIVATE SQLite::SQLite3)

target_link_libraries(test PRIVATE SQLite::SQLite3 BaseSQL)
