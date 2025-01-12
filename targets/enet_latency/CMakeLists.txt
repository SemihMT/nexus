add_executable(enet_latency
    "src/main.cpp"
)
target_include_directories(enet_latency PUBLIC
    "include"
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(enet_latency PUBLIC
    enet
)

if(WIN32)
    target_link_libraries(
        enet_latency
        PUBLIC
        wsock32
        ws2_32
        winmm
    )
endif()