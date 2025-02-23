cmake_minimum_required(VERSION 3.24)
project("simple-server" LANGUAGES CXX)

if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a build directory and run CMake from there.")
endif ()
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Make downloading of packages quiet")

# Options working for GCC and CLang. MSVC will require a special treatment or a CMake preset.
set(COMMON_COMPILE_OPTIONS
        "-Wall"
        "-Wextra"
        "-Werror"
        "$<$<CONFIG:Release>:-O3>"
        "$<$<CONFIG:Debug>:-O0>"
)

# Main application
add_executable(server
        source/main.cpp
        include/TcpServer.hpp
        include/PacketParser.hpp
        include/PacketPrinter.hpp
        source/PacketPrinter.cpp
        include/Params.hpp
        source/Params.cpp
)
target_include_directories(server PRIVATE include)
target_compile_options(server PRIVATE ${COMMON_COMPILE_OPTIONS})
set_target_properties(server PROPERTIES CXX_STANDARD 20)

# Downloadable dependencies
include(cmake/CPM.cmake)

set(BOOST_INCLUDE_LIBRARIES "asio" "crc" "program_options")
CPMAddPackage(
        NAME Boost
        URL "https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz"
        OPTIONS "BOOST_ENABLE_CMAKE ON"
)
target_link_libraries(server PRIVATE Boost::asio Boost::crc Boost::program_options)
