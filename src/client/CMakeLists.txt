

add_executable(client clientMain.cpp)
target_link_libraries(client pthread)

add_executable(login_test test_login.cpp)
target_link_libraries(login_test pthread)