cmake_minimum_required(VERSION 3.23)

project(Project)

set(CMAKE_CXX_STANDARD 20)

add_executable(Project main.cpp)

target_link_libraries(Project
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)

configure_file(calibrib.ttf calibrib.ttf COPYONLY)
