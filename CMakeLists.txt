cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_DEBUG_POSTFIX d)

set(TARGET_GROUP test CACHE STRING "Group to build")
set(SCHW_SCHED fifo CACHE STRING "Scheduler to use")
set(TARGET_NAME "schwimmbad")

set(BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(TEST_FIFO_DIR ${BUILD_DIR}/test_fifo)
set(TEST_PRIORITY_DIR ${BUILD_DIR}/test_priority)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${TARGET_NAME} VERSION 0.1.0)

# Include directory
set(INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.c")

if(TARGET_GROUP STREQUAL production)
  if(SCHW_SCHED STREQUAL fifo)
    target_compile_definitions(${TARGET_NAME} PUBLIC SCHW_SCHED_FIFO)
  elseif(SCHW_SCHED STREQUAL priority)
    target_compile_definitions(${TARGET_NAME} PUBLIC SCHW_SCHED_PRIORITY)
  else()
    message(FATAL_ERROR "Invalid SCHW_SCHEDULER: ${SCHW_SCHEDULER}")
  endif()
  add_library(${TARGET_NAME} STATIC ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
elseif(TARGET_GROUP STREQUAL test)
  include(CTest)
  add_subdirectory(external)
  add_subdirectory(test)

  # This is bad, figure out how to do this properly
  # Building libraries with different compile definitions
  add_library(${TARGET_NAME}_FIFO STATIC ${SOURCE_FILES})
  set_target_properties(${TARGET_NAME}_FIFO PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR}/${TEST_FIFO_DIR})
  target_include_directories(${TARGET_NAME}_FIFO PUBLIC ${INCLUDE_DIRECTORIES})
  target_compile_definitions(${TARGET_NAME}_FIFO PUBLIC SCHW_SCHED_FIFO)

  add_library(${TARGET_NAME}_PRIORITY STATIC ${SOURCE_FILES})
  set_target_properties(${TARGET_NAME}_PRIORITY PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR}/${TEST_PRIORITY_DIR})
  target_include_directories(${TARGET_NAME}_PRIORITY PUBLIC ${INCLUDE_DIRECTORIES})
  target_compile_definitions(${TARGET_NAME}_PRIORITY PUBLIC SCHW_SCHED_PRIORITY)
elseif(TARGET_GROUP STREQUAL example)
  add_library(${TARGET_NAME} STATIC ${SOURCE_FILES})
  target_include_directories(${TARGET_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
  target_compile_definitions(${TARGET_NAME} PUBLIC SCHW_SCHED_FIFO)
  add_subdirectory(example)
else()
  message(FATAL_ERROR "Invalid TARGET_GROUP: ${TARGET_GROUP}")
endif()
