cmake_minimum_required (VERSION 3.11.2)
project (Seeder)

set (SEEDER_VERSION_MAJOR 1)
set (SEEDER_VERSION_MINOR 0)

include_directories ("${PROJECT_SOURCE_DIR}")

add_executable(SeederClient seeder_client.cxx)
add_executable(SeederServer seeder_server.cxx)

