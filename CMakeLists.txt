cmake_minimum_required(VERSION 3.10)

project(OutsideSensor)

find_package(Threads)
file(GLOB SOURCES *.cpp)

add_executable(OutsideSensor ${SOURCES})

target_link_libraries(OutsideSensor PRIVATE Threads::Threads)
