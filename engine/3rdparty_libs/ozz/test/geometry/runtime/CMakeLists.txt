# skinning_job_tests
add_executable(test_skinning_job
  skinning_job_tests.cc)
target_link_libraries(test_skinning_job
  ozz_geometry
  ozz_base
  gtest)
target_copy_shared_libraries(test_skinning_job)
set_target_properties(test_skinning_job PROPERTIES FOLDER "NauEngine/3rdparty/ozz/tests/geometry")
add_test(NAME test_skinning_job COMMAND test_skinning_job)

# ozz_geometry fuse tests
set_source_files_properties(${PROJECT_BINARY_DIR}/src_fused/ozz_geometry.cc PROPERTIES GENERATED 1)
add_executable(test_fuse_geometry
  skinning_job_tests.cc
  ${PROJECT_BINARY_DIR}/src_fused/ozz_geometry.cc)
add_dependencies(test_fuse_geometry BUILD_FUSE_ozz_geometry)
target_link_libraries(test_fuse_geometry
  ozz_base
  gtest)
#target_copy_shared_libraries(test_fuse_geometry)
add_test(NAME test_fuse_geometry COMMAND test_fuse_geometry)
set_target_properties(test_fuse_geometry PROPERTIES FOLDER "NauEngine/3rdparty/ozz/tests/geometry")
target_compile_definitions(test_fuse_geometry PRIVATE $<$<BOOL:${BUILD_SHARED_LIBS}>:OZZ_BUILD_GEOMETRY_LIB>)