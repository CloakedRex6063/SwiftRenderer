add_executable(Example)
target_sources(Example PRIVATE Source/main.cpp)

include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG eb3fc0684c9957de108df5b338307418b0130c3b  
)
set(SDL_WASAPI OFF)
set(SDL_VIRTUAL_JOYSTICK OFF)
set(SDL_RENDER_GPU OFF)
set(SDL_RENDER_VULKAN OFF)
set(SDL_RENDER_D3D11 OFF)
set(SDL_RENDER_D3D12 OFF)
set(SDL_RENDER_D3D OFF)
set(SDL_OFFSCREEN OFF)
set(SDL_OPENGL OFF)
set(SDL_OPENGLES OFF)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)

FetchContent_MakeAvailable(SDL3)

target_link_libraries(Example PRIVATE SwiftRender)
target_link_libraries(Example PRIVATE SDL3::SDL3-static)