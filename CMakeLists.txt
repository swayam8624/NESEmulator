cmake_minimum_required(VERSION 3.16)

project(NESEmulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE BREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_executable(game
        NESEmulator/testing2C02.cpp
        NESEmulator/Bus.cpp
        NESEmulator/cpu6502.cpp
        NESEmulator/Cartridge.cpp
        NESEmulator/Mapper.cpp
        NESEmulator/Mapper_000.cpp
        NESEmulator/ppu2C02.cpp

)

target_include_directories(game PRIVATE
        ${BREW_PREFIX}/include
        NESEmulator
)

target_link_directories(game PRIVATE
        ${BREW_PREFIX}/lib
)

find_library(GLUT_FRAMEWORK GLUT REQUIRED)
find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
find_package(PNG REQUIRED)

target_link_libraries(game PRIVATE
        PNG::PNG
        ${GLUT_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${COCOA_FRAMEWORK}
)