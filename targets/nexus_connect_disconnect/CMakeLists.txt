add_executable(nexus_connect_disconnect
    "src/main.cpp"
)
target_include_directories(nexus_connect_disconnect PRIVATE
    "include"
)
target_link_libraries(nexus_connect_disconnect PRIVATE
    Nexus
)