if(NOT ozz_build_gltf)
  return()
endif()

add_executable(gltf2ozz
  gltf2ozz.cc)

target_link_libraries(gltf2ozz PRIVATE
  ozz_animation_tools)
target_copy_shared_libraries(gltf2ozz)
set_target_properties(gltf2ozz
  PROPERTIES FOLDER "NauEngine/3rdparty/ozz/tools")

install(TARGETS gltf2ozz DESTINATION bin/tools)
