# Include the FetchContent module
include(FetchContent)

# Fetch ASIO
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Fetch SDL2
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
)
FetchContent_MakeAvailable(SDL2)

# Fetch SDL2_ttf
FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.22.0
)
FetchContent_MakeAvailable(SDL2_ttf)

# Output debug messages
message(NOTICE "ASIO Populated?: ${asio_POPULATED}")
message(NOTICE "SDL2 Populated?: ${SDL2_POPULATED}")
