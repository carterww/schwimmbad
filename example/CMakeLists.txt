add_executable(example example.c)

target_link_libraries(example ${TARGET_NAME})
target_compile_definitions(example PUBLIC)
