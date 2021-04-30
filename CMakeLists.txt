cmake_minimum_required(VERSION 3.12)
project(DiRenderLab C CXX)
set(CMAKE_CXX_STANDARD 17) #enable filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(${CMAKE_SOURCE_DIR}/cmake/cmake_filesystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
find_package(glm CONFIG REQUIRED)
find_package(assimp REQUIRED)
find_package(spdlog REQUIRED)
if (WIN32)
    find_package(glfw3 REQUIRED)
else ()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
endif (WIN32)
set(SOURCE_LIST GLwrapper/globject.cc GLwrapper/globject.hh)
set(HEADER_LIST GLwrapper/glsupport.hh GLwrapper/shader.cc GLwrapper/shader.hh GLwrapper/global.hh GLwrapper/program.cc GLwrapper/program.hh GLwrapper/vertex_buffer.cc GLwrapper/vertex_buffer.hh GLwrapper/vertex_array.cc GLwrapper/vertex_array.hh GLwrapper/texture.cc GLwrapper/texture.hh GLwrapper/glsupport.cc GLwrapper/shapes.cc GLwrapper/shapes.hh GLwrapper/framebuffer.cc GLwrapper/framebuffer.hh)
file(GLOB IMGUI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/*")
file(GLOB THIRD_PARTY
        third_party/*.c third_party/*.cc third_party/*.h utils/*.cc utils/*.h
        )
message(${THIRD_PARTY})
include_directories("third_party")
add_subdirectory(third_party/glad)

set(EXAMPLES
        imgui_example
        ssao
        shadowmap
        skybox
        )
foreach (EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} ${HEADER_LIST} ${THIRD_PARTY} ${IMGUI_SOURCE} ${SOURCE_LIST} examples/${EXAMPLE}/${EXAMPLE}.cc)
    target_include_directories(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(${EXAMPLE} PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(${EXAMPLE} PRIVATE
            glad
            glfw
            ${CMAKE_DL_LIBS}
            spdlog::spdlog
            )
    if (WIN32)
        target_link_libraries(${EXAMPLE} PRIVATE assimp::assimp)
        target_compile_options(${EXAMPLE} PRIVATE /utf-8)
    else ()
        target_link_libraries(${EXAMPLE} PRIVATE assimp)
    endif ()
endforeach ()


