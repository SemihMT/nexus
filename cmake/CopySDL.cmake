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
    # Search for the libSDL2.so file using a find command
    execute_process(
        COMMAND find ${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE} -name "libSDL2*.so"
        RESULT_VARIABLE FIND_RESULT
        OUTPUT_VARIABLE FIND_OUTPUT
        ERROR_VARIABLE FIND_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Check if the find command was successful
    if(FIND_RESULT EQUAL 0 AND FIND_OUTPUT)
        string(REGEX REPLACE "^.*" "" SDL_SHARED_LIB ${FIND_OUTPUT})
    else()
        message(FATAL_ERROR "SDL2 shared library not found in ${SDL2_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
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
