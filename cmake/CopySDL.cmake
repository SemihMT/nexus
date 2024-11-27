# Determine the SDL library file to copy based on the platform
if(WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(SDL_SHARED_LIB "SDL2.dll")
    elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(SDL_SHARED_LIB "SDL2d.dll")
    endif()
elseif(APPLE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(SDL_SHARED_LIB "libSDL2-2.0.dylib")
    elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(SDL_SHARED_LIB "libSDL2-2.0d.dylib")
    endif()
elseif(UNIX)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(SDL_SHARED_LIB "libSDL2-2.0.so")
    elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(SDL_SHARED_LIB "libSDL2-2.0d.so")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform for SDL copy!")
endif()

# Debugging: Check if the SDL library exists in the expected location
message(STATUS "SDL Library to copy: ${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${SDL_SHARED_LIB}")

# Check if the SDL library exists
if(EXISTS "${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${SDL_SHARED_LIB}")
    message(STATUS "SDL library exists at: ${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${SDL_SHARED_LIB}")
else()
    message(WARNING "SDL library does not exist at: ${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${SDL_SHARED_LIB}")
endif()

# Add a post-build command to copy the appropriate SDL library
if(DEFINED SDL_SHARED_LIB)
    
    add_custom_command(TARGET Nexus_Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${SDL_SHARED_LIB}"
        $<TARGET_FILE_DIR:Nexus_Game>/${SDL_SHARED_LIB}
    )
    message("Configured post-build step to copy SDL library: ${SDL_SHARED_LIB}")
endif()
