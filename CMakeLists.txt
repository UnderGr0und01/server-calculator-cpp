cmake_minimum_required(VERSION 3.22.1)
project(server_calculator_cpp)
include_directories(/pqxx/pqxx)

set(CMAKE_CXX_STANDARD 14)

add_executable(server_calculator_cpp
        main.cpp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})
target_link_libraries(server_calculator_cpp ${PQXX_LIBRARIES})