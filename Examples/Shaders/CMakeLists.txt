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
    if(${FILE_EXT} STREQUAL ".mesh")
        set(SHADER_STAGE "-fshader-stage=mesh")
    else ()
        set(SHADER_STAGE "")
    endif()
    
    if (${FILE_NAME} STREQUAL "model.vert")
        message(STATUS ${GLSL})
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv")
        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -g
                DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
        message(STATUS "indirect_model.vert")
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/indirect_model.vert.spv")
        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -g -D="INDIRECT"
                DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    elseif (${FILE_NAME} STREQUAL "model.frag")
        message(STATUS ${GLSL})
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv")
        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -g
                DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
        message(STATUS "indirect_model.frag")
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/indirect_model.frag.spv")
        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -g -D="INDIRECT"
                DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    else()
        set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv")
        message(STATUS ${GLSL})
        add_custom_command(
                OUTPUT ${SPIRV}
                COMMAND ${GLSL_VALIDATOR} -V ${SHADER_STAGE} ${GLSL} --target-env spirv1.6 -o ${SPIRV} -g
                DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endif ()
endforeach()
add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})