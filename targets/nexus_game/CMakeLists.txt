# Define the Nexus_Game executable
add_executable(Nexus_Game
    "src/main.cpp"
    "src/nxsg_renderer.cpp"
    "src/nxsg_text.cpp"
    "src/nxsg_window.cpp"
)

# Include directories
target_include_directories(Nexus_Game PRIVATE
    "include"
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_ttf_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(Nexus_Game PRIVATE
    Nexus
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_ttf::SDL2_ttf
)

add_custom_command(TARGET Nexus_Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    "$<TARGET_FILE_DIR:Nexus_Game>/resources"
)

# Copy SDL libraries post-build
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/CopySDL.cmake)