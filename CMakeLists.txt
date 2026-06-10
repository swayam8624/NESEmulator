cmake_minimum_required(VERSION 4.0)

project(NESEmulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE BREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_executable(game)

target_sources(game
        PRIVATE
        NESEmulator/main.cpp
        NESEmulator/Bus.cpp
        NESEmulator/cpu6502.cpp
        NESEmulator/ppu2C02.cpp
        NESEmulator/apu2A03.cpp
        NESEmulator/Cartridge.cpp
        NESEmulator/Mapper.cpp
        NESEmulator/Mapper_000.cpp
        NESEmulator/Mapper_001.cpp
        NESEmulator/Mapper_002.cpp
        NESEmulator/Mapper_003.cpp
        NESEmulator/Mapper_004.cpp
        NESEmulator/Mapper_066.cpp
        PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        NESEmulator/KairoNES.cppm
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

add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/NESEmulator/nestest.nes
        $<TARGET_FILE_DIR:game>/nestest.nes
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/NESEmulator/roms
        $<TARGET_FILE_DIR:game>/roms
)
