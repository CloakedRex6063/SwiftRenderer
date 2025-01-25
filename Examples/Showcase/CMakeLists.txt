add_executable(Showcase Source/App.cpp)
set(SHADER_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "")
target_link_libraries(Showcase PUBLIC SwiftRender fastgltf glfw Utility)