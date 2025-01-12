add_executable(enet_throughput
    "src/main.cpp"
)
target_include_directories(enet_throughput PUBLIC
    "include"
    "${enet_SOURCE_DIR}/include"
)

target_link_libraries(enet_throughput PUBLIC
    enet
)

if(WIN32)
    target_link_libraries(
        enet_throughput
        PUBLIC
        wsock32
        ws2_32
        winmm
    )
endif()