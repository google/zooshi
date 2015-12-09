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
#include <cmath>
#include <limits>
#include "breadboard/event.h"
#include "components/rail_node.h"
#include "components/services.h"
#include "components_generated.h"
#include "corgi/component.h"
#include "corgi/entity_common.h"
#include "corgi_component_library/animation.h"
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/graph.h"
#include "corgi_component_library/transform.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "motive/init.h"
#include "rail_def_generated.h"
#include "scene_lab/scene_lab.h"

using mathfu::vec3;
using corgi::component_library::GraphData;

CORGI_DEFINE_COMPONENT(fpl::zooshi::RailDenizenComponent,
                       fpl::zooshi::RailDenizenData)

namespace fpl {
namespace zooshi {

BREADBOARD_DEFINE_EVENT(kNewLapEventId)

using corgi::component_library::AnimationComponent;
using corgi::component_library::CommonServicesComponent;
using corgi::component_library::TransformData;
using corgi::component_library::TransformComponent;
using scene_lab::SceneLab;

void Rail::Positions(float delta_time,
                     std::vector<mathfu::vec3_packed>* positions) const {
  const size_t num_positions =
      static_cast<size_t>(std::floor(EndTime() / delta_time)) + 1;
  positions->resize(num_positions);

  motive::CompactSpline::BulkYs<3>(splines_, 0.0f, delta_time, num_positions,
                                   &(*positions)[0]);
}

vec3 Rail::PositionCalculatedSlowly(float time) const {
  vec3 position;
  for (int i = 0; i < kDimensions; ++i) {
    position[i] = splines_[i].YCalculatedSlowly(time);
  }
  return position;
}

void RailDenizenData::Initialize(const Rail& rail,
                                 motive::MotiveEngine& engine) {
  const motive::SplinePlayback playback(start_time, true,
                                        initial_playback_rate);
  motivator.Initialize(motive::SplineInit(), &engine);
  motivator.SetSplines(rail.splines(), playback);
  // The interpolated orientation converges towards the target orientation
  // at a non-linear rate that is affected by delta-time and
  // orientation_convergence_rate, this hack estimates the look-ahead for the
  // convergence time given roughly a 60Hz update rate.
  orientation_motivator.Initialize(motive::SplineInit(), &engine);
  orientation_motivator.SetSplines(rail.splines(), playback);
  if (orientation_convergence_rate != 0.0f) {
    orientation_motivator.SetSplineTime(static_cast<motive::MotiveTime>(
        1.0f / logf(0.5f + orientation_convergence_rate) *
        corgi::kMillisecondsPerSecond));
  }
  playback_rate.InitializeWithTarget(motive::SplineInit(), &engine,
                                     motive::Current1f(initial_playback_rate));
}

void RailDenizenData::SetPlaybackRate(float rate, float transition_time) {
  const auto time = static_cast<motive::MotiveTime>(transition_time);
  playback_rate.SetTarget(motive::Target1f(rate, 0.0f, time));
}

void RailDenizenComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();
  // Scene Lab is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a Scene Lab.
  SceneLab* scene_lab = services->scene_lab();
  if (scene_lab) {
    scene_lab->AddOnUpdateEntityCallback(
        [this](const corgi::EntityRef& entity) { UpdateRailNodeData(entity); });
    scene_lab->AddOnEnterEditorCallback([this]() { OnEnterEditor(); });
    scene_lab->AddOnExitEditorCallback([this]() { PostLoadFixup(); });
  }
}

void RailDenizenComponent::UpdateAllEntities(corgi::WorldTime delta_time) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RailDenizenData* rail_denizen_data = GetComponentData(iter->entity);
    if (!rail_denizen_data->enabled) {
      continue;
    }
    rail_denizen_data->SetSplinePlaybackRate(rail_denizen_data->PlaybackRate());
    TransformData* transform_data = Data<TransformData>(iter->entity);
    vec3 position = rail_denizen_data->rail_orientation.Inverse() *
                    rail_denizen_data->Position();
    position *= rail_denizen_data->rail_scale;
    position += rail_denizen_data->rail_offset;
    transform_data->position = position;
    if (rail_denizen_data->update_orientation) {
      float convergence_rate = rail_denizen_data->orientation_convergence_rate;
      const motive::Motivator3f& motivator =
          convergence_rate == 0.0f ? rail_denizen_data->motivator
                                   : rail_denizen_data->orientation_motivator;
      mathfu::quat target_orientation =
          rail_denizen_data->rail_orientation *
          mathfu::quat::RotateFromTo(motivator.Direction(), mathfu::kAxisY3f);
      // Convergence is disabled when the playback rate is zero as
      // it's possible for the slerp to yield an invalid quaternion
      // with angles approaching zero.
      if (convergence_rate != 0.0f &&
          rail_denizen_data->PlaybackRate() > 0.0f) {
        rail_denizen_data->interpolated_orientation = mathfu::quat::Slerp(
            rail_denizen_data->interpolated_orientation, target_orientation,
            std::min(convergence_rate * static_cast<float>(delta_time) /
                         static_cast<float>(corgi::kMillisecondsPerSecond),
                     1.0f));
        transform_data->orientation =
            rail_denizen_data->interpolated_orientation;
      } else {
        transform_data->orientation = target_orientation;
      }
    }

    float previous_progress = rail_denizen_data->lap_progress;
    motive::MotiveTime total = rail_denizen_data->motivator.SplineTime() +
                               rail_denizen_data->motivator.TargetTime();
    rail_denizen_data->lap_progress =
        static_cast<float>(rail_denizen_data->motivator.SplineTime()) / total;

    // When the motivator has looped all the way back to the beginning of the
    // spline, the SplineTime returns back to 0. We can exploit this fact to
    // determine when a lap has been completed, by comparing against the
    // previous lap amount.
    if (rail_denizen_data->lap_progress < previous_progress) {
      rail_denizen_data->lap_number++;
      GraphData* graph_data = Data<GraphData>(iter->entity);
      if (graph_data) {
        graph_data->broadcaster.BroadcastEvent(kNewLapEventId);
      }
    }
    rail_denizen_data->total_lap_progress =
        rail_denizen_data->lap_progress + rail_denizen_data->lap_number;
  }
}

void RailDenizenComponent::AddFromRawData(corgi::EntityRef& entity,
                                          const void* raw_data) {
  auto rail_denizen_def = static_cast<const RailDenizenDef*>(raw_data);
  RailDenizenData* data = AddEntity(entity);

  if (rail_denizen_def->rail_name() != nullptr)
    data->rail_name = rail_denizen_def->rail_name()->c_str();

  data->start_time = rail_denizen_def->start_time();
  data->initial_playback_rate = rail_denizen_def->initial_playback_rate();

  auto offset = rail_denizen_def->rail_offset();
  auto orientation = rail_denizen_def->rail_orientation();
  auto scale = rail_denizen_def->rail_scale();
  if (offset != nullptr) {
    data->rail_offset = LoadVec3(offset);
  } else {
    data->rail_offset = mathfu::kZeros3f;
  }
  data->internal_rail_offset = data->rail_offset;
  if (orientation != nullptr) {
    data->rail_orientation =
        mathfu::quat::FromEulerAngles(LoadVec3(orientation));
  } else {
    data->rail_orientation = mathfu::quat::identity;
  }
  data->internal_rail_orientation = data->rail_orientation;
  if (scale != nullptr) {
    data->rail_scale = LoadVec3(scale);
  } else {
    data->rail_scale = mathfu::kOnes3f;
  }
  data->internal_rail_scale = data->rail_scale;
  data->orientation_convergence_rate =
      rail_denizen_def->orientation_convergence_rate();
  data->update_orientation =
      rail_denizen_def->update_orientation() ? true : false;
  data->inherit_transform_data =
      rail_denizen_def->inherit_transform_data() ? true : false;
  data->enabled = rail_denizen_def->enabled() ? true : false;

  entity_manager_->AddEntityToComponent<TransformComponent>(entity);

  InitializeRail(entity);

  data->interpolated_orientation =
      data->rail_orientation *
      mathfu::quat::RotateFromTo(data->orientation_motivator.Direction(),
                                 mathfu::kAxisY3f);
}

void RailDenizenComponent::InitializeRail(corgi::EntityRef& entity) {
  RailManager* rail_manager =
      entity_manager_->GetComponent<ServicesComponent>()->rail_manager();
  RailDenizenData* data = GetComponentData(entity);
  if (data->rail_name != "") {
    motive::MotiveEngine& engine =
        entity_manager_->GetComponent<AnimationComponent>()->engine();
    data->Initialize(*rail_manager->GetRailFromComponents(
                         data->rail_name.c_str(), entity_manager_),
                     engine);
  } else {
    fplbase::LogError("RailDenizen: Error, no rail name specified");
  }
}

corgi::ComponentInterface::RawDataUniquePtr RailDenizenComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  const RailDenizenData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  fplbase::Vec3 rail_offset(data->internal_rail_offset.x(),
                            data->internal_rail_offset.y(),
                            data->internal_rail_offset.z());
  mathfu::vec3 euler = data->internal_rail_orientation.ToEulerAngles();
  fplbase::Vec3 rail_orientation(euler.x(), euler.y(), euler.z());
  fplbase::Vec3 rail_scale(data->internal_rail_scale.x(),
                           data->internal_rail_scale.y(),
                           data->internal_rail_scale.z());

  auto rail_name =
      data->rail_name != "" ? fbb.CreateString(data->rail_name) : 0;

  RailDenizenDefBuilder builder(fbb);

  builder.add_start_time(data->start_time);
  builder.add_initial_playback_rate(data->initial_playback_rate);
  if (rail_name.o != 0) {
    builder.add_rail_name(rail_name);
  }
  builder.add_rail_offset(&rail_offset);
  builder.add_rail_orientation(&rail_orientation);
  builder.add_rail_scale(&rail_scale);
  builder.add_update_orientation(data->update_orientation);
  builder.add_inherit_transform_data(data->inherit_transform_data);
  builder.add_enabled(data->enabled);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void RailDenizenComponent::InitEntity(corgi::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RailDenizenComponent::UpdateRailNodeData(corgi::EntityRef entity) {
  const RailNodeData* node_data =
      entity_manager_->GetComponentData<RailNodeData>(entity);
  if (node_data != nullptr) {
    const std::string& rail_name = node_data->rail_name;
    for (auto iter = begin(); iter != end(); ++iter) {
      const RailDenizenData* rail_denizen_data = GetComponentData(iter->entity);
      if (rail_denizen_data->rail_name == rail_name) {
        InitializeRail(iter->entity);
      }
    }
  }
}

void RailDenizenComponent::PostLoadFixup() {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RailDenizenData* rail_data = Data<RailDenizenData>(iter->entity);
    if (rail_data->inherit_transform_data) {
      TransformData* transform_data = Data<TransformData>(iter->entity);
      rail_data->rail_offset =
          transform_data->position + rail_data->internal_rail_offset;
      rail_data->rail_orientation =
          transform_data->orientation * rail_data->internal_rail_orientation;
      rail_data->rail_scale =
          transform_data->scale * rail_data->internal_rail_scale;
    }
  }
}

void RailDenizenComponent::OnEnterEditor() {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    // If an entity inherits its offsets from the transform data, the transform
    // data needs to be reset, or else it will copy its own movement from the
    // transform data.
    RailDenizenData* rail_data = GetComponentData(iter->entity);
    if (rail_data->inherit_transform_data) {
      TransformData* td = Data<TransformData>(iter->entity);
      td->position = rail_data->rail_offset - rail_data->internal_rail_offset;
      td->orientation = rail_data->rail_orientation *
                        rail_data->internal_rail_orientation.Inverse();
      td->scale = rail_data->rail_scale / rail_data->internal_rail_scale;
    }
  }
}

}  // zooshi
}  // fpl
