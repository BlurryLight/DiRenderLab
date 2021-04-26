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
file(GLOB SHADERS *.vs *.fs *.vert *.frag)
file(COPY ${SHADERS}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(SOURCE_LIST shadow_mapping_base.cpp GLwrapper/globject.cc GLwrapper/globject.hh)
set(HEADER_LIST GLwrapper/glsupport.hh GLwrapper/shader.cc GLwrapper/shader.hh GLwrapper/global.hh GLwrapper/program.cc GLwrapper/program.hh GLwrapper/vertex_buffer.cc GLwrapper/vertex_buffer.hh GLwrapper/vertex_array.cpp GLwrapper/vertex_array.h GLwrapper/texture.cc GLwrapper/texture.hh GLwrapper/glsupport.cc GLwrapper/shapes.cc GLwrapper/shapes.hh)
file(GLOB IMGUI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/*")
file(GLOB THIRD_PARTY
        third_party/*.c third_party/*.cc third_party/*.h utils/*.cc utils/*.h
        third_party/glad/*
        )
message(${THIRD_PARTY})
include_directories("third_party")

add_executable(${PROJECT_NAME} ${HEADER_LIST} ${THIRD_PARTY} ${IMGUI_SOURCE} ${SOURCE_LIST})
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        ${CMAKE_DL_LIBS}
        spdlog::spdlog
        )
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            assimp::assimp
            )
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE
            assimp
            )
endif ()

