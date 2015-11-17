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
#include "components/services.h"
#include "components_generated.h"
#include "corgi_component_library/animation.h"
#include "corgi_component_library/rendermesh.h"
#include "corgi_component_library/transform.h"
#include "mathfu/glsl_mappings.h"
#include "world.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::SceneryComponent, fpl::zooshi::SceneryData)

namespace fpl {
namespace zooshi {

using corgi::component_library::AnimationComponent;
using corgi::component_library::AnimationData;
using corgi::component_library::RenderMeshComponent;
using corgi::component_library::TransformComponent;
using corgi::component_library::TransformData;
using corgi::EntityRef;
using scene_lab::SceneLab;
using mathfu::vec3;
using mathfu::kZeros3f;

void SceneryComponent::Init() {
  config_ = entity_manager_->GetComponent<ServicesComponent>()->config();

  // Scene Lab is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a Scene Lab.
  auto services = entity_manager_->GetComponent<ServicesComponent>();
  SceneLab* scene_lab = services->scene_lab();
  if (scene_lab) {
    scene_lab->AddOnEnterEditorCallback([this]() { ShowAll(true); });
    scene_lab->AddOnExitEditorCallback([this]() { PostLoadFixup(); });
  }
}

void SceneryComponent::AddFromRawData(corgi::EntityRef& scenery,
                                      const void* raw_data) {
  auto scenery_def = static_cast<const SceneryDef*>(raw_data);
  SceneryData* scenery_data = AddEntity(scenery);
  scenery_data->anim_object = scenery_def->anim_object();
}

corgi::ComponentInterface::RawDataUniquePtr SceneryComponent::ExportRawData(
    const corgi::EntityRef& /*scenery*/) const {
  return nullptr;
}

void SceneryComponent::InitEntity(corgi::EntityRef& /*scenery*/) {}

void SceneryComponent::PostLoadFixup() {
  const TransformComponent* transform_component =
      entity_manager_->GetComponent<TransformComponent>();

  // Initialize each scenery.
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef scenery = iter->entity;

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

const RailDenizenData& SceneryComponent::Raft() const {
  const corgi::EntityRef raft =
      entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
  const RailDenizenData* rail_data = Data<RailDenizenData>(raft);
  assert(rail_data != nullptr);
  return *rail_data;
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

float SceneryComponent::DistSq(const corgi::EntityRef& scenery,
                               const RailDenizenData& raft) const {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  const TransformData* transform_data = Data<TransformData>(scenery);
  const float disappear_time = AnimLength(scenery_data, kSceneryDisappear);
  const vec3 pop_out_position =
      raft.Position() + raft.Velocity() * static_cast<float>(disappear_time);
  return (transform_data->position - pop_out_position).LengthSquared();
}

float SceneryComponent::AnimTimeRemaining(
    const corgi::EntityRef& scenery) const {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  const AnimationData* anim_data =
      Data<AnimationData>(scenery_data->render_child);
  return anim_data->motivator.Valid() ? anim_data->motivator.TimeRemaining()
                                      : 0.0f;
}

bool SceneryComponent::HasAnim(const SceneryData* scenery_data,
                               SceneryState state) const {
  return entity_manager_->GetComponent<AnimationComponent>()->HasAnim(
      scenery_data->render_child, state);
}

float SceneryComponent::AnimLength(const SceneryData* scenery_data,
                                   SceneryState state) const {
  return static_cast<float>(
      entity_manager_->GetComponent<AnimationComponent>()->AnimLength(
          scenery_data->render_child, state));
}

SceneryState SceneryComponent::NextState(const corgi::EntityRef& scenery,
                                         const RailDenizenData& raft) const {
  // Check for state transitions.
  SceneryData* scenery_data = Data<SceneryData>(scenery);
  switch (scenery_data->state) {
    case kSceneryHide:
      if (DistSq(scenery, raft) < PopInDistSq()) {
        return kSceneryAppear;
      }
      break;

    case kSceneryShow:
      if (DistSq(scenery, raft) > PopOutDistSq()) {
        scenery_data->show_override = kSceneryInvalid;
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
    default:
      assert(0);
  }

  // No transition.
  return scenery_data->state;
}

void SceneryComponent::Animate(const corgi::EntityRef& scenery,
                               SceneryState state) {
  AnimationComponent* anim_component =
      entity_manager_->GetComponent<AnimationComponent>();
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  anim_component->AnimateFromTable(scenery_data->render_child, state);
}

void SceneryComponent::StopAnimating(const corgi::EntityRef& scenery) {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  AnimationData* anim_data = Data<AnimationData>(scenery_data->render_child);
  if (anim_data->motivator.Valid()) {
    anim_data->motivator.Invalidate();
  }
}

void SceneryComponent::Show(const corgi::EntityRef& scenery, bool show) {
  TransformComponent* tf_component =
      entity_manager_->GetComponent<TransformComponent>();
  RenderMeshComponent* rm_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  const corgi::EntityRef parent = tf_component->GetRootParent(scenery);
  rm_component->SetVisibilityRecursively(parent, show);
}

void SceneryComponent::ShowAll(bool show) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef scenery = iter->entity;
    Show(scenery, show);
  }
}

void SceneryComponent::AnimateScenery(const corgi::EntityRef& scenery,
                                      SceneryData* scenery_data,
                                      SceneryState state) {
  if (HasAnim(scenery_data, state)) {
    Animate(scenery, state);
  } else {
    StopAnimating(scenery);
  }
}

void SceneryComponent::SetVisibilityOnOtherChildren(
    const corgi::EntityRef& scenery, bool visible) {
  const SceneryData* scenery_data = Data<SceneryData>(scenery);
  const TransformData* transform_data = Data<TransformData>(scenery);
  RenderMeshComponent* rm_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  if (transform_data != nullptr) {
    for (auto iter = transform_data->children.begin();
         iter != transform_data->children.end(); ++iter) {
      if (scenery_data->render_child != iter->owner) {
        rm_component->SetVisibilityRecursively(iter->owner, visible);
      }
    }
  }
}

void SceneryComponent::TransitionState(const corgi::EntityRef& scenery,
                                       SceneryState next_state) {
  SceneryData* scenery_data = Data<SceneryData>(scenery);

  // Hide or show the rendermesh, but not the other children.
  if (next_state == kSceneryHide || next_state == kSceneryAppear) {
    Show(scenery, next_state == kSceneryAppear);
    SetVisibilityOnOtherChildren(scenery, false);
  }

  // If there is an override animation, we should apply it if we're in the show
  // state.
  SceneryState state = next_state;
  if (next_state == kSceneryShow && scenery_data->show_override != -1) {
    state = scenery_data->show_override;
  }
  // When in the show state, we want any other children to be visible.
  if (scenery_data->state == kSceneryShow || next_state == kSceneryShow) {
    SetVisibilityOnOtherChildren(scenery, next_state == kSceneryShow);
  }

  // Animate or stop animating the scenery.
  AnimateScenery(scenery, scenery_data, state);

  // Update the state variable.
  scenery_data->state = next_state;
}

void SceneryComponent::ApplyShowOverride(const corgi::EntityRef& scenery,
                                         SceneryState show_override) {
  SceneryData* scenery_data = Data<SceneryData>(scenery);
  if (scenery_data) {
    // If there is an override animation, we should apply it if we're in the
    // show state.
    if (scenery_data->state == kSceneryShow &&
        scenery_data->show_override != show_override) {
      AnimateScenery(scenery, scenery_data, show_override);
    }

    // Update the show_override variable.
    scenery_data->show_override = show_override;
  }
}

void SceneryComponent::UpdateAllEntities(corgi::WorldTime /*delta_time*/) {
  const RailDenizenData& raft = Raft();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    corgi::EntityRef scenery = iter->entity;
    const SceneryData* scenery_data = Data<SceneryData>(scenery);

    // Execute state machine for each piece of scenery.
    const SceneryState next_state = NextState(scenery, raft);
    if (scenery_data->state != next_state) {
      TransitionState(scenery, next_state);
    }
  }
}

}  // zooshi
}  // fpl
