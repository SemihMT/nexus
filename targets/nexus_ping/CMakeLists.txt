add_executable(nexus_ping
    "src/main.cpp"
)
target_include_directories(nexus_ping PRIVATE
    "include"
)
target_link_libraries(nexus_ping PRIVATE
    Nexus
)