# CMakeList.txt : CMake project for moonlight, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (moonlight WIN32
	"application.cpp"
	"command_queue.cpp"
	"gameplay_system.cpp"
	"moonlight.cpp" 
	"rendering_system.cpp"
	"swap_chain.cpp"
	"window.cpp" 
)

if (CMAKE_VERSION VERSION_GREATER 3.13)
  set_property(TARGET moonlight PROPERTY CXX_STANDARD 20)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

  target_link_libraries(moonlight
	d3dcompiler.lib    
    d3d12.lib 
    dxgi.lib
  )
endif()

# TODO: Add tests and install targets if needed.
