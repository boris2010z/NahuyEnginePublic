
add_custom_command(
  DEPENDS $<$<BOOL:${ozz_build_fbx}>:BUILD_DATA>
          "${CMAKE_CURRENT_LIST_DIR}/README.md"
          "${ozz_media_directory}/bin/pab_skeleton.ozz"
          "${ozz_media_directory}/bin/pab_walk.ozz"
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
          "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
          "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz"
  COMMAND ${CMAKE_COMMAND} -E make_directory media
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/README.md" .
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_skeleton.ozz" "./media/skeleton.ozz"
  COMMAND ${CMAKE_COMMAND} -E copy "${ozz_media_directory}/bin/pab_walk.ozz" "./media/animation.ozz"
  VERBATIM)

add_executable(sample_attach
  sample_attach.cc
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  "${CMAKE_CURRENT_BINARY_DIR}/media/skeleton.ozz"
  "${CMAKE_CURRENT_BINARY_DIR}/media/animation.ozz")

target_link_libraries(sample_attach
  sample_framework)
target_copy_shared_libraries(sample_attach)
set_target_properties(sample_attach
  PROPERTIES FOLDER "NauEngine/3rdparty/ozz/samples")

if(EMSCRIPTEN)
  # Resource files are embedded to the output file with emscripten
  set_target_properties(sample_attach
    PROPERTIES LINK_FLAGS "--embed-file media --embed-file README.md")

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.html
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.js
    ${CMAKE_CURRENT_BINARY_DIR}/sample_attach.wasm
    DESTINATION bin/samples/attach)
else()
  install(TARGETS sample_attach DESTINATION bin/samples/attach)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media DESTINATION bin/samples/attach)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md DESTINATION bin/samples/attach)
endif(EMSCRIPTEN)

add_test(NAME sample_attach COMMAND sample_attach "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_attach_load_path COMMAND sample_attach "--skeleton=media/skeleton.ozz" "--animation=media/animation.ozz" "--max_idle_loops=${ozz_sample_testing_loops}" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
add_test(NAME sample_attach_load_invalid_skeleton_path COMMAND sample_attach "--skeleton=media/bad_skeleton.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_attach_load_invalid_skeleton_path PROPERTIES WILL_FAIL true)
add_test(NAME sample_attach_load_invalid_animation_path COMMAND sample_attach "--animation=media/bad_animation.ozz" $<$<BOOL:${ozz_run_tests_headless}>:--norender>)
set_tests_properties(sample_attach_load_invalid_animation_path PROPERTIES WILL_FAIL true)
