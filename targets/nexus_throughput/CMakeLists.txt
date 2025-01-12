add_executable(nexus_throughput
    "src/main.cpp"
)
target_include_directories(nexus_throughput PRIVATE
    "include"
)
target_link_libraries(nexus_throughput PRIVATE
    Nexus
)