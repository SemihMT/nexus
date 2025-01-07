add_executable(enet_ping
    "src/main.cpp"
)
target_include_directories(enet_ping PUBLIC
    "include"
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(enet_ping PUBLIC
    enet
)

if(WIN32)
    target_link_libraries(
        enet_ping
        PUBLIC
        wsock32
        ws2_32
        winmm
    )
endif()