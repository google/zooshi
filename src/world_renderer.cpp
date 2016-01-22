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

#include "world_renderer.h"

#include "components/light.h"
#include "components/services.h"
#include "corgi_component_library/transform.h"
#include "fplbase/flatbuffer_utils.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat3;
using mathfu::mat4;
using mathfu::quat;

namespace fpl {
namespace zooshi {

using corgi::component_library::TransformData;
using corgi::EntityRef;

static const int kShadowMapTextureID = 7;
// 45 degrees in radians:
static const float kShadowMapViewportAngle = 0.7853975f;
static const vec4 kShadowMapClearColor = vec4(0.99f, 0.99f, 0.99f, 1.0f);
static const float kShadowMapBias = 0.007f;

void WorldRenderer::Initialize(World* world) {
  int shadow_map_resolution =
      world->config->rendering_config()->shadow_map_resolution();
  shadow_map_.Initialize(
      mathfu::vec2i(shadow_map_resolution, shadow_map_resolution));
  depth_shader_ = world->asset_manager->LoadShader("shaders/render_depth");
  depth_skinned_shader_ =
      world->asset_manager->LoadShader("shaders/render_depth_skinned");
  textured_shader_ = world->asset_manager->LoadShader("shaders/textured");
  textured_shadowed_shader_ =
      world->asset_manager->LoadShader("shaders/textured_shadowed");
  textured_lit_shader_ =
      world->asset_manager->LoadShader("shaders/textured_lit");
  textured_lit_bank_shader_ =
      world->asset_manager->LoadShader("shaders/textured_lit_bank");
  textured_shadowed_bank_shader_ =
      world->asset_manager->LoadShader("shaders/textured_shadowed_bank");
  textured_skinned_lit_shader_ =
      world->asset_manager->LoadShader("shaders/textured_skinned_lit");
  textured_skinned_shadowed_shader_ =
      world->asset_manager->LoadShader("shaders/textured_skinned_shadowed");
  textured_lit_cutout_shader_ =
      world->asset_manager->LoadShader("shaders/textured_lit_cutout");
  river_shader_ =
      world->asset_manager->LoadShader("shaders/origwater");
}

void WorldRenderer::CreateShadowMap(const corgi::CameraInterface& camera,
                                    fplbase::Renderer& renderer, World* world) {
  float shadow_map_resolution = static_cast<float>(
        world->config->rendering_config()->shadow_map_resolution());
  float shadow_map_zoom = world->config->rendering_config()->shadow_map_zoom();
  float shadow_map_offset =
      world->config->rendering_config()->shadow_map_offset();
  LightComponent* light_component =
      world->entity_manager.GetComponent<LightComponent>();

  const EntityRef& main_light_entity = light_component->begin()->entity;
  const TransformData* light_transform =
      world->entity_manager.GetComponentData<TransformData>(main_light_entity);
  vec3 light_position = light_transform->position;
  SetLightPosition(light_position);

  light_camera_.set_viewport_angle(kShadowMapViewportAngle / shadow_map_zoom);
  light_camera_.set_viewport_resolution(
      vec2(shadow_map_resolution, shadow_map_resolution));
  vec3 light_camera_focus =
      camera.position() + camera.facing() * shadow_map_offset;
  light_camera_focus.z() = 0;
  vec3 light_facing = light_camera_focus - light_camera_.position();
  light_camera_.set_facing(light_facing.Normalized());

  // Shadow map needs to be cleared to near-white, since that's
  // the maximum (furthest) depth.
  shadow_map_.SetAsRenderTarget();
  renderer.ClearFrameBuffer(kShadowMapClearColor);
  renderer.SetCulling(fplbase::Renderer::kCullBack);

  depth_shader_->Set(renderer);
  depth_skinned_shader_->Set(renderer);
  // Generate the shadow map:
  // TODO - modify this so that shadowcast is its own render pass

  for (int pass = 0; pass < corgi::RenderPass_Count; pass++) {
    world->render_mesh_component.RenderPass(pass, light_camera_,
                                            renderer, ShaderIndex_Depth);
  }

  fplbase::RenderTarget::ScreenRenderTarget(renderer).SetAsRenderTarget();
}

void WorldRenderer::RenderPrep(const corgi::CameraInterface& camera,
                               World* world) {
  world->render_mesh_component.RenderPrep(camera);
}

// Draw the shadow map in the world, so we can see it.
void WorldRenderer::DebugShowShadowMap(const corgi::CameraInterface& camera,
                                       fplbase::Renderer& renderer) {
  fplbase::RenderTarget::ScreenRenderTarget(renderer).SetAsRenderTarget();

  static const mat4 kDebugTextureWorldTransform =
      mat4::FromScaleVector(mathfu::vec3(10.0f, 10.0f, 10.0f));

  const mat4 mvp = camera.GetTransformMatrix() * kDebugTextureWorldTransform;
  const mat4 world_matrix_inverse = kDebugTextureWorldTransform.Inverse();

  renderer.set_camera_pos(world_matrix_inverse * camera.position());
  renderer.set_light_pos(world_matrix_inverse * light_camera_.position());
  renderer.set_model_view_projection(mvp);
  renderer.set_color(vec4(1.0f, 1.0f, 1.0f, 1.0f));

  shadow_map_.BindAsTexture(0);

  textured_shader_->Set(renderer);

  // Render a large quad in the world, with the shadowmap texture on it:
  fplbase::Mesh::RenderAAQuadAlongX(vec3(0.0f, 0.0f, 0.0f),
                                    vec3(10.0f, 0.0f, 10.0f),
                                    vec2(1.0f, 0.0f), vec2(0.0f, 1.0f));
}

void WorldRenderer::SetFogUniforms(fplbase::Shader* shader, World* world) {
  shader->SetUniform("fog_roll_in_dist",
                     world->config->rendering_config()->fog_roll_in_dist());
  shader->SetUniform("fog_max_dist",
                     world->config->rendering_config()->fog_max_dist());
  shader->SetUniform(
      "fog_color",
      LoadColorRGBA(world->config->rendering_config()->fog_color()));
  shader->SetUniform("fog_max_saturation",
                     world->config->rendering_config()->fog_max_saturation());
}

void WorldRenderer::SetLightingUniforms(fplbase::Shader* shader, World* world) {
  LightComponent* light_component =
      world->entity_manager.GetComponent<LightComponent>();
  const EntityRef& main_light_entity = light_component->begin()->entity;
  const LightData* light_data =
      world->entity_manager.GetComponentData<LightData>(main_light_entity);

  if (world->config->rendering_config()->render_shadows()) {
    shader->SetUniform("shadow_intensity", light_data->shadow_intensity);
  }
  shader->SetUniform("ambient_material", light_data->ambient_color);
  shader->SetUniform("diffuse_material", light_data->diffuse_color);
  shader->SetUniform("specular_material", light_data->specular_color);
  shader->SetUniform("shininess", light_data->specular_exponent);
}

void WorldRenderer::RenderWorld(const corgi::CameraInterface& camera,
                                fplbase::Renderer& renderer, World* world) {
  if (world->config->rendering_config()->create_shadow_map()) {
    CreateShadowMap(camera, renderer, world);
  }

  mat4 camera_transform = camera.GetTransformMatrix();
  renderer.set_color(mathfu::kOnes4f);
  renderer.DepthTest(true);
  renderer.set_model_view_projection(camera_transform);

  assert(textured_shadowed_shader_);
  textured_shadowed_shader_->SetUniform("view_projection", camera_transform);
  textured_shadowed_shader_->SetUniform("light_view_projection",
                                        light_camera_.GetTransformMatrix());

  assert(textured_shadowed_bank_shader_);
  textured_shadowed_bank_shader_->SetUniform("view_projection",
                                             camera_transform);
  textured_shadowed_bank_shader_->SetUniform(
      "light_view_projection",
      light_camera_.GetTransformMatrix());

  assert(textured_skinned_shadowed_shader_);
  textured_skinned_shadowed_shader_->SetUniform("view_projection",
                                             camera_transform);
  textured_skinned_shadowed_shader_->SetUniform(
      "light_view_projection",
      light_camera_.GetTransformMatrix());

  float texture_repeats = world->config->river_config()->texture_repeats();
  float river_offset = world->river_component.river_offset();

  river_shader_->SetUniform("river_offset",   river_offset);
  river_shader_->SetUniform("texture_repeats", texture_repeats);

  depth_shader_->SetUniform("bias", kShadowMapBias);
  depth_skinned_shader_->SetUniform("bias", kShadowMapBias);

  if (world->config->rendering_config()->render_shadows()) {
    SetLightingUniforms(textured_shadowed_shader_, world);
    SetLightingUniforms(textured_shadowed_bank_shader_, world);
    SetLightingUniforms(textured_skinned_shadowed_shader_, world);
    SetLightingUniforms(textured_lit_cutout_shader_, world);

    SetFogUniforms(textured_shadowed_shader_, world);
    SetFogUniforms(textured_shadowed_bank_shader_, world);
    SetFogUniforms(textured_skinned_shadowed_shader_, world);
  } else {
    SetLightingUniforms(textured_lit_shader_, world);
    SetLightingUniforms(textured_lit_bank_shader_, world);
    SetLightingUniforms(textured_skinned_lit_shader_, world);
    SetLightingUniforms(textured_lit_cutout_shader_, world);

    SetFogUniforms(textured_lit_shader_, world);
    SetFogUniforms(textured_lit_bank_shader_, world);
    SetFogUniforms(textured_skinned_lit_shader_, world);
  }

  shadow_map_.BindAsTexture(kShadowMapTextureID);

  size_t shader_index = (world->config->rendering_config()->render_shadows())
                         ? ShaderIndex_Shadowed : ShaderIndex_Lit;

  if (!world->skip_rendermesh_rendering) {
    for (int pass = 0; pass < corgi::RenderPass_Count; pass++) {
      world->render_mesh_component.RenderPass(pass, camera, renderer,
                                              shader_index);
    }
  }

  if (world->draw_debug_physics) {
    world->physics_component.DebugDrawWorld(&renderer, camera_transform);
  }

  world->render_3d_text_component.RenderAllEntities(camera);
}

}  // zooshi
}  // fpl
