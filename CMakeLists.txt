cmake_minimum_required(VERSION 3.28)

# Define project
# Version sets variables: Nexus_VERSION, PROJECT_VERSION
project(Nexus 
        VERSION 1.0.0
        DESCRIPTION "The Nexus API is a high-level game networking API built on top of ASIO using modern C++"
        LANGUAGES CXX)


# Include the fetchcontent package to get packages 
include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/")
# Get the ASIO library from the gtihub repo ###################################
FetchContent_Declare(
        asio                                                    # target name
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio    # repo link
        GIT_TAG asio-1-30-2                                     # tag to get 1.32.2 is latests stable version
)

# Populate variables:
#       asio_SOURCE_DIR
#       asio_BINARY_DIR
#       asio_POPULATED
#       asio_GIT_TAG
#       asio_GIT_REPOSITORY
#       asio
FetchContent_MakeAvailable(asio)

message(NOTICE "ASIO Populated?: " ${asio_POPULATED})
message(NOTICE "ASIO source DIR: " ${asio_SOURCE_DIR})
message(NOTICE "ASIO binary DIR: " ${asio_BINARY_DIR})

#########################################################################################

# Fetch SDL2 ############################################################################
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

#########################################################################################


# This project will be a static lib
# Maybe can be a header only library for ease of use, similar to RayLib
# This line creates the "Nexus" target
add_library(
        Nexus   #Library Target name
        STATIC  #Library type
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/nexus.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus/nexus.cpp"
)
target_include_directories(
        Nexus
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus"
        "${asio_SOURCE_DIR}/asio/include"
)
target_compile_definitions(
        Nexus
        PUBLIC
        ASIO_STANDALONE
)

if(WIN32)
        target_link_libraries(
                Nexus
                PUBLIC
                wsock32
                ws2_32
        )
endif()




# Create the exe
add_executable(
        Nexus_Game 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus_game/main.cpp"
)
target_include_directories(
        Nexus_Game
        PRIVATE
        "${SDL2_SOURCE_DIR}/include"
)

target_link_libraries(
        Nexus_Game
        PRIVATE
        Nexus
        SDL2::SDL2
        SDL2::SDL2main
)
