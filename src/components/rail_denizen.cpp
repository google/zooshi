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

#include "components/services.h"
#include "components/transform.h"
#include "components_generated.h"
#include "entity/component.h"
#include "event_system/event_manager.h"
#include "events/change_rail_speed.h"
#include "events/parse_action.h"
#include "events/utilities.h"
#include "fplbase/flatbuffer_utils.h"
#include "mathfu/constants.h"
#include "motive/init.h"
#include "rail_def_generated.h"

using mathfu::vec3;

namespace fpl {
namespace fpl_project {

void Rail::Initialize(const RailDef* rail_def, float spline_granularity) {
  // Get position extremes.
  vec3 position_min(std::numeric_limits<float>::infinity());
  vec3 position_max(-std::numeric_limits<float>::infinity());
  for (auto iter = rail_def->nodes()->begin(); iter != rail_def->nodes()->end();
       ++iter) {
    const vec3 position = LoadVec3(iter->position());
    position_min = vec3::Min(position_min, position);
    position_max = vec3::Max(position_max, position);
  }

  // Initialize the compact-splines to have the best precision possible,
  // given the range limits.
  for (motive::MotiveDimension i = 0; i < kDimensions; ++i) {
    splines_[i].Init(Range(position_min[i], position_max[i]),
                     spline_granularity);
  }

  // Initialize the splines. For now, the splines all have key points at the
  // same time values, but this is a limitation that we can (and should) lift
  // to maximize compression.
  for (auto iter = rail_def->nodes()->begin(); iter != rail_def->nodes()->end();
       ++iter) {
    const float t = iter->time();
    const vec3 position = LoadVec3(iter->position());
    const vec3 tangent = LoadVec3(iter->tangent());
    for (motive::MotiveDimension i = 0; i < kDimensions; ++i) {
      splines_[i].AddNode(t, position[i], tangent[i]);
    }
  }
}

void Rail::Positions(float delta_time,
                     std::vector<mathfu::vec3_packed>* positions) const {
  const size_t num_positions = std::floor(EndTime() / delta_time) + 1;
  positions->resize(num_positions);

  CompactSpline::BulkYs<3>(splines_, 0.0f, delta_time, num_positions,
                           &(*positions)[0]);
}

vec3 Rail::PositionCalculatedSlowly(float time) const {
  vec3 position;
  for (int i = 0; i < kDimensions; ++i) {
    position[i] = splines_[i].YCalculatedSlowly(time);
  }
  return position;
}

void RailDenizenData::Initialize(const Rail& rail, float start_time,
                                 motive::MotiveEngine* engine) {
  motivator.Initialize(motive::SmoothInit(), engine);
  motivator.SetSpline(SplinePlayback3f(rail.splines(), start_time, true));
  motivator.SetSplinePlaybackRate(spline_playback_rate);
}

void RailDenizenComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();

  engine_ = services->motive_engine();

  event_manager_ = services->event_manager();
  event_manager_->RegisterListener(EventSinkUnion_ChangeRailSpeed, this);
}

void RailDenizenComponent::SetRail(const RailDef* rail_def) {
  static const float kSplineGranularity = 10.0f;
  rail_.Initialize(rail_def, kSplineGranularity);
}

void RailDenizenComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RailDenizenData* rail_denizen_data = GetComponentData(iter->entity);
    TransformData* transform_data = Data<TransformData>(iter->entity);
    transform_data->position = rail_denizen_data->Position();
    transform_data->orientation = mathfu::quat::RotateFromTo(
        rail_denizen_data->Velocity(), mathfu::kAxisY3f);

    // When the motivator has looped all the way back to the beginning of the
    // spline, the SplineTime returns back to 0. We can exploit this fact to
    // determine when a lap has been completed.
    motive::MotiveTime current_time = rail_denizen_data->motivator.SplineTime();
    if (current_time < rail_denizen_data->previous_time &&
        rail_denizen_data->on_new_lap) {
      rail_denizen_data->lap++;
      EventContext context;
      context.source = iter->entity;
      ParseAction(rail_denizen_data->on_new_lap, &context, event_manager_,
                  entity_manager_);
    }
    rail_denizen_data->previous_time = current_time;
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
  data->on_new_lap = rail_denizen_def->on_new_lap();
  data->spline_playback_rate = rail_denizen_def->initial_playback_rate();
  data->motivator.SetSplinePlaybackRate(data->spline_playback_rate);
  if (rail_denizen_def->is_river() && !river_entity_.IsValid()) {
    river_entity_ = entity;
  }
}

void RailDenizenComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_TransformDef);
}

void RailDenizenComponent::OnEvent(const event::EventPayload& event_payload) {
  switch (event_payload.id()) {
    case EventSinkUnion_ChangeRailSpeed: {
      auto* speed_event = event_payload.ToData<ChangeRailSpeedPayload>();
      RailDenizenData* rail_denizen_data =
          Data<RailDenizenData>(speed_event->entity);
      if (rail_denizen_data) {
        ApplyOperation(&rail_denizen_data->spline_playback_rate,
                       speed_event->change_rail_speed->op(),
                       speed_event->change_rail_speed->value());
        rail_denizen_data->motivator.SetSplinePlaybackRate(
            rail_denizen_data->spline_playback_rate);
      }
      break;
    }
    default: { assert(0); }
  }
}

}  // fpl_project
}  // fpl
