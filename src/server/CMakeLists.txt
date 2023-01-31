aux_source_directory(. SRC_LISTS)

aux_source_directory(./db DB_LISTS)

aux_source_directory(./model MODEL_LISTS)

add_executable(ChatServer ${SRC_LISTS} ${DB_LISTS} ${MODEL_LISTS})
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread)