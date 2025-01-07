add_executable(enet_send_message
    "src/main.cpp"
)
target_include_directories(enet_send_message PUBLIC
    "include"
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(enet_send_message PUBLIC
    enet
)

if(WIN32)
    target_link_libraries(
        enet_send_message
        PUBLIC
        wsock32
        ws2_32
        winmm
    )
endif()