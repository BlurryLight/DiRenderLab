cmake_minimum_required(VERSION 3.12)
project(DiRenderLab C CXX)
set(CMAKE_CXX_STANDARD 17) #enable filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/cmake_filesystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/utils.cmake)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(spdlog REQUIRED)
if (DEFINED VCPKG_TARGET_TRIPLET) # we are using vcpkg
    message("VCPKG found!")
    find_package(glfw3 REQUIRED)
else ()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
endif (DEFINED VCPKG_TARGET_TRIPLET)


set(SOURCE_LIST GLwrapper/globject.cc GLwrapper/globject.hh GLwrapper/program.cc GLwrapper/texture.cc)
set(HEADER_LIST GLwrapper/glsupport.hh GLwrapper/shader.cc GLwrapper/shader.hh GLwrapper/global.hh GLwrapper/program.hh GLwrapper/vertex_buffer.cc GLwrapper/vertex_buffer.hh GLwrapper/vertex_array.cc GLwrapper/vertex_array.hh  GLwrapper/texture.hh GLwrapper/glsupport.cc GLwrapper/shapes.cc GLwrapper/shapes.hh GLwrapper/framebuffer.cc GLwrapper/framebuffer.hh)
file(GLOB IMGUI_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/*")
file(GLOB THIRD_PARTY
        third_party/*.c third_party/*.cc third_party/*.h utils/*.cc utils/*.h
        )
message(${THIRD_PARTY})
add_subdirectory(third_party/glad)



function(set_flags name)
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR})
    target_include_directories(${name} SYSTEM PUBLIC "third_party")
    target_compile_definitions(${name} PUBLIC -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(${name} PUBLIC glfw ${CMAKE_DL_LIBS} spdlog::spdlog)
    if (DEFINED VCPKG_TARGET_TRIPLET)
        target_link_libraries(${name} PUBLIC assimp::assimp glm::glm)
    else ()
        # In Linux with assimp installed from pacman, there is no target named assimp::assimp
        target_link_libraries(${name} PUBLIC assimp)
    endif ()
endfunction()

# add_library(glsupport_bindless STATIC $<TARGET_OBJECTS:glsupport_objs> GLwrapper/program.cc GLwrapper/texture.cc)
add_library(glsupport_bindless STATIC ${HEADER_LIST} ${THIRD_PARTY} ${IMGUI_SOURCE} ${SOURCE_LIST})
target_compile_definitions(glsupport_bindless PUBLIC -DGL_ARB_BINDLESS)
target_link_libraries(glsupport_bindless PUBLIC debug "glad_bindless_d" optimized "glad_bindless" )
target_set_warning_flags(glsupport_bindless)
set_flags("glsupport_bindless")

add_library(glsupport_core STATIC ${HEADER_LIST} ${THIRD_PARTY} ${IMGUI_SOURCE} ${SOURCE_LIST})
target_link_libraries(glsupport_core PUBLIC debug "glad_core_d" optimized "glad_core" )
target_set_warning_flags(glsupport_core)
set_flags("glsupport_core")


set(BINDLESS_EXAMPLES
        skybox
        pbrRender
        oit
        )
foreach (EXAMPLE ${BINDLESS_EXAMPLES})
    add_executable(${EXAMPLE} examples/${EXAMPLE}/${EXAMPLE}.cc)
    target_include_directories(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(${EXAMPLE} PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(${EXAMPLE} PRIVATE glsupport_bindless)
    # Flags are added only with the main target
    target_set_warning_flags(${EXAMPLE})
endforeach ()


set(CORE_EXAMPLES
        imgui_example
        ssao
        shadowmap
        instance
        LTC
        )
foreach (EXAMPLE ${CORE_EXAMPLES})
    add_executable(${EXAMPLE} examples/${EXAMPLE}/${EXAMPLE}.cc)
    target_include_directories(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(${EXAMPLE} PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(${EXAMPLE} PRIVATE glsupport_core)
    # Flags are added only with the main target
    target_set_warning_flags(${EXAMPLE})
endforeach ()
