# Determine the SDL library file to copy based on the platform
if(NOT WIN32 AND NOT APPLE AND NOT UNIX)
    message(FATAL_ERROR "Unsupported platform for SDL copy!")
endif()

# Copy SDL3 next to the executable — $<TARGET_FILE:...> resolves the correct
# built location on all platforms and generators without manual path construction.
add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:Nexus_Game>
)

add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:SDL3_ttf::SDL3_ttf>
        $<TARGET_FILE_DIR:Nexus_Game>
)

message(STATUS "Configured post-build step to copy SDL libraries")
