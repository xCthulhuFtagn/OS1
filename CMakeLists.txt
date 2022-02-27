cmake_minimum_required(VERSION 3.0)

#set(PROJECT_VERSION 0.0.1)
project(OS1)# VERSION ${PROJECT_VERSION})

add_executable(main main.cpp)

set_target_properties(
    main PROPERTIES
    CXX_VERSION 11
    CXX_VERSION_REQUIREMENTS ON
)
