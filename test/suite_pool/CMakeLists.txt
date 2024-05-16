add_executable(suite_pool suite_pool.c)

target_link_libraries(suite_pool ${TARGET_NAME}_FIFO Unity)

target_include_directories(suite_pool PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)

add_test(suite_pool suite_pool)
