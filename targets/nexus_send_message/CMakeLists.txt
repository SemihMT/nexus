add_executable(nexus_send_message
    "src/main.cpp"
)
target_include_directories(nexus_send_message PRIVATE
    "include"
)
target_link_libraries(nexus_send_message PRIVATE
    Nexus
)