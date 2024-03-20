add_executable(suite_pool suite_pool.c)

target_link_libraries(suite_pool ${TARGET_NAME}_FIFO Unity)

target_include_directories(suite_pool PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(suite_pool PUBLIC -DSCHW_SCHED_FIFO)

add_test(suite_pool suite_pool)
