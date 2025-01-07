cmake_minimum_required(VERSION 3.28)
 
# Define the project
project(Nexus 
        VERSION 1.0.0
        DESCRIPTION "The Nexus API is a high-level game networking API built on ASIO" 
        LANGUAGES CXX)

# Set the fetch content base directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include additional configuration files
include(cmake/Dependencies.cmake)    # External dependencies

add_subdirectory(targets/nexus)
add_subdirectory(targets/nexus_game)
add_subdirectory(targets/nexus_connect_disconnect)
add_subdirectory(targets/nexus_ping)
add_subdirectory(targets/nexus_send_message)

add_subdirectory(targets/enet_connect_disconnect)
add_subdirectory(targets/enet_ping)
add_subdirectory(targets/enet_send_message)