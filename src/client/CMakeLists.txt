aux_source_directory(. SRC_LISTS)

add_executable(client ${SRC_LISTS})
target_link_libraries(client pthread)