cmake_minimum_required(VERSION 3.28)
 
# Define the project
project(Nexus 
        VERSION 1.0.0
        DESCRIPTION "The Nexus API is a high-level game networking API built on ASIO" 
        LANGUAGES CXX)

# Set the fetch content base directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")

# Include additional configuration files
include(cmake/Dependencies.cmake)    # External dependencies
include(cmake/NexusLibrary.cmake)    # Nexus static library
include(cmake/NexusGame.cmake)       # Nexus_Game executable
