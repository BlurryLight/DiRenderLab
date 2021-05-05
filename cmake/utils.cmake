if ((${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC"))
    # clang-cl
    set(CLANG_CL True)
    message("clang-cl detected!")
endif ()

function(target_set_warning_flags arg)

    if (MSVC)
        if (DEFINED CLANG_CL)
            target_compile_options(${arg} PRIVATE /EHa /EHs) # 打开异常
            target_compile_definitions(${arg} PRIVATE -D_CLANG_CL) # 打开异常
        endif ()
        target_compile_options(${arg} PRIVATE /W3 /WX)
        # ignore stupid conversion warnings
        target_compile_options(${arg} PRIVATE /wd4305 /wd4244 /wd4287 /wd4838)
        # special case: nameless struct/union is nonstandard but major compilers support it so I use it
        # disable the warning about it
        target_compile_options(${arg} PRIVATE /wd4201)
        # stupid msvc only accepts UTF-8 with Bom unless we force it to accept utf-8
        target_compile_options(${arg} PRIVATE /utf-8)
        #clang
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${arg} PRIVATE -Wall -Wextra -pedantic -Wno-unknown-warning-option)
        #gcc
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${arg} PRIVATE -Wall -Wextra -pedantic)
    endif ()
endfunction()