file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.mesh")
find_program(GLSL_VALIDATOR glslangValidator REQUIRED)
message(STATUS "Gathering shaders")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    get_filename_component(FILE_EXT ${GLSL} EXT)
    set(SPIRV "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILE_NAME}.spv")
    if(${FILE_EXT} STREQUAL ".mesh")
        set(SHADER_STAGE "-fshader-stage=mesh")
    else ()
        set(SHADER_STAGE "")
    endif()
    message(STATUS ${GLSL})
    message(STATUS COMMAND "" ${GLSL_VALIDATOR} -V " " ${SHADER_STAGE} " " ${GLSL} " " --target-env spirv1.6 " " -o " " ${SPIRV} -gVS)
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -gVS
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()
add_custom_target(SwiftCompileShaders DEPENDS ${SPIRV_BINARY_FILES})