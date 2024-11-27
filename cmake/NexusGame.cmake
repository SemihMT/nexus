# Glob all .cpp files from the src/nexus_game directory
file(GLOB NEXUS_GAME_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus_game/*.cpp"
)

# Define the Nexus_Game executable
add_executable(
    Nexus_Game
    ${NEXUS_GAME_SRC}
)

# Include directories for Nexus_Game
target_include_directories(
    Nexus_Game
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus_game"
    "${SDL2_SOURCE_DIR}/include"
)

# Link Nexus and SDL2 libraries
target_link_libraries(
    Nexus_Game
    PRIVATE
    Nexus
    SDL2::SDL2main
    SDL2::SDL2
)

# Include post-build commands
include(cmake/CopySDL.cmake)
