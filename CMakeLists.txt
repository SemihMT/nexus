cmake_minimum_required(VERSION 3.28)

# Define project
# Version sets variables: Nexus_VERSION, PROJECT_VERSION
project(Nexus 
        VERSION 1.0.0
        DESCRIPTION "The Nexus API is a high-level game networking API built on top of ASIO using modern C++"
        LANGUAGES CXX)




# Include the fetchcontent package to get packages 
include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")
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


set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
# Fetch SDL2 ############################################################################
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
)

FetchContent_MakeAvailable(SDL2)


# Set output directory for SDL2 binaries only
# if(TARGET SDL2)
#     set_target_properties(SDL2 PROPERTIES
#         RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#         LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#         ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#     )
# endif()

# if(TARGET SDL2main)
#     set_target_properties(SDL2main PROPERTIES
#         RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#         LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#         ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl2-build/${CMAKE_BUILD_TYPE}"
#     )
# endif()

# Output SDL2 paths for verification
message(NOTICE "SDL2 Source DIR: " ${SDL2_SOURCE_DIR})
message(NOTICE "SDL2 Binary DIR: " ${SDL2_BINARY_DIR})

if(EXISTS "${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
        message("${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE} exists")
else()
message("${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE} does not exist")
endif()
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
)

target_link_libraries(
        Nexus_Game
        PRIVATE
        SDL2::SDL2main
        SDL2::SDL2
)



if(WIN32)
        if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
                message("Copying in Release")
                add_custom_command(TARGET Nexus_Game POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/SDL2.dll"
                        $<TARGET_FILE_DIR:Nexus_Game>/SDL2.dll
                )
        endif()
        if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
                message("Copying in Debug")
                add_custom_command(TARGET Nexus_Game POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/SDL2d.dll"
                        $<TARGET_FILE_DIR:Nexus_Game>/SDL2d.dll
                )
        endif()
endif()

# # For Linux: copy libSDL2.so or libSDL2d.so based on the build type
# if(UNIX AND NOT APPLE)
#     if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
#         add_custom_command(TARGET Nexus_Game POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL2_BINARY_DIR}/libSDL2.so"
#             $<TARGET_FILE_DIR:Nexus_Game>/libSDL2.so
#         )
#     endif()
#     if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#         add_custom_command(TARGET Nexus_Game POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL2_BINARY_DIR}/libSDL2d.so"
#             $<TARGET_FILE_DIR:Nexus_Game>/libSDL2d.so
#         )
#     endif()
# endif()

# # For macOS: copy libSDL2.dylib or libSDL2d.dylib based on the build type
# if(APPLE)
#     if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
#         add_custom_command(TARGET Nexus_Game POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL2_BINARY_DIR}/libSDL2.dylib"
#             $<TARGET_FILE_DIR:Nexus_Game>/libSDL2.dylib
#         )
#     endif()
#     if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#         add_custom_command(TARGET Nexus_Game POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             "${SDL2_BINARY_DIR}/libSDL2d.dylib"
#             $<TARGET_FILE_DIR:Nexus_Game>/libSDL2d.dylib
#         )
#     endif()
# endif()