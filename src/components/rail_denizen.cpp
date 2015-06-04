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

#include "components/rail_denizen.h"

#include <algorithm>

#include "components/transform.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "motive/init.h"
#include "rail_def_generated.h"
#include "components_generated.h"

namespace fpl {
namespace fpl_project {

void Rail::Initialize(const RailDef* rail_def, float spline_granularity) {
  // Initialize to +inf and -inf so that min and max can be set appropriately.
  x_range.set_start(std::numeric_limits<float>::infinity());
  x_range.set_end(-std::numeric_limits<float>::infinity());
  y_range.set_start(std::numeric_limits<float>::infinity());
  y_range.set_end(-std::numeric_limits<float>::infinity());
  z_range.set_start(std::numeric_limits<float>::infinity());
  z_range.set_end(-std::numeric_limits<float>::infinity());
  for (auto iter = rail_def->nodes()->begin(); iter != rail_def->nodes()->end();
       ++iter) {
    if (iter->position()->x() < x_range.start()) {
      x_range.set_start(iter->position()->x());
    }
    if (iter->position()->y() < y_range.start()) {
      y_range.set_start(iter->position()->y());
    }
    if (iter->position()->z() < z_range.start()) {
      z_range.set_start(iter->position()->z());
    }
    if (iter->position()->x() > x_range.end()) {
      x_range.set_end(iter->position()->x());
    }
    if (iter->position()->y() > y_range.end()) {
      y_range.set_end(iter->position()->y());
    }
    if (iter->position()->z() > x_range.end()) {
      z_range.set_end(iter->position()->z());
    }
  }

  x_spline.Init(x_range, spline_granularity);
  y_spline.Init(y_range, spline_granularity);
  z_spline.Init(z_range, spline_granularity);

  for (auto iter = rail_def->nodes()->begin(); iter != rail_def->nodes()->end();
       ++iter) {
    auto node = *iter;
    x_spline.AddNode(node->time(), node->position()->x(), node->tangent()->x());
    y_spline.AddNode(node->time(), node->position()->y(), node->tangent()->y());
    z_spline.AddNode(node->time(), node->position()->z(), node->tangent()->z());
  }
}

void RailDenizenData::Initialize(Rail& rail, float start_time,
                                 motive::MotiveEngine* engine) {
  motive::SmoothInit x_init(rail.x_range, false);
  fpl::SplinePlayback x_playback(rail.x_spline, start_time, true);
  x_motivator.Initialize(x_init, engine);
  x_motivator.SetSpline(x_playback);

  motive::SmoothInit y_init(rail.y_range, false);
  fpl::SplinePlayback y_playback(rail.y_spline, start_time, true);
  y_motivator.Initialize(y_init, engine);
  y_motivator.SetSpline(y_playback);

  motive::SmoothInit z_init(rail.z_range, false);
  fpl::SplinePlayback z_playback(rail.z_spline, start_time, true);
  z_motivator.Initialize(z_init, engine);
  z_motivator.SetSpline(z_playback);
}

mathfu::vec3 RailDenizenData::Position() const {
  return mathfu::vec3(x_motivator.Value(), y_motivator.Value(),
                      z_motivator.Value());
}

mathfu::vec3 RailDenizenData::Velocity() const {
  return mathfu::vec3(x_motivator.Velocity(), y_motivator.Velocity(),
                      z_motivator.Velocity());
}

void RailDenizenComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RailDenizenData* rail_denizen_data = GetComponentData(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);
    transform_data->position = rail_denizen_data->Position();
    transform_data->orientation = mathfu::quat::RotateFromTo(
        rail_denizen_data->Velocity(), mathfu::kAxisY3f);
  }
}

void RailDenizenComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_RailDenizenDef);
  auto rail_denizen_def =
      static_cast<const RailDenizenDef*>(component_data->data());
  RailDenizenData* data = AddEntity(entity);
  data->Initialize(rail_, rail_denizen_def->start_time(), engine_);
}

void RailDenizenComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_TransformDef);
}

void RailDenizenComponent::Initialize(const RailDef* rail_def) {
  static const float kSplineGranularity = 10.0f;
  rail_.Initialize(rail_def, kSplineGranularity);
}

}  // fpl_project
}  // fpl
