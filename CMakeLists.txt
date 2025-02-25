cmake_minimum_required(VERSION 3.22.1)
project(SQLiteTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SQLite3 REQUIRED)

include_directories(headers)

add_library(BaseSQL STATIC src/SQL/BaseSQL.cpp headers/SQL/BaseSQL.h)
add_library(TgSQL STATIC src/SQL/TgSQL.cpp headers/SQL/TgSQL.h)
add_library(events STATIC src/events.cpp headers/events.h)

add_executable(main src/main.cpp)

target_link_libraries(BaseSQL PRIVATE SQLite::SQLite3)
target_link_libraries(TgSQL PRIVATE SQLite::SQLite3 BaseSQL)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE SQLite::SQLite3 BaseSQL TgSQL events)

# Добавляем поддержку тестов
enable_testing()
add_test(NAME RunTests COMMAND tests)

# Автоматический запуск тестов после сборки
add_custom_target(run_tests ALL
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS tests
    COMMENT "Running tests after build"
)
