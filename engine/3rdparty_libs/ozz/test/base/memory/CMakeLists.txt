add_executable(test_memory
  allocator_tests.cc)
target_link_libraries(test_memory
  ozz_base
  gtest)
target_copy_shared_libraries(test_memory)
add_test(NAME test_memory COMMAND test_memory)
set_target_properties(test_memory PROPERTIES FOLDER "NauEngine/3rdparty/ozz/tests/base")

add_executable(test_unique_ptr
  unique_ptr_tests.cc)
target_link_libraries(test_unique_ptr
  ozz_base
  gtest)
target_copy_shared_libraries(test_unique_ptr)
add_test(NAME test_unique_ptr COMMAND test_unique_ptr)
set_target_properties(test_unique_ptr PROPERTIES FOLDER "NauEngine/3rdparty/ozz/tests/base")
