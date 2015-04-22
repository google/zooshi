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
#include <limits>

#include "components/transform.h"
#include "entity/component.h"
#include "motive/init.h"
#include "rail_def_generated.h"
#include "components_generated.h"

namespace fpl {

void RailDenizenData::Initialize(const motive::MotivatorInit& x_init,
                                 const motive::MotivatorInit& y_init,
                                 const motive::MotivatorInit& z_init,
                                 motive::MotiveEngine* engine) {
  fpl::SplinePlayback x_playback(x_spline, 0.0f, true);
  x_motivator.Initialize(x_init, engine);
  x_motivator.SetSpline(x_playback);

  fpl::SplinePlayback y_playback(y_spline, 0.0f, true);
  y_motivator.Initialize(y_init, engine);
  y_motivator.SetSpline(y_playback);

  fpl::SplinePlayback z_playback(z_spline, 0.0f, true);
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
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    RailDenizenData* rail_denizen_data = GetEntityData(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);
    mathfu::vec3 look_at =
        rail_denizen_data->Position() + rail_denizen_data->Velocity();
    mathfu::vec3 up = mathfu::kAxisY3f;
    transform_data->matrix.LookAt(look_at, rail_denizen_data->Position(), up);
  }
}

void RailDenizenComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_RailDenizenDef);
  auto rail_denizen_def =
      static_cast<const RailDenizenDef*>(component_data->data());
  auto rail_def = rail_denizen_def->rail_def();

  RailDenizenData* data = AddEntity(entity);

  const float infinity = std::numeric_limits<float>::infinity();
  fpl::RangeT<float> x_range(infinity, -infinity);
  fpl::RangeT<float> y_range(infinity, -infinity);
  fpl::RangeT<float> z_range(infinity, -infinity);
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
  data->x_spline.Init(x_range, 10.0f);
  data->y_spline.Init(y_range, 10.0f);
  data->z_spline.Init(z_range, 10.0f);
  for (auto iter = rail_def->nodes()->begin(); iter != rail_def->nodes()->end();
       ++iter) {
    auto node = *iter;
    data->x_spline.AddNode(node->time(), node->position()->x(),
                           node->tangent()->x());
    data->y_spline.AddNode(node->time(), node->position()->y(),
                           node->tangent()->y());
    data->z_spline.AddNode(node->time(), node->position()->z(),
                           node->tangent()->z());
  }
  motive::SmoothInit x_init(x_range, false);
  motive::SmoothInit y_init(y_range, false);
  motive::SmoothInit z_init(z_range, false);
  data->Initialize(x_init, y_init, z_init, engine_);
}

void RailDenizenComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_TransformDef);
}

}  // fpl

