cmake_minimum_required(VERSION 3.12)
project(cfgdb C)

set(CMAKE_C_STANDARD 99)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

# Add libraries
ExternalProject_Add(zmq
        URL https://github.com/zeromq/libzmq/archive/master.zip
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/zmq
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        )
ExternalProject_Add(cJSON
        URL https://github.com/DaveGamble/cJSON/archive/master.zip
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/cJSON
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
                   -DCMAKE_C_STANDARD:=99
        )

# Setup dependencies
add_library(dependencies STATIC IMPORTED)
add_dependencies(dependencies PUBLIC zmq cJSON)

ExternalProject_Get_Property(cJSON install_dir)
include_directories(${install_dir}/include)
set_property(TARGET dependencies PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libcjson.dylib)
set(CJSON_DIR "${install_dir}")

ExternalProject_Get_Property(zmq install_dir)
include_directories(${install_dir}/include)
set_property(TARGET dependencies PROPERTY IMPORTED_LOCATION ${install_dir}/lib/libzmq.a)
set(ZMQ_DIR "${install_dir}")

# Project build
add_library(cfgdb STATIC cfgdb.c cfgdb.h hashtable.c hashtable.h)
target_link_libraries(cfgdb dependencies)
