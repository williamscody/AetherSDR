# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#
# Modified from qtbase/cmake/FindWrapOpenGL.cmake:
# AGL block removed to support macOS Tahoe.
# Matches the upstream Qt 6.9 fix.

if(TARGET WrapOpenGL::WrapOpenGL)
    set(WrapOpenGL_FOUND ON)
    return()
endif()

set(WrapOpenGL_FOUND OFF)

find_package(OpenGL ${WrapOpenGL_FIND_VERSION})

if(OpenGL_FOUND)
    set(WrapOpenGL_FOUND ON)

    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)

    if(APPLE)
        get_target_property(__opengl_fw_lib_path OpenGL::GL IMPORTED_LOCATION)

        if(__opengl_fw_lib_path AND NOT __opengl_fw_lib_path MATCHES "/([^/]+)\\.framework$")
            get_filename_component(
                __opengl_fw_path
                "${__opengl_fw_lib_path}"
                DIRECTORY
            )
        endif()

        if(NOT __opengl_fw_path)
            set(__opengl_fw_path "-framework OpenGL")
        endif()

        target_link_libraries(
            WrapOpenGL::WrapOpenGL
            INTERFACE
            ${__opengl_fw_path}
        )
    else()
        target_link_libraries(
            WrapOpenGL::WrapOpenGL
            INTERFACE
            OpenGL::GL
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    WrapOpenGL
    DEFAULT_MSG
    WrapOpenGL_FOUND
)