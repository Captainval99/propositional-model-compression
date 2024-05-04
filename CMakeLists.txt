cmake_minimum_required(VERSION 3.20)

project(modelCompression VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


include(FetchContent)
FetchContent_Declare(
    libarchive
    GIT_REPOSITORY https://github.com/libarchive/libarchive.git
    GIT_TAG v3.7.3
)

# Ensure that FetchContent is available
FetchContent_GetProperties(libarchive)
if(NOT libarchive_POPULATED)
  FetchContent_Populate(libarchive)
  add_subdirectory(${libarchive_SOURCE_DIR} ${libarchive_BINARY_DIR})
endif()


add_executable(compression src/CompressionMain.cpp)
# Include directories for libarchive header files
target_include_directories(compression PRIVATE ${LibArchive_INCLUDE_DIRS})

# Link against the libarchive library
target_link_libraries(compression PRIVATE archive)

target_include_directories(compression PRIVATE "src/parser")
target_include_directories(compression PRIVATE "src/propagation")
target_include_directories(compression PRIVATE "src/util")

add_executable(decompression src/DecompressionMain.cpp)
# Include directories for libarchive header files
target_include_directories(decompression PRIVATE ${LibArchive_INCLUDE_DIRS})

# Link against the libarchive library
target_link_libraries(decompression PRIVATE archive)

target_include_directories(decompression PRIVATE "src/parser")
target_include_directories(decompression PRIVATE "src/propagation")
target_include_directories(decompression PRIVATE "src/util")