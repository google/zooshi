# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 2.8.12)

project(zooshi)

# Compile the game with the debug flag
set(ZOOSHI_DEBUG ON)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_BINARY_DIR}/obj)

# Directory which contains the source for 3rd party libraries.
get_filename_component(
    third_party_root "${CMAKE_SOURCE_DIR}/../../../../external" REALPATH)
# Directory which contains source for FPL libraries.
get_filename_component(
    fpl_root "${CMAKE_SOURCE_DIR}/../../libs" REALPATH)

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
set(dependencies_root "${CMAKE_SOURCE_DIR}/dependencies")
if(EXISTS "${dependencies_root}")
  set(third_party_root "${dependencies_root}")
  set(fpl_root "${dependencies_root}")
endif()

# Configurable locations of dependencies of this project.
set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
set(dependencies_pindrop_dir "${fpl_root}/audio_engine"
    CACHE PATH "Directory containing the pindrop library.")
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
set(dependencies_imgui_dir "${fpl_root}/imgui"
    CACHE PATH "Directory containing the IMGUI library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_sdl_dir "${third_party_root}/sdl"
    CACHE PATH "Directory containing the SDL library.")
set(dependencies_libogg_cmake_dir "${CMAKE_MODULE_PATH}/libogg"
    CACHE PATH "Directory containing the libogg cmake project.")
set(dependencies_libogg_distr_dir "${third_party_root}/libogg"
    CACHE PATH "Directory containing the libogg library.")
set(dependencies_libvorbis_cmake_dir "${CMAKE_MODULE_PATH}/libvorbis"
    CACHE PATH "Directory containing the libvorbis cmake project.")
set(dependencies_libvorbis_distr_dir "${third_party_root}/libvorbis"
    CACHE PATH "Directory containing the libvorbis library.")
set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")
set(dependencies_sdl_mixer_cmake_dir "${CMAKE_MODULE_PATH}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer cmake project.")
set(dependencies_sdl_mixer_distr_dir "${third_party_root}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer library.")
set(dependencies_webp_cmake_dir "${CMAKE_MODULE_PATH}/webp"
    CACHE PATH "Directory containing the Webp cmake project.")
set(dependencies_webp_distr_dir "${third_party_root}/webp"
    CACHE PATH "Directory containing the Webp distribution.")

# Option to enable / disable the test build.
option(zooshi_build_tests "Build tests for this project." ON)

# Option to enable / disable the build of cwebp from source.
option(zooshi_build_cwebp "Build cwebp from source." OFF)

# Option to only build flatc
option(zooshi_only_flatc "Only build FlatBuffers compiler." OFF)

if(zooshi_build_cwebp)
  # Depend upon the cwebp target to build assets.
  set(CWEBP_TARGET cwebp)
  # Enable the build of cwebp from source.
  set(webp_build_cwebp ON CACHE BOOL "")
endif()

# Include SDL in this project while working around SDL requirement of out of
# tree builds.
#set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library")
# (hack) We redefine install to stop SDL's CMakeLists.txt from referring to SDL2 lib
function(install)
endfunction(install)
# For some reason SDL defaults to these not being defined, and then defines their own,
# causing linker errors.
add_definitions(-DHAVE_LIBC)
add_definitions(-DHAVE_STDIO_H)
set(BIN_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/.)
if(APPLE)
  set(SDL_AUDIO_DRIVER_COREAUDIO 1)
endif(APPLE)
if(NOT zooshi_only_flatc AND NOT TARGET SDL2-static)
  add_subdirectory(${dependencies_sdl_dir} ${tmp_dir}/sdl)
endif()
set(CMAKE_BINARY_DIR ${BIN_DIR})

# Include pindrop.
if(NOT zooshi_only_flatc AND NOT TARGET pindrop)
  add_subdirectory("${dependencies_pindrop_dir}" ${tmp_dir}/pindrop)
endif()

# Include motive.
if(NOT zooshi_only_flatc AND NOT TARGET motive)
  set(motive_build_samples OFF CACHE BOOL "")
  set(motive_build_tests OFF CACHE BOOL "")
  add_subdirectory("${dependencies_motive_dir}" ${tmp_dir}/motive)
endif()

# Include libogg.
if(NOT zooshi_only_flatc AND NOT TARGET libogg)
#  add_subdirectory("${dependencies_libogg_cmake_dir}" ${tmp_dir}/libogg)
endif()

# Include libvorbis.
if(NOT zooshi_only_flatc AND NOT TARGET libvorbis)
#  add_subdirectory("${dependencies_libvorbis_cmake_dir}" ${tmp_dir}/libvorbis)
endif()

# Include fplbase.
if(NOT zooshi_only_flatc AND NOT TARGET fplbase)
  add_subdirectory("${dependencies_fplbase_dir}" ${tmp_dir}/fplbase)
endif()

# Include imgui.
if(NOT zooshi_only_flatc AND NOT TARGET imgui)
  add_subdirectory("${dependencies_imgui_dir}" ${tmp_dir}/imgui)
endif()

# Include SDL Mixer.
set(SDL_MIXER_OGG_SUPPORT ON CACHE BOOL "Compile in Ogg Vorbis support")
if(NOT zooshi_only_flatc AND NOT TARGET sdl_mixer)
#  add_subdirectory("${dependencies_sdl_mixer_cmake_dir}" ${tmp_dir}/sdl_mixer)
endif()

# Include Webp in this project, and force build in release mode for speed.
set(original_build_type ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
if(NOT zooshi_only_flatc AND NOT TARGET webp)
  add_subdirectory("${dependencies_webp_cmake_dir}" ${tmp_dir}/webp)
endif()
set(CMAKE_BUILD_TYPE ${original_build_type})

# Include FlatBuffers in this project.
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
if(NOT TARGET flatc)
  add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)
endif()

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(FLATBUFFERS_GENERATED_INCLUDES_DIR
    ${CMAKE_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include)
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_SOURCE_DIR}/src/*.fbs)

# Generate rules to build the set of output files from the set of input
# schema files.
foreach(flatbuffers_schema ${FLATBUFFERS_SCHEMAS})
  get_filename_component(filename ${flatbuffers_schema} NAME_WE)
  set(flatbuffers_include
      ${FLATBUFFERS_GENERATED_INCLUDES_DIR}/${filename}_generated.h)
  add_custom_command(
    OUTPUT ${flatbuffers_include}
    COMMAND flatc --gen-includes -o ${FLATBUFFERS_GENERATED_INCLUDES_DIR}
        -I ${dependencies_motive_dir}/schemas
        -I ${dependencies_pindrop_dir}/schemas -c ${flatbuffers_schema}
    DEPENDS flatc motive pindrop ${flatbuffers_schema})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

# Build rule that uses make to build the assets.
get_property(flatc_location TARGET flatc PROPERTY LOCATION)
add_custom_target(assets
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/build_assets.py
                 --flatc ${flatc_location} --output ${CMAKE_BINARY_DIR}/assets
  DEPENDS flatc ${CWEBP_TARGET})

if(APPLE)
  find_library(OPENGL_LIBRARIES NAMES GL GLU)
  SET(CMAKE_CXX_LINK_FLAGS "-framework OpenGL")
endif()

# zooshi source files.
set(zooshi_SRCS
    src/camera.cpp
    src/camera.h
    src/components/physics.cpp
    src/components/physics.h
    src/components/player.cpp
    src/components/player.h
    src/components/rail_denizen.cpp
    src/components/rail_denizen.h
    src/components/rendermesh.cpp
    src/components/rendermesh.h
    src/components/transform.cpp
    src/components/transform.h
    src/entity/component.h
    src/entity/component_id_lookup.h
    src/entity/component_interface.h
    src/entity/entity.h
    src/entity/entity_common.h
    src/entity/entity_manager.cpp
    src/entity/entity_manager.h
    src/entity/vector_pool.h
    src/flatbufferschemas
    src/game.cpp
    src/game.h
    src/game_state.cpp
    src/game_state.h
    src/gui.cpp
    src/gui.h
    src/main.cpp
    src/rawassets
)

# Includes for this project.
include_directories(src)
if(WIN32)
  include_directories(external/include/windows)
endif()

include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${dependencies_fplutil_dir}/libfplutil/include)
include_directories(${dependencies_motive_dir}/include)
include_directories(${dependencies_pindrop_dir}/include)
include_directories(${dependencies_webp_distr_dir}/include)
include_directories(${dependencies_webp_distr_dir}/src)
include_directories(${dependencies_fplbase_dir}/include)
include_directories(${dependencies_sdl_dir}/include)
include_directories(${dependencies_imgui_dir}/include)


# libogg includes.
include_directories(${dependencies_libogg_distr_dir}/include)

# libvorbis includes.
include_directories(${dependencies_libvorbis_distr_dir}/include)

# SDL Mixer includes.
include_directories(${dependencies_sdl_mixer_distr_dir})

# Detect clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4065 /wd4244 /wd4511 /wd4512 /wd4706")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra -Wno-unused-function")
  # -Wno-unused-function is because of webp encode.h
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

if(ZOOSHI_DEBUG)
  # if we want to define this, it needs to be only in debug builds
  #add_definitions(-D_DEBUG)
endif()

#set (SDL_LIBRARIES SDL2-static)
if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  set(OPENGL_LIBRARIES opengl32)
  set(SDL_LIBRARIES SDL2main ${SDL_LIBRARIES})
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

# Executable target.
add_executable(zooshi ${zooshi_SRCS})
# Additional flags for the target.
mathfu_configure_flags(zooshi)
# Dependencies for the executable target.
add_dependencies(zooshi generated_includes assets)
target_link_libraries(zooshi
  ${SDL_LIBRARIES}
  motive
  fplbase
  imgui
  pindrop
  sdl_mixer
  libvorbis
  libogg
  ${OPENGL_LIBRARIES}
  webp)

# Tests.
if(NOT zooshi_only_flatc)
  if(zooshi_build_tests)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
  endif()
endif()

# Create a zipped tar of all the necessary files to run the game.
add_custom_target(export
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/export.py
  DEPENDS zooshi)

