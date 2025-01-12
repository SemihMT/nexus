add_executable(asio_throughput
    "src/main.cpp"
)
target_include_directories(asio_throughput PRIVATE
    "include"
)
target_link_libraries(asio_throughput PRIVATE
    Nexus
)