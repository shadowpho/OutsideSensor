cmake_minimum_required(VERSION 3.10)

project(OutsideSensor)
add_compile_options("-std=c++20")
add_compile_options("-ggdb3")
find_package(Threads)
find_package(SQLite3)
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)
file(GLOB SOURCE *.cpp BME680/*.c)
file(GLOB_RECURSE BSEC_SOURCE BSEC/*.c)
file(GLOB_RECURSE OLED_WAVESHARE_SOURCE OLED_Waveshare/lib/*.c)

include_directories(${SQLITE3_INCLUDE_DIRS})
include_directories(OLED_Waveshare/lib/Config)
include_directories(OLED_Waveshare/lib/Fonts)
include_directories(OLED_Waveshare/lib/GUI)
include_directories(OLED_Waveshare/lib/OLED)



add_executable(OutsideSensor ${SOURCE} ${OLED_WAVESHARE_SOURCE} ${BSEC_SOURCE})

target_link_libraries(OutsideSensor PRIVATE Threads::Threads)
target_link_libraries(OutsideSensor PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(OutsideSensor PRIVATE ${SQLite3_LIBRARIES})
target_link_libraries(OutsideSensor PRIVATE ${PQXX_LIB} ${PQ_LIB})
target_link_libraries(OutsideSensor PRIVATE bcm2835)
#target_link_libraries(OutsideSensor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/BSEC/libalgobsec.a)
target_compile_definitions(OutsideSensor PRIVATE -DUSE_BCM2835_LIB)
target_compile_definitions(OutsideSensor PRIVATE -DDEBUG)