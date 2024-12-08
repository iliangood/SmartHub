cmake_minimum_required(VERSION 3.22.1)

project(SQLiteTest)

find_package(SQLite3 REQUIRED)

add_executable(test TgSQL.cpp)

target_link_libraries(test PRIVATE SQLite::SQLite3)
