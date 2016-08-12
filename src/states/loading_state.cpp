// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "states/loading_state.h"

#include <cmath>

#include "assets_generated.h"
#include "camera.h"
#include "fplbase/asset_manager.h"
#include "fplbase/glplatform.h"
#include "fplbase/input.h"
#include "fplbase/material.h"
#if ANDROID_HMD
#include "fplbase/renderer_hmd.h"
#endif // ANDROID_HMD
#include "fplbase/shader.h"
#include "fplbase/utilities.h"
#include "full_screen_fader.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix.h"
#include "mathfu/quaternion.h"
#include "mathfu/vector_4.h"
#include "pindrop/pindrop.h"
#include "states/states.h"
#include "states/states_common.h"
#include "world.h"

#define ZOOSHI_WAIT_ON_LOADING_SCREEN 0

namespace fpl {
namespace zooshi {

using mathfu::kOnes3f;
using mathfu::kOnes4f;
using mathfu::kZeros3f;
using mathfu::kZeros4f;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;

static const corgi::WorldTime kLoadingScreenFadeInTime = 400;
static const corgi::WorldTime kLoadingScreenFadeOutTime = 200;

void LoadingState::Initialize(fplbase::InputSystem* input_system, World* world,
                              const AssetManifest& asset_manifest,
                              fplbase::AssetManager* asset_manager,
                              pindrop::AudioEngine* audio_engine,
                              fplbase::Shader* shader_textured,
                              FullScreenFader* fader) {
  input_system_ = input_system;
  world_ = world;
  asset_manager_ = asset_manager;
  audio_engine_ = audio_engine;
  asset_manifest_ = &asset_manifest;
  shader_textured_ = shader_textured;
  loading_complete_ = false;
  fader_ = fader;
}

void LoadingState::AdvanceFrame(int delta_time, int* next_state) {
  const bool fade_out_complete = fader_->AdvanceFrame(delta_time);
#if ZOOSHI_WAIT_ON_LOADING_SCREEN
  loading_complete_ = false;
#endif  // !ZOOSHI_WAIT_ON_LOADING_SCREEN

  // Exit state when everything has finished loading.
  if (loading_complete_ && fade_out_complete) {
    *next_state = kGameStateGameMenu;
  }

#ifdef ANDROID_HMD
  // Get the direction vector from the HMD input.
  const fplbase::HeadMountedDisplayInput& head_mounted_display_input =
    input_system_->head_mounted_display_input();
  const vec3 hmd_forward = head_mounted_display_input.forward();
  const vec2 forward = vec2(hmd_forward.x(), -hmd_forward.z()).Normalized();
  const float rotation = atan2(forward.x(), forward.y());
  // Rate of convergence (angle moved = 0.5 * convergence * time) to the
  // target rotation angle.
  static const float kConvergenceRate = 2.0f;
  banner_rotation_ += (((M_PI * 2) - rotation) - banner_rotation_) *
    kConvergenceRate * std::min(delta_time / 1000.0f, 1.0f);
#endif  // ANDROID_HMD

}

void LoadingState::Render(fplbase::Renderer* renderer) {
  // Ensure assets are instantiated after they've been loaded.
  // This must be called from the render thread.
  loading_complete_ =
      asset_manager_->TryFinalize() && audio_engine_->TryFinalize();

  // Get a handle to the loading material.
  const char* loading_material_name =
      asset_manifest_->loading_material()->c_str();
  fplbase::Material* loading_material =
      asset_manager_->FindMaterial(loading_material_name);

  // Render black until the loading material itself has loaded.
  auto texture = loading_material->textures()[0];
  if (!texture->id() ||
      (world_->is_in_cardboard() &&
       !world_->cardboard_settings_gear->textures()[0]->id())) {
    renderer->ClearFrameBuffer(kZeros4f);
    return;
  }

  const vec2 res(renderer->window_size());
  const float aspect_ratio = res.x() / res.y();
  if (world_->is_in_cardboard()) {
#if ANDROID_HMD
    fplbase::Shader *shader_textured = shader_textured_;
    fplbase::HeadMountedDisplayViewSettings view_settings;
    HeadMountedDisplayRenderStart(input_system_->head_mounted_display_input(),
                                  renderer, mathfu::kZeros4f, true,
                                  &view_settings);
    // Create rotation matrix for banner based upon banner_angle_
    static const float kAngle = M_PI / 2.0f;  // Around X to face the camera.
    const mat4 banner_transform = mat4::FromRotationMatrix(
      mat3::RotationX(kAngle) * mat3::RotationY(banner_rotation_));
    for (auto i = 0; i < 2; ++i) {
      renderer->set_camera_pos(kZeros3f);
      Camera camera;
      renderer->set_model_view_projection(
          camera.GetTransformMatrix() * banner_transform *
          view_settings.viewport_transforms[i]);
      renderer->set_color(kOnes4f);
      loading_material->Set(*renderer);
      shader_textured->Set(*renderer);
      auto vp = view_settings.viewport_extents[i];
      glViewport(vp.x(), vp.y(), vp.z(), vp.w());

      // Render the loading banner floating in front of the viewer.
      const vec2 size = vec2(texture->size()).Normalized() * 50.0f;
      // Place the banner just in front and behind the camera.
      static const float kDistance = -120.0f;
      const vec3 bottom_left(-size.x(), size.y(), kDistance);
      const vec3 top_right(size.x(), -size.y(), kDistance);
      fplbase::Mesh::RenderAAQuadAlongX(bottom_left, top_right);
    }
    HeadMountedDisplayRenderEnd(renderer, true);
#endif // ANDROID_HMD
  } else {
    // Always clear the background.
    renderer->ClearFrameBuffer(kOnes4f);

    // In ortho space:
    //  - screen centered at (0,0)
    //  - unit of 1 is distance from middle to top of screen
    //  - bottom left is (-aspect_ratio, -1)
    //  - top right is (aspect_ratio, +1)
    //  - horizontal and vertical units are same distance on screen
    renderer->set_model_view_projection(
        mat4::Ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f));

    // Set up rendering context.
    renderer->set_color(kOnes4f);
    loading_material->Set(*renderer);
    shader_textured_->Set(*renderer);

    // Only scale the image by 2, 1, or 0.5 so that it remains crisp.
    // We assume the loading texture is square and that the screen is wider
    // than it is high, so only check height.
    const int window_y = renderer->window_size().y();
    const int image_y = texture->original_size().y();
    const float scale = image_y * 2 <= window_y ? 2.0f :
                        image_y > window_y ? 0.5f : 1.0f;

    // Render the overlay in front on the screen.
    const vec2 size = scale * vec2(texture->size()) / res.y();
    const vec3 bottom_left(-size.x(), size.y(), 0.0f);
    const vec3 top_right(size.x(), -size.y(), 0.0f);
    fplbase::Mesh::RenderAAQuadAlongX(bottom_left, top_right);
  }

  const vec3 fade_bottom_left(-aspect_ratio, 1.0f, 0.0f);
  const vec3 fade_top_right(aspect_ratio, -1.0f, 0.0f);
  if (fader_->current_fade_time() == 0) {
    // If this is the first frame the textures have been loaded, start fade-in.
    fader_->Start(kLoadingScreenFadeInTime, kZeros3f, kFadeIn, fade_bottom_left,
                  fade_top_right);
  } else if (loading_complete_ && fader_->Finished()) {
    // If this is the first frame the textures have been loaded, start fade-out.
    fader_->Start(kLoadingScreenFadeOutTime, kZeros3f, kFadeOutThenIn,
                  fade_bottom_left, fade_top_right);
  }

  // Draw fader on top of everything.
  if (!fader_->Finished()) {
    fader_->Render(renderer);
  }
}

void LoadingState::OnEnter(int /*previous_state*/) {
#ifdef ANDROID_HMD
  input_system_->head_mounted_display_input().ResetHeadTracker();
#endif  // ANDROID_HMD
}


}  // zooshi
}  // fpl
