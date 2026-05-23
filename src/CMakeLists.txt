cmake_minimum_required(VERSION 3.10)

project(fisheye_stitcher C)

include(ExternalProject)


# Set the path to the libjpeg-turbo directory
set(LIBJPEG_TURBO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libjpeg-turbo)

# Define the installation directory for libjpeg-turbo
set(LIBJPEG_TURBO_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libjpeg-turbo-install)


get_property(CMAKE_SYSTEM_NAME GLOBAL PROPERTY CMAKE_SYSTEM_NAME)
get_property(CMAKE_SYSTEM_NAME GLOBAL PROPERTY CMAKE_SYSTEM_NAME)
get_property(CMAKE_EXPORT_COMPILE_COMMANDS GLOBAL PROPERTY CMAKE_EXPORT_COMPILE_COMMANDS)
get_property(CMAKE_SYSTEM_VERSION GLOBAL PROPERTY CMAKE_SYSTEM_VERSION)
get_property(ANDROID_PLATFORM GLOBAL PROPERTY ANDROID_PLATFORM)
get_property(ANDROID_ABI GLOBAL PROPERTY ANDROID_ABI)
get_property(CMAKE_ANDROID_ARCH_ABI GLOBAL PROPERTY CMAKE_ANDROID_ARCH_ABI)
get_property(ANDROID_NDK GLOBAL PROPERTY ANDROID_NDK)
get_property(CMAKE_ANDROID_NDK GLOBAL PROPERTY CMAKE_ANDROID_NDK)
get_property(CMAKE_TOOLCHAIN_FILE GLOBAL PROPERTY CMAKE_TOOLCHAIN_FILE)
get_property(CMAKE_LIBRARY_OUTPUT_DIRECTORY GLOBAL PROPERTY CMAKE_LIBRARY_OUTPUT_DIRECTORY)
get_property(CMAKE_RUNTIME_OUTPUT_DIRECTORY GLOBAL PROPERTY CMAKE_RUNTIME_OUTPUT_DIRECTORY)
get_property(CMAKE_BUILD_TYPE GLOBAL PROPERTY CMAKE_BUILD_TYPE)


# Android 15 (API 35) requires shared libraries to be aligned to a 16 KB
# page size. Pass -Wl,-z,max-page-size=16384 to the linker for every binary
# we build so the LOAD segments are aligned correctly even when the host
# app is built with an older NDK that defaults to 4 KB pages (NDK < r28).
set(PAGE_SIZE_16KB_LINKER_FLAGS "-Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384")

# Set up libjpeg-turbo as an external project.
#
# We link libjpeg-turbo statically (libjpeg.a / libturbojpeg.a) so there is
# no need to also build the shared variants. Disabling ENABLE_SHARED keeps
# unused, 4 KB-aligned libturbojpeg.so / libjpeg.so out of the final APK,
# which is what triggers the Play Store / Android 15 "ELF alignment check
# failed" warning. CMAKE_POSITION_INDEPENDENT_CODE is required so the
# static archive can be linked into our shared plugin library.
ExternalProject_Add(
  libjpeg_turbo
  SOURCE_DIR ${LIBJPEG_TURBO_DIR}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
    -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
    -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}
    -DANDROID_PLATFORM=${ANDROID_PLATFORM}
    -DANDROID_ABI=${ANDROID_ABI}
    -DCMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI}
    -DANDROID_NDK=${ANDROID_NDK}
    -DCMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DENABLE_SHARED=0
    -DENABLE_STATIC=1
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_SHARED_LINKER_FLAGS=${PAGE_SIZE_16KB_LINKER_FLAGS}
    -DCMAKE_EXE_LINKER_FLAGS=${PAGE_SIZE_16KB_LINKER_FLAGS}
  BUILD_ALWAYS 1
  BUILD_BYPRODUCTS 
    ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libturbojpeg.a
    ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libjpeg.a
)

# Create your plugin library
add_library(${PROJECT_NAME} SHARED
  bitmaplib.c
  fisheye_stitcher.c
)

# Set C standard (adjust as needed)
set_target_properties(${PROJECT_NAME} PROPERTIES
  PUBLIC_HEADER "fisheye_stitcher.h"
  OUTPUT_NAME "fisheye_stitcher"
)

# Force 16 KB-aligned LOAD segments in libfisheye_stitcher.so so the plugin
# is compatible with Android 15's 16 KB page size requirement regardless of
# the NDK version used by the host app.
target_link_options(${PROJECT_NAME} PRIVATE
  "-Wl,-z,max-page-size=16384"
  "-Wl,-z,common-page-size=16384"
)

# Make sure libjpeg-turbo is built before your plugin
add_dependencies(${PROJECT_NAME} libjpeg_turbo)

# Link against libjpeg-turbo and libjpeg
target_link_libraries(
  ${PROJECT_NAME} PRIVATE
  ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libjpeg.a
  ${LIBJPEG_TURBO_INSTALL_DIR}/lib/libturbojpeg.a
)

# Specify include directories
target_include_directories(${PROJECT_NAME} PUBLIC
  ${LIBJPEG_TURBO_INSTALL_DIR}/include
)

# Ensure the libjpeg-turbo libraries exist before linking
add_custom_command(
  TARGET ${PROJECT_NAME}
  PRE_LINK
  COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBJPEG_TURBO_INSTALL_DIR}/lib
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target libjpeg_turbo
)
