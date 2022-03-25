cmake_minimum_required(VERSION 3.10)

project(OutsideSensor)
add_compile_options("-std=c++20")
find_package(Threads)
find_package(SQLite3)
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)
file(GLOB SOURCES *.cpp BME680/*.cpp)


include_directories(${SQLITE3_INCLUDE_DIRS})

add_executable(OutsideSensor ${SOURCES})

target_link_libraries(OutsideSensor PRIVATE Threads::Threads)
target_link_libraries(OutsideSensor PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(OutsideSensor PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(OutsideSensor PRIVATE ${PQXX_LIB} ${PQ_LIB})