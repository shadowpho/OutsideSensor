cmake_minimum_required(VERSION 3.10)

project(OutsideSensor)
add_compile_options("-std=c++20")
find_package(Threads)
find_package(SQLite3)
file(GLOB SOURCES *.cpp)

include_directories(${SQLITE3_INCLUDE_DIRS})

add_executable(OutsideSensor ${SOURCES})

target_link_libraries(OutsideSensor PRIVATE Threads::Threads)
target_link_libraries(OutsideSensor PRIVATE ${SQLite3_LIBRARIES})