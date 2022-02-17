cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("Computer graphics in Game development")

set(INCLUDE
        libs/stb
        libs/tinyobjloader
        libs/linalg
        libs/cxxopts/include
        libs/D3DX12
        src)

set(COMMON_SOURCES
        src/settings.cpp
        src/renderer/renderer.cpp
        src/world/camera.cpp
        src/world/model.cpp
        src/utils/resource_utils.cpp
	  src/renderer/renderer.h)

set(COMMON_HEADERS
        src/settings.h
        src/resource.h
        src/world/camera.h
        src/world/model.h
        src/utils/error_handler.h
        src/utils/resource_utils.h
        src/renderer/renderer.h)

set(Rasterization_SOURCES ${COMMON_SOURCES} src/main.cpp src/renderer/rasterizer/rasterizer_renderer.cpp)
set(Raytracing_SOURCES ${COMMON_SOURCES} src/main.cpp src/renderer/raytracer/raytracer_renderer.cpp)
set(DirectX12_SOURCES ${COMMON_SOURCES} src/win_main.cpp src/utils/window.cpp src/renderer/dx12/dx12_renderer.cpp)

set(Rasterization_HEADERS ${COMMON_HEADERS} src/renderer/rasterizer/rasterizer.h src/renderer/rasterizer/rasterizer_renderer.h)
set(Raytracing_HEADERS ${COMMON_HEADERS} src/renderer/raytracer/raytracer.h src/renderer/raytracer/raytracer_renderer.h)
set(DirectX12_HEADERS ${COMMON_HEADERS} src/utils/com_error_handler.h src/utils/window.h src/renderer/dx12/dx12_renderer.h)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(Rasterization ${Rasterization_HEADERS} ${Rasterization_SOURCES})
target_compile_definitions(Rasterization PUBLIC RASTERIZATION)
target_include_directories(Rasterization PRIVATE ${INCLUDE})
set_property(TARGET Rasterization PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(Raytracing ${Raytracing_HEADERS} ${Raytracing_SOURCES})
target_compile_definitions(Raytracing PUBLIC RAYTRACING)
target_include_directories(Raytracing PRIVATE ${INCLUDE})
set_property(TARGET Rasterization PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(DirectX12 WIN32 ${DirectX12_HEADERS} ${DirectX12_SOURCES})
target_compile_definitions(DirectX12 PUBLIC DX12 WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS _UNICODE UNICODE)
target_include_directories(DirectX12 PRIVATE ${INCLUDE})
target_link_libraries(DirectX12 d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib)
# Copy shader as a source to the binary directory
configure_file(shaders/shaders.hlsl ${CMAKE_CURRENT_BINARY_DIR}/shaders.hlsl COPYONLY)
set_target_properties(Rasterization PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(Raytracing PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(DirectX12 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
