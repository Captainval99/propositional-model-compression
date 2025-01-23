cmake_minimum_required(VERSION 3.20)

project(modelCompression VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#include libarchive
find_package(LibArchive REQUIRED)

#incude boost library
find_package(Boost 1.70 REQUIRED COMPONENTS iostreams)

#include lz4 library
find_package(PkgConfig REQUIRED)
pkg_check_modules(lz4 REQUIRED IMPORTED_TARGET liblz4)

add_executable(compression src/CompressionMain.cpp)
# Include directories for libarchive header files
target_include_directories(compression PRIVATE ${LibArchive_INCLUDE_DIRS})

# Link against the libarchive library
target_link_libraries(compression PRIVATE archive)
target_link_libraries(compression PRIVATE Boost::iostreams)
target_link_libraries(compression PRIVATE PkgConfig::lz4)

target_include_directories(compression PRIVATE "src/parser")
target_include_directories(compression PRIVATE "src/propagation")
target_include_directories(compression PRIVATE "src/util")

add_executable(decompression src/DecompressionMain.cpp)
# Include directories for libarchive header files
target_include_directories(decompression PRIVATE ${LibArchive_INCLUDE_DIRS})

# Link against the libraries
target_link_libraries(decompression PRIVATE archive)
target_link_libraries(decompression PRIVATE Boost::iostreams)
target_link_libraries(decompression PRIVATE PkgConfig::lz4)

target_include_directories(decompression PRIVATE "src/parser")
target_include_directories(decompression PRIVATE "src/propagation")
target_include_directories(decompression PRIVATE "src/util")
