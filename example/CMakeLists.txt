cmake_minimum_required(VERSION 3.10)
project(ftgl_example CXX)

cmake_policy(SET CMP0074 NEW)  # honour <PackageName>_ROOT variables

# ---- Locate dependencies ----
find_package(OpenGL REQUIRED)

# Prefer FreeGLUT config-mode (provides FreeGLUT::freeglut imported target)
# Fall back to cmake's FindGLUT module for other GLUT installations
find_package(FreeGLUT CONFIG QUIET)
if(FreeGLUT_FOUND)
    add_library(GLUT::GLUT ALIAS FreeGLUT::freeglut)
else()
    set(GLUT_ROOT "" CACHE PATH "Root of GLUT / freeglut installation")
    find_package(GLUT REQUIRED)
endif()

# FTGL can be consumed either:
#   a) from the parent build (add_subdirectory usage), or
#   b) from an installed FTGLConfig.cmake via find_package
if(NOT TARGET ftgl)
    set(FTGL_DIR "" CACHE PATH "Directory containing FTGLConfig.cmake")
    find_package(FTGL REQUIRED CONFIG)
endif()

# ---- Example executable ----
add_executable(ftgl_example main.cpp)

target_link_libraries(ftgl_example PRIVATE
    ftgl
    GLUT::GLUT
    OpenGL::GL
)

# Optional: default font path baked in at compile time (can be overridden)
set(FTGL_EXAMPLE_FONT ""
    CACHE FILEPATH "Font .ttf to bake into the example as the default")
if(FTGL_EXAMPLE_FONT)
    target_compile_definitions(ftgl_example PRIVATE
        FONT_PATH="${FTGL_EXAMPLE_FONT}")
endif()

# Windows: copy the ftgl DLL next to the executable for easy running
if(WIN32)
    add_custom_command(TARGET ftgl_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ftgl>
            $<TARGET_FILE_DIR:ftgl_example>
        COMMENT "Copying ftgl DLL alongside ftgl_example"
    )
endif()
