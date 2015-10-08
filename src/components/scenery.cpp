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

#include "components/scenery.h"

#include <vector>
#include "component_library/animation.h"
#include "component_library/rendermesh.h"
#include "component_library/transform.h"
#include "components_generated.h"
#include "components/services.h"
#include "mathfu/glsl_mappings.h"
#include "world.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::zooshi::SceneryComponent,
                            fpl::zooshi::SceneryData)

namespace fpl {
namespace zooshi {

using fpl::component_library::AnimationComponent;
using fpl::component_library::AnimationData;
using fpl::component_library::RenderMeshComponent;
using fpl::component_library::TransformComponent;
using fpl::component_library::TransformData;
using fpl::entity::EntityRef;
using fpl::editor::WorldEditor;
using mathfu::vec3;
using mathfu::kZeros3f;

void SceneryComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();

  // World editor is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a world editor.
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  WorldEditor* world_editor = services->world_editor();
  if (world_editor) {
    world_editor->AddOnEnterEditorCallback([this]() { ShowAll(true); });
    world_editor->AddOnExitEditorCallback([this]() { PostLoadFixup(); });
  }
}

void SceneryComponent::AddFromRawData(entity::EntityRef& scenery,
                                      const void* raw_data) {
  auto scenery_def = static_cast<const SceneryDef*>(raw_data);
  SceneryData* scenery_data = AddEntity(scenery);
  scenery_data->anim_object = scenery_def->anim_object();
}

entity::ComponentInterface::RawDataUniquePtr SceneryComponent::ExportRawData(
    const entity::EntityRef& /*scenery*/) const {
  return nullptr;
}

void SceneryComponent::InitEntity(entity::EntityRef& /*scenery*/) {}

void SceneryComponent::PostLoadFixup() {
  const TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();

  // Initialize each scenery.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef scenery = iter->entity;

    // Get reference to the first child with a rendermesh. We assume there will
    // only be one such child.
    SceneryData* scenery_data = Data<SceneryData>(scenery);
    scenery_data->render_child = transform_component->ChildWithComponent(
        scenery, RenderMeshComponent::GetComponentId());
    assert(scenery_data->render_child);

    // Add animation to the entity with the rendermesh.
    entity_manager_->AddEntityToComponent<AnimationComponent>(
        scenery_data->render_child);
    AnimationData* animation_data =
        Data<AnimationData>(scenery_data->render_child);
    animation_data->anim_table_object = scenery_data->anim_object;

    // Everything starts off-screen.
    scenery_data->state = kSceneryHide;

    // Ensure all scenery starts hidden.
    Show(scenery, false);
  }
}

vec3 SceneryComponent::CenterPosition() const {
  const entity::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  return raft ? Data<TransformData>(raft)->position : kZeros3f;
}

float SceneryComponent::PopInDistSq() const {
  auto render_config = config_->rendering_config();
  const float dist = render_config->pop_in_distance();
  return dist * dist;
}

float SceneryComponent::PopOutDistSq() const {
  auto render_config = config_->rendering_config();
  const float dist = render_config->pop_out_distance();
  return dist * dist;
}

float SceneryComponent::DistSq(
    const entity::EntityRef& scenery, const vec3& center) const {
  const TransformData* transform_data = Data<TransformData>(scenery);
  return (transform_data->position - center).LengthSquared();
}

float SceneryComponent::AnimTimeRemaining(
    const entity::EntityRef& scenery) const {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  const AnimationData* anim_data =
      Data<AnimationData>(scenery_data->render_child);
  return anim_data->motivator.Valid() ?
         anim_data->motivator.TimeRemaining() : 0.0f;
}

bool SceneryComponent::HasAnim(const SceneryData* scenery_data,
                               SceneryState state) const {
  return entity_manager_->GetComponent<AnimationComponent>()->HasAnim(
      scenery_data->render_child, state);
}

SceneryState SceneryComponent::NextState(
    const entity::EntityRef& scenery, const vec3& center) const {

  // Check for state transitions.
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  switch (scenery_data->state) {
    case kSceneryHide:
      if (DistSq(scenery, center) < PopInDistSq()) {
        return kSceneryAppear;
      }
      break;

    case kSceneryShow:
      if (DistSq(scenery, center) > PopOutDistSq()) {
        return kSceneryDisappear;
      }
      break;

    case kSceneryAppear:
      if (AnimTimeRemaining(scenery) <= 0.0f) {
        return kSceneryShow;
      }
      break;

    case kSceneryDisappear:
      if (AnimTimeRemaining(scenery) <= 0.0f) {
        return kSceneryHide;
      }
      break;
  }

  // No transition.
  return scenery_data->state;
}

void SceneryComponent::Animate(const entity::EntityRef& scenery,
                               SceneryState state) {
  AnimationComponent* anim_component =
      entity_manager_->GetComponent<AnimationComponent>();
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  anim_component->AnimateFromTable(scenery_data->render_child, state);
}

void SceneryComponent::StopAnimating(const entity::EntityRef& scenery) {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  AnimationData* anim_data =
      Data<AnimationData>(scenery_data->render_child);
  if (anim_data->motivator.Valid()) {
    anim_data->motivator.Invalidate();
  }
}

void SceneryComponent::Show(const entity::EntityRef& scenery, bool show) {
  TransformComponent* tf_component =
      entity_manager_->GetComponent<TransformComponent>();
  RenderMeshComponent* rm_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  const entity::EntityRef parent = tf_component->GetRootParent(scenery);
  rm_component->SetHiddenRecursively(parent, !show);
}

void SceneryComponent::ShowAll(bool show) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef scenery = iter->entity;
    Show(scenery, show);
  }
}

void SceneryComponent::TransitionState(const entity::EntityRef& scenery,
                                       SceneryState next_state) {

  SceneryData* scenery_data = Data<SceneryData>(scenery);

  // Hide or show the rendermesh.
  if (next_state == kSceneryHide || next_state == kSceneryAppear) {
    Show(scenery, next_state == kSceneryAppear);
  }

  // Animate or stop animating the scene.
  if (HasAnim(scenery_data, next_state)) {
    Animate(scenery, next_state);
  } else {
    StopAnimating(scenery);
  }

  // Update the state variable.
  scenery_data->state = next_state;
}

void SceneryComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  const vec3 center = CenterPosition();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef scenery = iter->entity;
    const SceneryData* scenery_data = Data<SceneryData>(scenery);

    // Execute state machine for each piece of scenery.
    const SceneryState next_state = NextState(scenery, center);
    if (scenery_data->state != next_state) {
      TransitionState(scenery, next_state);
    }
  }
}

}  // zooshi
}  // fpl
