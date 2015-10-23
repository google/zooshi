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
if(NOT DEFINED third_party_root)
  get_filename_component(
    third_party_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../../external"
    REALPATH)
endif()

# Directory which contains source for FPL libraries.
if(NOT DEFINED fpl_root)
  get_filename_component(
      fpl_root "${CMAKE_CURRENT_SOURCE_DIR}/../../libs" REALPATH)
endif()

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
if(NOT DEFINED dependencies_root)
  set(dependencies_root "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
  if(EXISTS "${dependencies_root}")
    set(third_party_root "${dependencies_root}")
    set(fpl_root "${dependencies_root}")
  endif()
endif()

# Configurable locations of dependencies of this project.
set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
set(dependencies_breadboard_dir "${fpl_root}/breadboard"
    CACHE PATH "Directory containing the breadboard library.")
set(dependencies_module_library_dir
    "${dependencies_breadboard_dir}/module_library" CACHE PATH
    "Directory containing the breadboard module library.")
set(dependencies_pindrop_dir "${fpl_root}/pindrop"
    CACHE PATH "Directory containing the pindrop library.")
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FPLBase library.")
set(dependencies_entity_dir "${fpl_root}/entity"
    CACHE PATH "Directory containing the Entity library.")
set(dependencies_flatui_dir "${fpl_root}/flatui"
    CACHE PATH "Directory containing the FlatUI library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")
set(dependencies_scene_lab_dir "${fpl_root}/scene_lab"
    CACHE PATH "Directory containing the Scene Lab library.")
set(dependencies_component_library_dir
    "${dependencies_entity_dir}/component_library"
    CACHE PATH "Directory containing the entity system component library")

# Option to enable / disable the build of cwebp from source.
option(zooshi_build_cwebp "Build cwebp from source." OFF)

# Option to only build flatc
option(zooshi_only_flatc "Only build FlatBuffers compiler." OFF)

# Option to output profiling numbers on motive.
option(zooshi_profile_motive "Output motive profiling stats." OFF)

# Include pindrop.
if(NOT zooshi_only_flatc AND NOT TARGET pindrop)
  set(pindrop_build_demo OFF CACHE BOOL "")
  add_subdirectory("${dependencies_pindrop_dir}" ${tmp_dir}/pindrop)
endif()

# Include breadboard.
if(NOT zooshi_only_flatc AND NOT TARGET breadboard)
  add_subdirectory("${dependencies_breadboard_dir}" ${tmp_dir}/breadboard)
endif()

# Include motive.
if(NOT zooshi_only_flatc AND NOT TARGET motive)
  set(motive_build_samples OFF CACHE BOOL "")
  set(motive_build_tests OFF CACHE BOOL "")
  if(zooshi_profile_motive)
    set(motive_enable_benchmarks ON CACHE BOOL "")
    add_definitions(-DBENCHMARK_MOTIVE)
  endif()
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

# Include entities.
if(NOT zooshi_only_flatc AND NOT TARGET entity)
  add_subdirectory("${dependencies_entity_dir}" ${tmp_dir}/entity)
endif()

# Include FlatUI.
if(NOT zooshi_only_flatc AND NOT TARGET flatui)
  add_subdirectory("${dependencies_flatui_dir}" ${tmp_dir}/flatui)
endif()

# Include Scene Lab.
if(NOT zooshi_only_flatc AND NOT TARGET scene_lab)
  add_subdirectory("${dependencies_scene_lab_dir}" ${tmp_dir}/scene_lab)
endif()

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

set(FLATBUFFERS_SCHEMA_INCLUDES
    -I ${dependencies_component_library_dir}/schemas
    -I ${dependencies_scene_lab_dir}/schemas
    -I ${dependencies_fplbase_dir}/schemas
    -I ${dependencies_motive_dir}/schemas
    -I ${dependencies_pindrop_dir}/schemas)

# Generate rules to build the set of output files from the set of input
# schema files.
unset(FLATBUFFERS_GENERATED_INCLUDES)
foreach(flatbuffers_schema ${FLATBUFFERS_SCHEMAS})
  get_filename_component(filename ${flatbuffers_schema} NAME_WE)
  set(flatbuffers_include
      ${FLATBUFFERS_GENERATED_INCLUDES_DIR}/${filename}_generated.h)
  set(flatbuffers_binary_schema
      ${CMAKE_BINARY_DIR}/assets/flatbufferschemas/${filename}.bfbs)
  file(COPY ${flatbuffers_schema}
       DESTINATION ${CMAKE_BINARY_DIR}/assets/flatbufferschemas/)
  add_custom_command(
    OUTPUT ${flatbuffers_include}
    COMMAND flatc --gen-mutable -o ${FLATBUFFERS_GENERATED_INCLUDES_DIR}
        ${FLATBUFFERS_SCHEMA_INCLUDES} -c ${flatbuffers_schema}
    DEPENDS
        flatc
        motive_generated_includes
        pindrop_generated_includes
        component_library_generated_includes
        ${flatbuffers_schema})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
  # Generate the binary schema files as well.
  add_custom_command(
    OUTPUT ${flatbuffers_binary_schema}
    COMMAND flatc -b --schema
        -o ${CMAKE_BINARY_DIR}/assets/flatbufferschemas
        ${FLATBUFFERS_SCHEMA_INCLUDES}
        ${flatbuffers_schema}
    DEPENDS
        flatc
        motive_generated_includes
        pindrop_generated_includes
        component_library_generated_includes
        ${flatbuffers_schema})
  # Temporarily don't generate all binary schemas until b/22063091 is fixed.
  # list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_binary_schema})
endforeach()
# Only build the components binary schema until b/22063091 is fixed.
list(APPEND
     FLATBUFFERS_GENERATED_INCLUDES
     ${CMAKE_BINARY_DIR}/assets/flatbufferschemas/components.bfbs)
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

if(NOT zooshi_only_flatc)
  # If building cwebp then pass in the location.
  if(zooshi_build_cwebp)
    get_property(cwebp_location TARGET cwebp PROPERTY LOCATION)
    set(cwebp_option --cwebp ${cwebp_location})
    set(cwebp_depends cwebp)
  endif()

  add_custom_target(assets
    COMMAND python ${CMAKE_SOURCE_DIR}/scripts/build_assets.py
    --flatc $<TARGET_FILE:flatc>
    ${cwebp_option}
    --output ${CMAKE_BINARY_DIR}/assets
    DEPENDS flatc ${cwebp_depends})

  # Copy FlatUI shaders
  add_custom_command(TARGET assets POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory
                         ${dependencies_flatui_dir}/assets/shaders
                         ${CMAKE_BINARY_DIR}/assets/shaders)

  # Copy Font resource files
  add_custom_command(TARGET assets POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory
                         ${CMAKE_SOURCE_DIR}/src/rawassets/fonts
                         ${CMAKE_BINARY_DIR}/assets/fonts)
endif()

# zooshi source files.
set(zooshi_SRCS
    src/camera.cpp
    src/camera.h
    src/common.h
    src/components/attributes.cpp
    src/components/attributes.h
    src/components/audio_listener.cpp
    src/components/audio_listener.h
    src/components/digit.cpp
    src/components/digit.h
    src/components/lap_dependent.cpp
    src/components/lap_dependent.h
    src/components/patron.cpp
    src/components/patron.h
    src/components/player.cpp
    src/components/player.h
    src/components/player_projectile.cpp
    src/components/player_projectile.h
    src/components/rail_denizen.cpp
    src/components/rail_denizen.h
    src/components/rail_node.cpp
    src/components/rail_node.h
    src/components/river.cpp
    src/components/river.h
    src/components/scenery.cpp
    src/components/scenery.h
    src/components/services.cpp
    src/components/services.h
    src/components/shadow_controller.cpp
    src/components/shadow_controller.h
    src/components/simple_movement.cpp
    src/components/simple_movement.h
    src/components/sound.cpp
    src/components/sound.h
    src/components/time_limit.cpp
    src/components/time_limit.h
    src/default_entity_factory.cpp
    src/full_screen_fader.cpp
    src/full_screen_fader.h
    src/game.cpp
    src/game.h
    src/gpg_manager.h
    src/gpg_manager.cpp
    src/gui.cpp
    src/inputcontrollers/gamepad_controller.cpp
    src/inputcontrollers/gamepad_controller.h
    src/inputcontrollers/onscreen_controller.cpp
    src/inputcontrollers/onscreen_controller.h
    src/inputcontrollers/mouse_controller.cpp
    src/inputcontrollers/mouse_controller.h
    src/main.cpp
    src/modules/attributes.cpp
    src/modules/attributes.h
    src/modules/gpg.cpp
    src/modules/gpg.h
    src/modules/patron.cpp
    src/modules/patron.h
    src/modules/player.cpp
    src/modules/player.h
    src/modules/rail_denizen.cpp
    src/modules/rail_denizen.h
    src/modules/state.cpp
    src/modules/state.h
    src/modules/zooshi.cpp
    src/modules/zooshi.h
    src/railmanager.cpp
    src/railmanager.h
    src/states/game_over_state.cpp
    src/states/game_over_state.h
    src/states/game_menu_state.cpp
    src/states/game_menu_state.h
    src/states/gameplay_state.cpp
    src/states/gameplay_state.h
    src/states/intro_state.cpp
    src/states/intro_state.h
    src/states/loading_state.cpp
    src/states/loading_state.h
    src/states/pause_state.cpp
    src/states/pause_state.h
    src/states/state_machine.h
    src/states/states.h
    src/states/states_common.cpp
    src/states/states_common.h
    src/states/scene_lab_state.cpp
    src/states/scene_lab_state.h
    src/world.cpp
    src/world.h
    src/world_renderer.cpp
    src/world_renderer.h
    src/zooshi_graph_factory.cpp
    src/zooshi_graph_factory.h
    # For outputting flatbuffer files as json.
    ${dependencies_flatbuffers_dir}/src/idl_parser.cpp
    ${dependencies_flatbuffers_dir}/src/idl_gen_text.cpp
)

# Includes for this project.
include_directories(src)
if(WIN32)
  include_directories(${dependencies_fplbase_dir}/external/include)
endif()

include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET component_library_generated_includes
             PROPERTY COMPONENT_LIBRARY_DIR)
include_directories(${COMPONENT_LIBRARY_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET fplbase_generated_includes PROPERTY FPLBASE_DIR)
include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(SCENE_LAB_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET scene_lab_generated_includes PROPERTY SCENE_LAB_DIR)
include_directories(${SCENE_LAB_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET motive_generated_includes PROPERTY MOTIVE_DIR)
include_directories(${MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET pindrop_generated_includes PROPERTY PINDROP_DIR)
include_directories(${PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${dependencies_entity_dir}/include)
include_directories(${dependencies_component_library_dir}/include)
include_directories(${dependencies_breadboard_dir}/include)
include_directories(${dependencies_module_library_dir}/include)
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${dependencies_fplbase_dir}/include)
include_directories(${dependencies_fplutil_dir}/libfplutil/include)
include_directories(${dependencies_flatui_dir}/include)
include_directories(${dependencies_motive_dir}/include)
include_directories(${dependencies_pindrop_dir}/include)
include_directories(${dependencies_scene_lab_dir}/include)

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# Detect clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  # C4127: conditional expression is constant
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4127")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra -Wno-unused-function")
  # -Wno-unused-function is because of webp encode.h
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

set (SDL_LIBRARIES SDL2-static)
if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  set(OPENGL_LIBRARIES opengl32)
  set(SDL_LIBRARIES SDL2main ${SDL_LIBRARIES})
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

if(ZOOSHI_DEBUG)
  # if we want to define this, it needs to be only in debug builds
  #add_definitions(-D_DEBUG)
endif()

# Executable target.
add_executable(zooshi ${zooshi_SRCS})

if(NOT zooshi_only_flatc)
  # Additional flags for the target.
  mathfu_configure_flags(zooshi)
endif()

# Dependencies for the executable target.
add_dependencies(zooshi generated_includes assets)
target_link_libraries(zooshi
  motive
  fplbase
  flatui
  flatbuffers
  breadboard
  entity
  component_library
  module_library
  scene_lab
  pindrop)

# Create a zipped tar of all the necessary files to run the game.
add_custom_target(export
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/export.py
  DEPENDS zooshi)

