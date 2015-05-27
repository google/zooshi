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
set(dependencies_pindrop_dir "${fpl_root}/pindrop"
    CACHE PATH "Directory containing the pindrop library.")
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
set(dependencies_imgui_dir "${fpl_root}/imgui"
    CACHE PATH "Directory containing the IMGUI library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")

# Option to enable / disable the test build.
option(zooshi_build_tests "Build tests for this project." ON)

# Option to enable / disable the build of cwebp from source.
option(zooshi_build_cwebp "Build cwebp from source." ON)

# Option to only build flatc
option(zooshi_only_flatc "Only build FlatBuffers compiler." OFF)

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

# Include fplbase.
if(NOT zooshi_only_flatc AND NOT TARGET fplbase)
  # By default, don't build the mesh_pipeline. Use the one that's prebuilt.
  # We do this to avoid having to install the FBX SDK.
  set(fplbase_build_mesh_pipeline OFF CACHE BOOL "")
  if(zooshi_build_cwebp)
    # enable cwebp build included in fplbase.
    set(webp_build_cwebp ON CACHE BOOL "")
  endif()
  add_subdirectory("${dependencies_fplbase_dir}" ${tmp_dir}/fplbase)
endif()

# Include imgui.
if(NOT zooshi_only_flatc AND NOT TARGET imgui)
  add_subdirectory("${dependencies_imgui_dir}" ${tmp_dir}/imgui)
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
        -I ${dependencies_fplbase_dir}/schemas
        -I ${dependencies_motive_dir}/schemas
        -I ${dependencies_pindrop_dir}/schemas -c ${flatbuffers_schema}
    DEPENDS flatc motive pindrop ${flatbuffers_schema})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

# Build rule that uses make to build the assets.
get_property(flatc_location TARGET flatc PROPERTY LOCATION)
get_property(cwebp_location TARGET cwebp PROPERTY LOCATION)
add_custom_target(assets
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/build_assets.py
  --flatc ${flatc_location}
  --cwebp ${cwebp_location}
  --output ${CMAKE_BINARY_DIR}/assets
  DEPENDS flatc cwebp)

# Copy imgui resource files (font files, shaders)
add_custom_command(TARGET assets POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${dependencies_imgui_dir}/assets
                       ${CMAKE_BINARY_DIR}/assets)

# zooshi source files.
set(zooshi_SRCS
    src/camera.cpp
    src/camera.h
    src/components/audio_listener.cpp
    src/components/audio_listener.h
    src/components/patron.cpp
    src/components/patron.h
    src/components/physics.cpp
    src/components/physics.h
    src/components/player.cpp
    src/components/player.h
    src/components/player_projectile.cpp
    src/components/player_projectile.h
    src/components/rail_denizen.cpp
    src/components/rail_denizen.h
    src/components/rendermesh.cpp
    src/components/rendermesh.h
    src/components/score.cpp
    src/components/score.h
    src/components/sound.cpp
    src/components/sound.h
    src/components/time_limit.cpp
    src/components/time_limit.h
    src/components/transform.cpp
    src/components/transform.h
    src/editor/world_editor.cpp
    src/editor/world_editor.h
    src/entity/component.h
    src/entity/component_id_lookup.h
    src/entity/component_interface.h
    src/entity/entity.h
    src/entity/entity_common.h
    src/entity/entity_manager.cpp
    src/entity/entity_manager.h
    src/entity/vector_pool.h
    src/event_system/event_listener.h
    src/event_system/event_manager.cpp
    src/event_system/event_manager.h
    src/event_system/event_payload.h
    src/event_system/event_registry.h
    src/events/audio_event.h
    src/events/entity_event.h
    src/events/event_ids.h
    src/events/hit_patron.h
    src/events/hit_patron_body.h
    src/events/hit_patron_mouth.h
    src/events/projectile_fired_event.h
    src/flatbufferschemas
    src/game.cpp
    src/game.h
    src/game_state.cpp
    src/game_state.h
    src/gui.cpp
    src/gui.h
    src/inputcontrollers/base_player_controller.h
    src/inputcontrollers/mouse_controller.cpp
    src/inputcontrollers/mouse_controller.h
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
include_directories(${dependencies_fplbase_dir}/include)
include_directories(${dependencies_imgui_dir}/include)

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

# Executable target.
add_executable(zooshi ${zooshi_SRCS})
# Additional flags for the target.
mathfu_configure_flags(zooshi)
# Dependencies for the executable target.
add_dependencies(zooshi generated_includes assets)
target_link_libraries(zooshi
  motive
  fplbase
  imgui
  pindrop)

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

