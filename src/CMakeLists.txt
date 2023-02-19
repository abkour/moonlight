# CMakeList.txt : CMake project for moonlight, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (moonlight WIN32
	"application.cpp"
	"moonlight.cpp" 
	"window.cpp" 
	"demos/00_cube_application/cube_application.cpp"
	"demos/01_frustum_culling/frustum_culling.cpp"
	"camera.cpp" 
	"core/render_texture.cpp" 
	"core/dx12_resource.cpp"  
	"utility/arena_allocator.cpp"
 "core/descriptor_heap.cpp" "utility/glyph_renderer.cpp")

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
