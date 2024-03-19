add_executable(suite_job_fifo suite_job_queue_common.c suite_job_queue_fifo.c)
add_executable(suite_job_priority suite_job_queue_common.c suite_job_queue_priority.c)

target_link_libraries(suite_job_fifo ${TARGET_NAME}_FIFO Unity)
target_link_libraries(suite_job_priority ${TARGET_NAME}_PRIORITY Unity)

target_include_directories(suite_job_fifo PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(suite_job_priority PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(suite_job_fifo PUBLIC -DSCHW_SCHED_FIFO)
target_compile_definitions(suite_job_priority PUBLIC -DSCHW_SCHED_PRIORITY)

add_test(suite_job_fifo suite_job_fifo)
add_test(suite_job_priority suite_job_priority)
