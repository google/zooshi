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

LOCAL_PATH:=$(call my-dir)/../../..

# Project directory relative to this file.
ZOOSHI_DIR:=$(LOCAL_PATH)
FLATBUFFERS_FLATC_ARGS:=--gen-mutable

include $(ZOOSHI_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

# Build rule which builds assets for the game.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
.PHONY: build_assets
# Create a binary schema file for the components.fbs schema.
build_assets: $(flatc_target)
	cp -f -r $(ZOOSHI_DIR)/src/rawassets/fonts $(ZOOSHI_DIR)/assets/fonts
	cp -f -r $(DEPENDENCIES_FLATUI_DIR)/assets/shaders \
$(ZOOSHI_DIR)/assets
	$(hide) python $(ZOOSHI_DIR)/scripts/build_assets.py
	-mkdir -p $(ZOOSHI_DIR)/assets/flatbufferschemas
	$(FLATBUFFERS_FLATC) -b --schema \
	  $(foreach include,$(ZOOSHI_FLATBUFFER_INCLUDE_DIRS),-I $(include)) \
	  -o $(ZOOSHI_DIR)/assets/flatbufferschemas \
	  $(ZOOSHI_SCHEMA_DIR)/components.fbs


.PHONY: clean_assets
clean_assets:
	$(hide) python $(ZOOSHI_DIR)/scripts/build_assets.py clean
endif
PROJECT_GLOBAL_BUILD_RULES_DEFINED:=1

include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_ARM_MODE := arm

ZOOSHI_GENERATED_OUTPUT_DIR := $(ZOOSHI_DIR)/gen/include

LOCAL_C_INCLUDES := \
  $(DEPENDENCIES_ENTITY_DIR)/include \
  $(DEPENDENCIES_COMPONENT_LIBRARY_DIR)/include \
  $(DEPENDENCIES_BREADBOARD_DIR)/include \
  $(DEPENDENCIES_BREADBOARD_MODULE_LIBRARY_DIR)/include \
  $(DEPENDENCIES_SCENE_LAB_DIR)/include \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_GPG_DIR)/include \
  $(DEPENDENCIES_WEBP_DIR)/src \
  $(DEPENDENCIES_BULLETPHYSICS_DIR)/src \
  $(COMPONENTS_GENERATED_OUTPUT_DIR) \
  $(BREADBOARD_MODULE_LIBRARY_GENERATED_OUTPUT_DIR) \
  $(ZOOSHI_GENERATED_OUTPUT_DIR) \
  $(SCENE_LAB_GENERATED_OUTPUT_DIR) \
  src

LOCAL_SRC_FILES := \
  src/camera.cpp \
  src/components/attributes.cpp \
  src/components/audio_listener.cpp \
  src/components/digit.cpp \
  src/components/lap_dependent.cpp \
  src/components/patron.cpp \
  src/components/player.cpp \
  src/components/player_projectile.cpp \
  src/components/rail_denizen.cpp \
  src/components/rail_node.cpp \
  src/components/river.cpp \
  src/components/scenery.cpp \
  src/components/services.cpp \
  src/components/shadow_controller.cpp \
  src/components/simple_movement.cpp \
  src/components/sound.cpp \
  src/components/time_limit.cpp \
  src/default_entity_factory.cpp \
  src/default_graph_factory.cpp \
  src/full_screen_fader.cpp \
  src/game.cpp \
  src/gpg_manager.cpp \
  src/gui.cpp \
  src/inputcontrollers/android_cardboard_controller.cpp \
  src/inputcontrollers/gamepad_controller.cpp \
  src/inputcontrollers/onscreen_controller.cpp \
  src/main.cpp \
  src/modules/attributes.cpp \
  src/modules/gpg.cpp \
  src/modules/patron.cpp \
  src/modules/player.cpp \
  src/modules/rail_denizen.cpp \
  src/modules/state.cpp \
  src/modules/zooshi.cpp \
  src/railmanager.cpp \
  src/states/game_menu_state.cpp \
  src/states/game_over_state.cpp \
  src/states/gameplay_state.cpp \
  src/states/intro_state.cpp \
  src/states/loading_state.cpp \
  src/states/pause_state.cpp \
  src/states/states_common.cpp \
  src/states/scene_lab_state.cpp \
  src/world.cpp \
  src/world_renderer.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/idl_parser.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/idl_gen_text.cpp \
  $(DEPENDENCIES_FLATBUFFERS_DIR)/src/reflection.cpp

ZOOSHI_SCHEMA_DIR := $(ZOOSHI_DIR)/src/flatbufferschemas

ZOOSHI_SCHEMA_FILES := \
  $(ZOOSHI_SCHEMA_DIR)/assets.fbs \
  $(ZOOSHI_SCHEMA_DIR)/attributes.fbs \
  $(ZOOSHI_SCHEMA_DIR)/components.fbs \
  $(ZOOSHI_SCHEMA_DIR)/config.fbs \
  $(ZOOSHI_SCHEMA_DIR)/graph.fbs \
  $(ZOOSHI_SCHEMA_DIR)/gpg.fbs \
  $(ZOOSHI_SCHEMA_DIR)/input_config.fbs \
  $(ZOOSHI_SCHEMA_DIR)/rail_def.fbs \
  $(ZOOSHI_SCHEMA_DIR)/save_data.fbs

# Make each source file order-only dependent upon the assets (via the pipe |)
# This guarantees build_assets will run first, but not force all src files to
# rebuild once it has.
$(foreach src,$(LOCAL_SRC_FILES),\
  $(eval $(call local-source-file-path,$(src)): | build_assets))

ZOOSHI_FLATBUFFER_INCLUDE_DIRS := \
  $(DEPENDENCIES_PINDROP_DIR)/schemas \
  $(DEPENDENCIES_MOTIVE_DIR)/schemas \
  $(DEPENDENCIES_FPLBASE_DIR)/schemas \
  $(DEPENDENCIES_SCENE_LAB_DIR)/schemas \
  $(DEPENDENCIES_COMPONENT_LIBRARY_DIR)/schemas \
  $(DEPENDENCIES_BREADBOARD_MODULE_LIBRARY_DIR)/schemas

# Override JNI_OnLoad functions.
FPLBASE_JNI_ONLOAD_FUNCTIONS := SDL_JNI_OnLoad GPG_JNI_OnLoad

ifeq (,$(ZOOSHI_RUN_ONCE))
ZOOSHI_RUN_ONCE := 1
$(call flatbuffers_header_build_rules,\
  $(ZOOSHI_SCHEMA_FILES),\
  $(ZOOSHI_SCHEMA_DIR),\
  $(ZOOSHI_GENERATED_OUTPUT_DIR),\
  $(ZOOSHI_FLATBUFFER_INCLUDE_DIRS),\
  $(LOCAL_SRC_FILES),\
  zooshi_generated_includes,\
  component_library_generated_includes \
  breadboard_module_library_generated_includes \
  motive_generated_includes \
  fplbase_generated_includes \
  pindrop_generated_includes \
  scene_lab_generated_includes)

.PHONY: clean_generated_includes
clean_generated_includes:
	$(hide) rm -rf $(ZOOSHI_GENERATED_OUTPUT_DIR)
endif

.PHONY: clean
clean: clean_assets clean_generated_includes

LOCAL_STATIC_LIBRARIES := \
  libbreadboard \
  libmodule_library \
  libgpg \
  libmathfu \
  libwebp \
  libfplbase \
  libflatui \
  libpindrop \
  libentity \
  libcomponent_library \
  libscene_lab \
  libmotive \
  libfreetype \
  libharfbuzz \
  libflatbuffers \
  libbullet

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)

PINDROP_ASYNC_LOADING := 1

$(call import-add-path,$(DEPENDENCIES_BREADBOARD_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FPLBASE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FLATUI_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MOTIVE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_SCENE_LAB_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_PINDROP_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_ENTITY_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_WEBP_DIR)/..)

$(call import-module,fplbase/jni)
$(call import-module,entity/jni)
$(call import-module,breadboard/jni)
$(call import-module,breadboard/module_library/jni)
$(call import-module,pindrop/jni)
$(call import-module,flatbuffers/android/jni)
$(call import-module,flatui/jni)
$(call import-module,mathfu/jni)
$(call import-module,motive/jni)
$(call import-module,entity/component_library/jni)
$(call import-module,scene_lab/jni)
$(call import-module,webp)

