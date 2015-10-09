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

#ifdef ANDROID_CARDBOARD
#include "fplbase/renderer_hmd.h"
#endif

namespace fpl {
namespace zooshi {

static const vec4 kGreenishColor(0.05f, 0.2f, 0.1f, 1.0f);

static void RenderStereoscopic(Renderer& renderer, World* world, Camera& camera,
                               Camera* cardboard_camera,
                               InputSystem* input_system) {
#ifdef ANDROID_CARDBOARD
  auto render_callback = [&renderer, world, &camera, cardboard_camera](
      const mat4& hmd_viewport_transform) {
    // Update the Cardboard camera with the translation changes from the given
    // transform, which contains the shifts for the eyes.
    const vec3 hmd_translation = (hmd_viewport_transform * mathfu::kAxisW4f *
                                  hmd_viewport_transform).xyz();
    const vec3 corrected_translation =
        vec3(hmd_translation.x(), -hmd_translation.z(), hmd_translation.y());
    cardboard_camera->set_position(camera.position() + corrected_translation);
    cardboard_camera->set_facing(camera.facing());
    cardboard_camera->set_up(camera.up());

    world->world_renderer->RenderWorld(*cardboard_camera, renderer, world);
  };

  HeadMountedDisplayRender(input_system, &renderer, kGreenishColor,
                           render_callback, true);
#else
  (void)renderer;
  (void)world;
  (void)camera;
  (void)cardboard_camera;
  (void)input_system;
#endif  // ANDROID_CARDBOARD
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

gui::Event ImageButtonWithLabel(const Texture& tex, float size,
                                const gui::Margin& margin, const char* label) {
  gui::StartGroup(gui::kLayoutVerticalLeft, size, "ImageButtonWithLabel");
  gui::SetMargin(margin);
  auto event = gui::CheckEvent();
  gui::ImageBackground(tex);
  gui::Label(label, size);
  gui::EndGroup();
  return event;
}

}  // zooshi
}  // fpl
