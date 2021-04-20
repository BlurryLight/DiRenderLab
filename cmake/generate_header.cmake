# generate header
set(ROOT_DIR_VAR ${CMAKE_SOURCE_DIR})
if (DEFINED CMAKE_BUILD_TYPE)
    set(BUILD_TYPE_VAR ${CMAKE_BUILD_TYPE})
else ()
    # In Linux the default is empty.
    set(BUILD_TYPE_VAR "Debug")
endif ()
set(BUILD_SYSTEM_VERSION_VAR ${CMAKE_SYSTEM_VERSION})
set(BUILD_SYSTEM_NAME_VAR ${CMAKE_SYSTEM_NAME})
string(TIMESTAMP BUILD_UTC_TIMESTAMP_VAR UTC)
configure_file(${CMAKE_SOURCE_DIR}/utils/cmake_vars.h.in ${CMAKE_SOURCE_DIR}/utils/cmake_vars.h)

