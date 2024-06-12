cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_DEBUG_POSTFIX d)

set(TARGET_GROUP lib CACHE STRING
  "Group to build. Possible values are 'test', 'lib', and 'example'.")
set(LIB_TYPE STATIC CACHE STRING
  "Library type. Possible values are 'STATIC' and 'SHARED'.")
set(TARGET_NAME "schwimmbad")

set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${TARGET_NAME} VERSION 1.0.0)

# Include directory
set(INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.c")
add_compile_options(-Wall -Wextra -Wpedantic -Wundef -Wunreachable-code)

if(TARGET_GROUP STREQUAL lib)
  add_compile_options(-O2)

  add_library(${TARGET_NAME} ${LIB_TYPE} ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
elseif(TARGET_GROUP STREQUAL test)
  include(CTest)
  add_subdirectory(external)
  add_subdirectory(test)

  add_library(${TARGET_NAME}_FIFO STATIC ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME}_FIFO PUBLIC ${INCLUDE_DIRECTORIES})
  add_library(${TARGET_NAME}_PRIORITY STATIC ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME}_PRIORITY PUBLIC ${INCLUDE_DIRECTORIES})
elseif(TARGET_GROUP STREQUAL example)
  add_library(${TARGET_NAME} STATIC ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
  target_compile_definitions(${TARGET_NAME} PUBLIC SCHW_SCHED_FIFO)
  add_subdirectory(example)
else()
  message(FATAL_ERROR "Invalid TARGET_GROUP: ${TARGET_GROUP}")
endif()
