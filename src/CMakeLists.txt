# CMakeList.txt : CMake project for moonlight, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (moonlight WIN32
	"application.cpp"
	"moonlight.cpp" 
	"window.cpp" 
	"camera.cpp" 
	"demos/00_cube_application/cube_application.cpp"
	"demos/01_frustum_culling/frustum_culling.cpp"
	"demos/02_environment_mapping/environment_mapping.cpp"
	"core/render_texture.cpp" 
	"core/dx12_resource.cpp"  
	"core/command_queue.cpp" 
	"core/swap_chain.cpp"
	"core/descriptor_heap.cpp" 
	"utility/arena_allocator.cpp"
	"utility/glyph_renderer.cpp" 
 "demos/03_global_illumination/rtx_renderer.cpp" "demos/03_global_illumination/ray_camera.cpp" "math/ray.cpp" "core/texture2D.cpp" "utility/common.cpp" "utility/bvh.cpp" "math/aabb.cpp" "utility/random_number.cpp" "logging_file.cpp")

if (MSVC)
set_target_properties(${BUILD_TARGET} PROPERTIES LINK_FLAGS "/PROFILE")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG" CACHE INTERNAL "")

if (CMAKE_VERSION VERSION_GREATER 3.13)
  set_property(TARGET moonlight PROPERTY CXX_STANDARD 20)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

  target_link_libraries(moonlight
	d3dcompiler.lib    
    d3d12.lib 
    dxgi.lib
	dxguid.lib
  )

  target_link_libraries(moonlight debug "${CMAKE_SOURCE_DIR}/ext/DirectXTex/build/bin/CMake/Debug/DirectXTex.lib")
  target_link_libraries(moonlight optimized "${CMAKE_SOURCE_DIR}/ext/DirectXTex/build/bin/CMake/Release/DirectXTex.lib")

  target_link_libraries(moonlight debug "${CMAKE_SOURCE_DIR}/ext/DirectXTK12/build/bin/CMake/Debug/DirectXTK12.lib")
  target_link_libraries(moonlight optimized "${CMAKE_SOURCE_DIR}/ext/DirectXTK12/build/bin/CMake/Release/DirectXTK12.lib")

  add_compile_definitions(ROOT_DIRECTORY_ASCII="${CMAKE_SOURCE_DIR}")
endif()
