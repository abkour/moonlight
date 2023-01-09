# CMakeList.txt : CMake project for moonlight, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (moonlight WIN32
	"application.cpp"
	"command_queue.cpp"
	"gameplay_system.cpp"
	"moonlight.cpp" 
	"swap_chain.cpp"
	"window.cpp" 
	"dx12_wrapper.cpp" 
	"demos/cube_application/cube_application.cpp"
	"demos/basic_sphere/basic_sphere.cpp"
)

if (CMAKE_VERSION VERSION_GREATER 3.13)
  set_property(TARGET moonlight PROPERTY CXX_STANDARD 20)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

  target_link_libraries(moonlight
	d3dcompiler.lib    
    d3d12.lib 
    dxgi.lib
	dxguid.lib
  )

  add_compile_definitions(ROOT_DIRECTORY_ASCII="${CMAKE_SOURCE_DIR}")
endif()

# TODO: Add tests and install targets if needed.

# Compile HLSL shaders
#add_custom_target(shaders)

#set(HLSL_SHADER_FILES 
#	"demos/shaders/cube_vs.hlsl" 
#	"demos/shaders/cube_ps.hlsl")

#set_source_files_properties("demos/shaders/cube_vs.hlsl" PROPERTIES ShaderType "vs")
#set_source_files_properties("demos/shaders/cube_ps.hlsl" PROPERTIES ShaderType "ps")
#set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "4_0")

#foreach(FILE ${HLSL_SHADER_FILES})
#	get_filename_component(FILE_WE ${FILE} NAME_WE)
#	get_source_file_property(shadertype ${FILE} ShaderType)
#	get_source_file_property(shadermodel ${FILE} ShaderModel)
#	add_custom_command(
#		TARGET shaders
#		COMMAND fxc.exe /nologo /Emain /T${shadertype}_{shadermodel} $<IF:$<CONFIG:DEBUG>, /Od, /O1> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
#		MAIN_DEPENDENCY ${FILE}
#		COMMENT "HLSL ${FILE}"
#		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#		VERBATIM
#	)
#endforeach(FILE)
#
#add_dependencies(moonlight shaders)
