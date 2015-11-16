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

#include "fplbase/input.h"
#include "states/states_common.h"

#ifdef ANDROID_HMD
#include "fplbase/renderer_hmd.h"
#endif

#include "flatui/flatui_common.h"

namespace fpl {
namespace zooshi {

#ifdef ANDROID_HMD
static const float kGearSize = 72.0f;

static vec3 CorrectTransform(const mat4& mat) {
  vec3 hmd_translation = (mat * mathfu::kAxisW4f * mat).xyz();
  vec3 corrected_translation =
      vec3(hmd_translation.x(), -hmd_translation.z(), hmd_translation.y());
  return corrected_translation;
}

static void RenderSettingsGear(Renderer& renderer, World* world) {
  vec2i res = renderer.window_size();
  renderer.set_model_view_projection(mathfu::OrthoHelper<float>(
      0.0f, static_cast<float>(res.x()), static_cast<float>(res.y()), 0.0f,
      -1.0f, 1.0f));
  renderer.set_color(mathfu::kOnes4f);
  auto shader = world->asset_manager->LoadShader("shaders/textured");
  world->cardboard_settings_gear->Set(renderer);
  shader->Set(renderer);

  Mesh::RenderAAQuadAlongX(
      vec3((res.x() - kGearSize) / 2.0f, res.y() - kGearSize, 0.0f),
      vec3((res.x() + kGearSize) / 2.0f, res.y(), 0.0f));
}
#endif  // ANDROID_HMD

static void RenderStereoscopic(Renderer& renderer, World* world, Camera& camera,
                               Camera* cardboard_camera,
                               InputSystem* input_system) {
#ifdef ANDROID_HMD
  HeadMountedDisplayViewSettings view_settings;
  HeadMountedDisplayRenderStart(input_system->head_mounted_display_input(),
                                &renderer, mathfu::kZeros4f, true,
                                &view_settings);
  // Update the Cardboard camera with the translation changes from the given
  // transform, which contains the shifts for the eyes.
  const vec3 corrected_translation_left =
    CorrectTransform(view_settings.viewport_transforms[0]);
  const vec3 corrected_translation_right =
    CorrectTransform(view_settings.viewport_transforms[1]);

  cardboard_camera->set_facing(camera.facing());
  cardboard_camera->set_up(camera.up());

  // Set up stereoscopic rendering parameters.
  cardboard_camera->set_stereo(true);
  cardboard_camera->set_position(
      0, camera.position() + corrected_translation_left);
  cardboard_camera->set_viewport(0, view_settings.viewport_extents[0]);
  cardboard_camera->set_position(
      1, camera.position() + corrected_translation_right);
  cardboard_camera->set_viewport(1, view_settings.viewport_extents[1]);

  world->world_renderer->RenderWorld(*cardboard_camera, renderer, world);

  HeadMountedDisplayRenderEnd(&renderer, true);
  RenderSettingsGear(renderer, world);
#else
  (void)renderer;
  (void)world;
  (void)camera;
  (void)cardboard_camera;
  (void)input_system;
#endif  // ANDROID_HMD
}

void RenderWorld(Renderer& renderer, World* world, Camera& camera,
                 Camera* cardboard_camera, InputSystem* input_system) {
  vec2 window_size = vec2(renderer.window_size());
  world->river_component.UpdateRiverMeshes();
  if (world->is_in_cardboard()) {
    window_size.x() = window_size.x() / 2;
    cardboard_camera->set_viewport_resolution(window_size);
  }
  camera.set_viewport_resolution(window_size);
  if (world->is_in_cardboard()) {
    // This takes care of setting/clearing the framebuffer for us.
    RenderStereoscopic(renderer, world, camera, cardboard_camera, input_system);
  } else {
    // Always clear the framebuffer, even though we overwrite it with the
    // skybox, since it's a speedup on tile-based architectures, see .e.g.:
    // http://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-TileBasedArchitectures.pdf
    renderer.ClearFrameBuffer(mathfu::kZeros4f);

    world->world_renderer->RenderWorld(camera, renderer, world);
  }
}

void UpdateMainCamera(Camera* main_camera, World* world) {
  auto player = world->player_component.begin()->entity;
  auto transform_component = &world->transform_component;
  main_camera->set_position(transform_component->WorldPosition(player));
  main_camera->set_facing(
      transform_component->WorldOrientation(player).Inverse() *
      mathfu::kAxisY3f);
  auto player_data = world->entity_manager.GetComponentData<PlayerData>(player);
  auto raft_orientation = transform_component->WorldOrientation(
      world->entity_manager.GetComponent<ServicesComponent>()->raft_entity());
  main_camera->set_up(raft_orientation.Inverse() * player_data->GetUp());
}

}  // zooshi
}  // fpl
