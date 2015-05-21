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

LOCAL_PATH:=$(call my-dir)

# Project directory relative to this file.
ZOOSHI_RELATIVE_DIR:=../../..
ZOOSHI_DIR:=$(LOCAL_PATH)/$(ZOOSHI_RELATIVE_DIR)
include $(ZOOSHI_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

# Build rule which builds assets for the game.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
.PHONY: build_assets
build_assets: $(flatc_target)
	cp -f -r $(DEPENDENCIES_IMGUI_DIR)/assets $(ZOOSHI_DIR)
	$(hide) python $(ZOOSHI_DIR)/scripts/build_assets.py

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
  $(DEPENDENCIES_FPLBASE_DIR)/include \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_GPG_DIR)/include \
  $(DEPENDENCIES_SDL_DIR) \
  $(DEPENDENCIES_SDL_DIR)/include \
  $(DEPENDENCIES_SDL_MIXER_DIR) \
  $(DEPENDENCIES_WEBP_DIR)/src \
  $(ZOOSHI_GENERATED_OUTPUT_DIR) \
  src

LOCAL_SRC_FILES := \
  $(subst $(LOCAL_PATH)/,,$(DEPENDENCIES_SDL_DIR))/src/main/android/SDL_android_main.c \
  $(ZOOSHI_RELATIVE_DIR)/src/camera.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/game.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/game_state.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/main.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/audio_listener.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/patron.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/physics.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/player.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/player_projectile.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/rail_denizen.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/rendermesh.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/sound.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/time_limit.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/components/transform.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/editor/world_editor.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/entity/entity_manager.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/event_system/event_manager.cpp \
  $(ZOOSHI_RELATIVE_DIR)/src/inputcontrollers/android_cardboard_controller.cpp

ZOOSHI_SCHEMA_DIR := $(ZOOSHI_DIR)/src/flatbufferschemas

ZOOSHI_SCHEMA_FILES := \
  $(ZOOSHI_SCHEMA_DIR)/assets.fbs \
  $(ZOOSHI_SCHEMA_DIR)/components.fbs \
  $(ZOOSHI_SCHEMA_DIR)/config.fbs \
  $(ZOOSHI_SCHEMA_DIR)/world_editor.fbs \
  $(ZOOSHI_SCHEMA_DIR)/input_config.fbs \
  $(ZOOSHI_SCHEMA_DIR)/rail_def.fbs

# Make each source file dependent upon the assets
$(foreach src,$(LOCAL_SRC_FILES),$(eval $(LOCAL_PATH)/$$(src): build_assets))

ifeq (,$(ZOOSHI_RUN_ONCE))
ZOOSHI_RUN_ONCE := 1
$(call flatbuffers_header_build_rules,\
  $(ZOOSHI_SCHEMA_FILES),\
  $(ZOOSHI_SCHEMA_DIR),\
  $(ZOOSHI_GENERATED_OUTPUT_DIR),\
  $(DEPENDENCIES_PINDROP_DIR)/schemas $(DEPENDENCIES_MOTIVE_DIR)/schemas \
    $(DEPENDENCIES_FPLBASE_DIR)/schemas,\
  $(LOCAL_SRC_FILES))

.PHONY: clean_generated_includes
clean_generated_includes:
	$(hide) rm -rf $(ZOOSHI_GENERATED_OUTPUT_DIR)
endif

.PHONY: clean
clean: clean_assets clean_generated_includes

LOCAL_STATIC_LIBRARIES := \
  libgpg \
  libmathfu \
  libwebp \
  SDL2 \
  SDL2_mixer \
  libfplbase \
  libimgui \
  libpindrop \
  libmotive \
  libfreetype \
  libharfbuzz \
  libflatbuffers

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_FPLBASE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_IMGUI_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MOTIVE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_PINDROP_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_WEBP_DIR)/..)

$(call import-module,audio_engine/jni)
$(call import-module,flatbuffers/android/jni)
$(call import-module,fplbase/jni)
$(call import-module,imgui/jni)
$(call import-module,mathfu/jni)
$(call import-module,motive/jni)
$(call import-module,webp)

