# Determine the SDL library file to copy based on the platform
if(WIN32)
    set(SDL_SHARED_LIB      "$<IF:$<CONFIG:Debug>,SDL3d.dll,SDL3.dll>")
    set(SDL_TTF_SHARED_LIB  "$<IF:$<CONFIG:Debug>,SDL3_ttfd.dll,SDL3_ttf.dll>")
elseif(APPLE)
    set(SDL_SHARED_LIB      "$<IF:$<CONFIG:Debug>,libSDL3-2.0d.dylib,libSDL3-2.0.dylib>")
    set(SDL_TTF_SHARED_LIB  "$<IF:$<CONFIG:Debug>,libSDL3_ttf-2.0d.dylib,libSDL3_ttf-2.0.dylib>")
elseif(UNIX)
    set(SDL_SHARED_LIB      "$<IF:$<CONFIG:Debug>,libSDL3-2.0d.so,libSDL3-2.0.so>")
    set(SDL_TTF_SHARED_LIB  "$<IF:$<CONFIG:Debug>,libSDL3_ttf-2.0d.so,libSDL3_ttf-2.0.so>")
else()
    message(FATAL_ERROR "Unsupported platform for SDL copy!")
endif()

# Debugging: these will print the genex literally at configure time, which is expected
message(STATUS "SDL Library to copy: ${SDL3_BINARY_DIR}/$<CONFIG>/${SDL_SHARED_LIB}")

# Check if the SDL library exists (build-time echo since genex can't be used in if(EXISTS ...))
add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "SDL source path: ${SDL3_BINARY_DIR}/$<CONFIG>/${SDL_SHARED_LIB}"
)

# Unix-specific: copy from flat build dir into config subdir first
if(UNIX)
    add_custom_command(TARGET Nexus_Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SDL3_BINARY_DIR}/${SDL_SHARED_LIB}"
            "${SDL3_BINARY_DIR}/$<CONFIG>/${SDL_SHARED_LIB}"
    )
    add_custom_command(TARGET Nexus_Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${SDL3_ttf_BINARY_DIR}/${SDL_TTF_SHARED_LIB}"
            "${SDL3_ttf_BINARY_DIR}/$<CONFIG>/${SDL_TTF_SHARED_LIB}"
    )
endif()

# Copy SDL3 to output dir
add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:Nexus_Game>
)

# Copy SDL3_ttf to output dir
add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:SDL3_ttf::SDL3_ttf>
        $<TARGET_FILE_DIR:Nexus_Game>
)

message(STATUS "Configured post-build step to copy SDL libraries")
