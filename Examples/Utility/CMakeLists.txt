FILE(GLOB_RECURSE SOURCES Source/*.cpp)
add_library(Utility ${SOURCES})
target_include_directories(Utility PUBLIC Include ${CMAKE_SOURCE_DIR}/External/GLFW/include)
target_link_libraries(Utility PUBLIC SwiftRender glfw fastgltf::fastgltf)
