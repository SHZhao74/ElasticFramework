cmake_minimum_required(VERSION 2.6)
project(NEWSCL_CL)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -Wall -L/opt/AMDAPP/lib/x86_64/")
add_definitions(-DGET_AMOUNT_TRANSMIT_DATA_)
set(cn_src ../src/cn/cn.cc ../src/cn/VM.cc ../src/cn/VMAFS_N_Scheduler.cc
    ../src/cn/cn.cc ../src/cn/command.cc ../src/cn/common.cc
    ../src/cn/handler.cc ../src/common/IO_server.cc  ../src/common/IO_client.cc
		../src/common/IO.cc ../src/cn/cn_info.cc)
set(cn_dep_lib pthread OpenCL GL GLU glut)

set(dispatcher_src ../src/dispatcher/dispatcher.cc ../src/common/IO.cc 
    ../src/common/IO_server.cc ../src/dispatcher/VMAFS_N_Dispatcher.cc
    ../src/dispatcher/cn.cc ../src/dispatcher/handler.cc ../src/dispatcher/Scoring.cc)
set(dispatcher_dep_lib pthread)
include_directories(../include/ ../../../src/ ../../../Demos/OpenGL/)


add_executable(cn ${cn_src} )
target_link_libraries(cn ${cn_dep_lib}   ${cn_dep_lib}) 


add_executable(dispatcher ${dispatcher_src})
target_link_libraries(dispatcher ${dispatcher_dep_lib})
