cmake_minimum_required(VERSION 3.0)
project(chat)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
# include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
# include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
# include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
# include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
# link_directories(/usr/lib64/mysql)

#配置库文件搜索路径几个系统就有的不需要配置
# 但是你可以把依赖的库，放进本项目，然后指定路径
# link_libraries(/usr/local/lib)
# link_directories(/usr/lib)

# 加载子目录
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)