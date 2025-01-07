add_executable(enet_connect_disconnect
    "src/main.cpp"
)
target_include_directories(enet_connect_disconnect PUBLIC
    "include"
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(enet_connect_disconnect PUBLIC
    enet
)

if(WIN32)
    target_link_libraries(
        enet_connect_disconnect
        PUBLIC
        wsock32
        ws2_32
        winmm
    )
endif()