add_executable(nexus_latency
    "src/main.cpp"
)
target_include_directories(nexus_latency PRIVATE
    "include"
)
target_link_libraries(nexus_latency PRIVATE
    Nexus
)