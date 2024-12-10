cmake_minimum_required(VERSION 3.22.1)

project(SQLiteTest)

find_package(SQLite3 REQUIRED)

add_executable(test TgSQL.cpp)

add_library(BaseSQL STATIC BaseSQL.cpp BaseSQL.h)

target_link_libraries(BaseSQL PRIVATE SQLite::SQLite3)

target_link_libraries(test PRIVATE SQLite::SQLite3 BaseSQL)
