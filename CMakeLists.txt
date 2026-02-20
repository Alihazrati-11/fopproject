cmake_minimum_required(VERSION 4.0)
project(scrach)

set(CMAKE_CXX_STANDARD 20)

add_executable(scrach main.cpp)

target_link_libraries(${PROJECT_NAME} -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer)

configure_file(calibrib.ttf calibrib.ttf COPYONLY)

