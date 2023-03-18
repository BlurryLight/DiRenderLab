set(BINDLESS_EXAMPLES
        skybox
        pbrRender
        )
foreach (EXAMPLE ${BINDLESS_EXAMPLES})
    add_executable(${EXAMPLE} ${CMAKE_SOURCE_DIR}/examples/${EXAMPLE}/${EXAMPLE}.cc)
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
        oit
        PboBench
        DeferredMSAA
        )
foreach (EXAMPLE ${CORE_EXAMPLES})
    add_executable(${EXAMPLE} ${CMAKE_SOURCE_DIR}/examples/${EXAMPLE}/${EXAMPLE}.cc)
    target_include_directories(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR})
    target_compile_definitions(${EXAMPLE} PRIVATE -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
    target_link_libraries(${EXAMPLE} PRIVATE glsupport_core)
    # Flags are added only with the main target
    target_set_warning_flags(${EXAMPLE})
endforeach ()

# private examples
add_subdirectory(internal_examples)