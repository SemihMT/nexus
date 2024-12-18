# Define the Nexus static library
add_library(
    Nexus
    STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/nexus.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus/nexus.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/client.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus/client.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/server.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus/server.cpp"
    
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus/connection.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nexus/connection.cpp"

)

# Include directories for Nexus
target_include_directories(
    Nexus
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/nexus"
    "${asio_SOURCE_DIR}/asio/include"
)

# Compile definitions for Nexus
target_compile_definitions(
    Nexus
    PUBLIC
    ASIO_STANDALONE
)

# Link Windows-specific libraries
if(WIN32)
    target_link_libraries(
        Nexus
        PUBLIC
        wsock32
        ws2_32
    )
endif()
