cmake_minimum_required(VERSION 3.16.0)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/led_strip)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(MMEBF)

#include(CMakeListsPrivate.txt)
#
#if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeListsUser.txt)
#include(CMakeListsUser.txt)
#endif()
#
#add_custom_target(
#    Production ALL
#    COMMAND platformio -c clion run "$<$<NOT:$<CONFIG:All>>:-e${CMAKE_BUILD_TYPE}>"
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)
#
#add_custom_target(
#    Debug ALL
#    COMMAND platformio -c clion debug "$<$<NOT:$<CONFIG:All>>:-e${CMAKE_BUILD_TYPE}>"
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)
#
#add_executable(Z_DUMMY_TARGET ${SRC_LIST})
