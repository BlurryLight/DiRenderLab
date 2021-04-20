cmake_minimum_required(VERSION 3.12)
project(SSAO CXX)
set(CMAKE_CXX_STANDARD 17) #enable filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(${CMAKE_SOURCE_DIR}/cmake/cmake_filesystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/generate_header.cmake)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
file(GLOB SHADERS *.vs *.fs *.vert *.frag)
file(COPY ${SHADERS}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
set(SOURCE_LIST main.cc)
set(HEADER_LIST glsupport.hpp)
file(GLOB THIRD_PARTY third_party/*.c third_party/*.cc third_party/*.h utils/*.cc utils/*.h)
include_directories("third_party")
add_subdirectory("glad")
message(${THIRD_PARTY})
add_executable(${PROJECT_NAME} ${HEADER_LIST} ${SOURCE_LIST} ${THIRD_PARTY})
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${OPENGL_LIBRARIES}
        ${GLFW_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${GLM_LIBRARIES}
        dl
        ${ASSIMP_LIBRARIES}
        glad
        )

