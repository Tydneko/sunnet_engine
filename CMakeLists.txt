project(sunnet_engine)

cmake_minimum_required (VERSION 2.8)

#头文件目录
include_directories(include)

#查找./src目录下所有源文件，保存到DIR_SRCS中
aux_source_directory(./src DIR_SRCS)

#C++11
add_definitions(-std=c++11)

#指定生成目标文件
add_executable(sunnet ${DIR_SRCS})

#库文件
find_package(Threads)
target_link_libraries (sunnet ${CMAKE_THREAD_LIBS_INIT})