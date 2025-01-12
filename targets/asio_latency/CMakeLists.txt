add_executable(asio_latency
    "src/main.cpp"
)
target_include_directories(asio_latency PRIVATE
    "include"
)
target_link_libraries(asio_latency PRIVATE
    Nexus
)