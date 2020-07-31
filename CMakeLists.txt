cmake_minimum_required(VERSION 3.16)
project(zfs_exporter)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_package(ZLIB)
find_package(prometheus-cpp CONFIG REQUIRED)

pkg_check_modules(LIBZFS REQUIRED IMPORTED_TARGET libzfs)

add_executable(zfs_exporter main.cpp)

target_link_libraries(zfs_exporter PUBLIC
        PkgConfig::LIBZFS
        PRIVATE prometheus-cpp::pull
        z)
