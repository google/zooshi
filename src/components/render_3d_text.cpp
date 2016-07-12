// Copyright 2016 Google Inc. All rights reserved.
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

#include "components/render_3d_text.h"

#include "components/attributes.h"
#include "components/player.h"
#include "components_generated.h"
#include "corgi_component_library/animation.h"
#include "corgi_component_library/rendermesh.h"
#include "fplbase/flatbuffer_utils.h"
#include "motive/math/angle.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::Render3dTextComponent,
                       fpl::zooshi::Render3dTextData)

using corgi::component_library::AnimationData;
using corgi::component_library::RenderMeshData;
using corgi::component_library::RenderMeshComponent;
using corgi::component_library::TransformData;
using corgi::EntityRef;
using mathfu::mat4;
using mathfu::vec2i;
using mathfu::vec3;
using motive::kDegreesToRadians;

namespace fpl {
namespace zooshi {

void Render3dTextComponent::AddFromRawData(EntityRef& entity,
                                           const void* raw_data) {
  auto render_3d_text_def = static_cast<const Render3dTextDef*>(raw_data);
  Render3dTextData* render_3d_text_data = AddEntity(entity);
  render_3d_text_data->animation_bone = render_3d_text_def->animation_bone();
  render_3d_text_data->canvas_size = render_3d_text_def->canvas_size();
  render_3d_text_data->font = render_3d_text_def->font()->c_str();
  render_3d_text_data->label_size = render_3d_text_def->label_size();
  render_3d_text_data->translation =
      LoadVec3(render_3d_text_def->translation());
  render_3d_text_data->rotation = LoadVec3(render_3d_text_def->rotation());
  render_3d_text_data->scale = LoadVec3(render_3d_text_def->scale());
  render_3d_text_data->text = render_3d_text_def->text()->c_str();
}

const mat4 Render3dTextComponent::CalculateAnimationTransform(
    const EntityRef& entity, const int animation_bone) const {
  const AnimationData* anim_data = Data<AnimationData>(entity);
  const bool has_anim = anim_data != nullptr && anim_data->motivator.Valid();
  const int num_anim_bones =
      has_anim ? anim_data->motivator.DefiningAnim()->NumBones() : 0;

  if (has_anim && num_anim_bones > animation_bone) {
    return mat4::FromAffineTransform(
        anim_data->motivator.GlobalTransforms()[animation_bone]);
  }

  const RenderMeshData* rendermesh_data = Data<RenderMeshData>(entity);
  const int num_mesh_bones =
      static_cast<int>(rendermesh_data->mesh->num_bones());

  if (num_mesh_bones > 1 && num_mesh_bones > animation_bone) {
    return mat4::FromAffineTransform(
        rendermesh_data->mesh->bone_global_transforms()[animation_bone]);
  }

  return mat4::Identity();
}

const mat4 Render3dTextComponent::CalculateModelViewProjection(
    const EntityRef& entity, const corgi::CameraInterface& camera) const {
  const TransformData* transform_data = Data<TransformData>(entity);
  const Render3dTextData* render_3d_text_data = Data<Render3dTextData>(entity);

  const vec2i window_size =
      services_->asset_manager()->renderer().window_size();
  const float aspect_ratio =
      static_cast<float>(window_size.x()) / static_cast<float>(window_size.y());

  // Calculate the animation transform.
  const mat4 anim_transform =
      CalculateAnimationTransform(entity, render_3d_text_data->animation_bone);

  // Get the world transform (M) from the Entity, and adjust it by the animation
  // transform.
  const mat4 world_transform = transform_data->world_transform * anim_transform;

  // Center the FlatUI canvas at 0,0 (camera origin).
  const mat4 center_at_origin = mat4::FromTranslationVector(
      vec3(render_3d_text_data->canvas_size * aspect_ratio / -2.0f,
           render_3d_text_data->canvas_size / -2.0f, 0.0f));

  // Move the text from the camera origin (0,0) to its location in world space.
  const mat4 translation =
      mat4::FromTranslationVector(vec3(render_3d_text_data->translation));

  // Rotate the text to face the camera.
  const mat4 orientation = mat4::FromRotationMatrix(
      mathfu::quat::FromEulerAngles(
          vec3(render_3d_text_data->rotation.data[0] * kDegreesToRadians,
               render_3d_text_data->rotation.data[1] * kDegreesToRadians,
               render_3d_text_data->rotation.data[2] * kDegreesToRadians))
          .ToMatrix());

  // Scale the FlatUI down to the correct size for the entity.
  const mat4 scale = mat4::FromScaleVector(vec3(render_3d_text_data->scale));

  // Calculate MVP -> Perspective * View * Model * Translation * Rotation *
  //                  Scale * Center At Origin.
  return camera.GetTransformMatrix() * world_transform * translation *
         orientation * scale * center_at_origin;
}

void Render3dTextComponent::Init() {
  services_ = entity_manager_->GetComponent<ServicesComponent>();
}

void Render3dTextComponent::InitEntity(EntityRef& entity) {
  entity_manager_->AddEntityToComponent<RenderMeshComponent>(entity);
}

void Render3dTextComponent::Render(const EntityRef& entity,
                                   const corgi::CameraInterface& camera) {
  SetModelViewProjectionMatrix(entity, camera);

  const RenderMeshData* rendermesh_data = Data<RenderMeshData>(entity);
  const Render3dTextData* render_3d_text_data = Data<Render3dTextData>(entity);

  if (rendermesh_data && rendermesh_data->visible) {
    // Create FlatUI in 3D space.
    flatui::Run(*services_->asset_manager(), *services_->font_manager(),
                *services_->input_system(), [&]() {
                  const vec2i window_size =
                      services_->asset_manager()->renderer().window_size();
                  const float aspect_ratio =
                      static_cast<float>(window_size.x()) /
                      static_cast<float>(window_size.y());

                  flatui::SetDepthTest(true);
                  flatui::UseExistingProjection(
                      vec2i(static_cast<int>(render_3d_text_data->canvas_size *
                                             aspect_ratio),
                            render_3d_text_data->canvas_size));
                  flatui::StartGroup(flatui::kLayoutOverlay);
                  {
                    flatui::PositionGroup(flatui::kAlignCenter,
                                          flatui::kAlignCenter,
                                          mathfu::kZeros2f);
                    {
                      flatui::SetTextFont(render_3d_text_data->font.c_str());
                      flatui::Label(render_3d_text_data->text.c_str(),
                                    render_3d_text_data->label_size);
                    }
                  }
                  flatui::EndGroup();
                });
  }
}

void Render3dTextComponent::RenderAllEntities(
    const corgi::CameraInterface& camera) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    Render(iter->entity, camera);
  }
}

void Render3dTextComponent::SetModelViewProjectionMatrix(
    const EntityRef& entity, const corgi::CameraInterface& camera) {
  const mat4 mvp = CalculateModelViewProjection(entity, camera);
  services_->asset_manager()->renderer().set_model_view_projection(mvp);
}

}  // zooshi
}  // fpl
