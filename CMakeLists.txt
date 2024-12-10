cmake_minimum_required(VERSION 3.22.1)

project(SQLiteTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(Boost_USE_MULTITHREADED ON)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(CURL)


include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
include_directories(headers)

add_executable(test src/SQL/TgSQL.cpp)
add_executable(TgBot src/TgBot/TgBot.cpp)
add_library(BaseSQL STATIC src/SQL/BaseSQL.cpp headers/SQL/BaseSQL.h)

if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

target_link_libraries(TgBot /usr/local/lib/libTgBot.a
 ${CMAKE_THREAD_LIBS_INIT}
 ${OPENSSL_LIBRARIES}
 ${Boost_LIBRARIES}
 ${CURL_LIBRARIES}
)
target_link_libraries(BaseSQL PRIVATE SQLite::SQLite3)
target_link_libraries(test PRIVATE SQLite::SQLite3 BaseSQL)
