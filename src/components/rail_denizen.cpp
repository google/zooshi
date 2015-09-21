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
#include "component_library/animation.h"
#include "component_library/common_services.h"
#include "component_library/transform.h"
#include "components/graph.h"
#include "components/rail_node.h"
#include "components/services.h"
#include "components_generated.h"
#include "entity/component.h"
#include "event/event.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "motive/init.h"
#include "rail_def_generated.h"
#include "world_editor/world_editor.h"

using mathfu::vec3;

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::RailDenizenComponent,
                            fpl::fpl_project::RailDenizenData)

namespace fpl {
namespace fpl_project {

FPL_EVENT_DEFINE_EVENT(kNewLapEventId)

using fpl::component_library::AnimationComponent;
using fpl::component_library::CommonServicesComponent;
using fpl::component_library::TransformData;
using fpl::component_library::TransformComponent;
using fpl::editor::WorldEditor;

void Rail::Positions(float delta_time,
                     std::vector<mathfu::vec3_packed>* positions) const {
  const size_t num_positions =
      static_cast<size_t>(std::floor(EndTime() / delta_time)) + 1;
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

void RailDenizenData::Initialize(const Rail& rail, float start_time) {
  motivator.SetSpline(SplinePlayback3f(rail.splines(), start_time, true));
}

void RailDenizenComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();
  // World editor is not guaranteed to be present in all versions of the game.
  // Only set up callbacks if we actually have a world editor.
  WorldEditor* world_editor = services->world_editor();
  if (world_editor) {
    world_editor->AddOnUpdateEntityCallback([this](
        const entity::EntityRef& entity) { UpdateRailNodeData(entity); });
  }
}

void RailDenizenComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    RailDenizenData* rail_denizen_data = GetComponentData(iter->entity);
    if (!rail_denizen_data->enabled) {
      continue;
    }
    TransformData* transform_data = Data<TransformData>(iter->entity);
    vec3 position = rail_denizen_data->rail_orientation.Inverse() *
                    rail_denizen_data->Position();
    position *= rail_denizen_data->rail_scale;
    position += rail_denizen_data->rail_offset;
    transform_data->position = position;
    if (rail_denizen_data->update_orientation) {
      transform_data->orientation = mathfu::quat::RotateFromTo(
          rail_denizen_data->Velocity(), mathfu::kAxisY3f);
      if (rail_denizen_data->inherit_transform_data) {
        transform_data->orientation =
            rail_denizen_data->rail_orientation * transform_data->orientation;
      }
    }

    float previous_lap = rail_denizen_data->lap;
    motive::MotiveTime total = rail_denizen_data->motivator.SplineTime() +
                               rail_denizen_data->motivator.TargetTime();
    float percent =
        static_cast<float>(rail_denizen_data->motivator.SplineTime()) / total;
    rail_denizen_data->lap = floor(previous_lap) + percent;

    // When the motivator has looped all the way back to the beginning of the
    // spline, the SplineTime returns back to 0. We can exploit this fact to
    // determine when a lap has been completed, by comparing against the
    // previous lap amount.
    if (rail_denizen_data->lap < previous_lap) {
      rail_denizen_data->lap++;
      GraphData* graph_data = Data<GraphData>(iter->entity);
      if (graph_data) {
        graph_data->broadcaster.BroadcastEvent(kNewLapEventId);
      }
    }
  }
}

void RailDenizenComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto rail_denizen_def = static_cast<const RailDenizenDef*>(raw_data);
  RailDenizenData* data = AddEntity(entity);

  if (rail_denizen_def->rail_name() != nullptr)
    data->rail_name = rail_denizen_def->rail_name()->c_str();

  data->start_time = rail_denizen_def->start_time();
  data->spline_playback_rate = rail_denizen_def->initial_playback_rate();

  auto offset = rail_denizen_def->rail_offset();
  auto orientation = rail_denizen_def->rail_orientation();
  auto scale = rail_denizen_def->rail_scale();
  if (offset != nullptr) {
    data->rail_offset = LoadVec3(offset);
  }
  if (orientation != nullptr) {
    data->rail_orientation =
        mathfu::quat::FromEulerAngles(LoadVec3(orientation));
  }
  if (scale != nullptr) {
    data->rail_scale = LoadVec3(scale);
  }
  data->update_orientation =
      rail_denizen_def->update_orientation() ? true : false;
  data->inherit_transform_data =
      rail_denizen_def->inherit_transform_data() ? true : false;
  data->enabled = rail_denizen_def->enabled() ? true : false;

  entity_manager_->AddEntityToComponent<TransformComponent>(entity);

  motive::MotiveEngine& engine =
      entity_manager_->GetComponent<AnimationComponent>()->engine();
  data->motivator.Initialize(motive::SmoothInit(), &engine);
  data->motivator.SetSplinePlaybackRate(data->spline_playback_rate);

  InitializeRail(entity);
}

void RailDenizenComponent::InitializeRail(entity::EntityRef& entity) {
  RailManager* rail_manager =
      entity_manager_->GetComponent<ServicesComponent>()->rail_manager();
  RailDenizenData* data = GetComponentData(entity);
  if (data->rail_name != "") {
    data->Initialize(*rail_manager->GetRailFromComponents(
                         data->rail_name.c_str(), entity_manager_),
                     data->start_time);
  } else {
    LogError("RailDenizen: Error, no rail name specified");
  }
  data->motivator.SetSplinePlaybackRate(data->spline_playback_rate);
}

entity::ComponentInterface::RawDataUniquePtr
RailDenizenComponent::ExportRawData(const entity::EntityRef& entity) const {
  const RailDenizenData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  Vec3 rail_offset(data->rail_offset.x(), data->rail_offset.y(),
                   data->rail_offset.z());
  mathfu::vec3 euler = data->rail_orientation.ToEulerAngles();
  Vec3 rail_orientation(euler.x(), euler.y(), euler.z());
  Vec3 rail_scale(data->rail_scale.x(), data->rail_scale.y(),
                  data->rail_scale.z());

  auto rail_name =
      data->rail_name != "" ? fbb.CreateString(data->rail_name) : 0;

  RailDenizenDefBuilder builder(fbb);

  builder.add_start_time(data->start_time);
  builder.add_initial_playback_rate(data->spline_playback_rate);
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

void RailDenizenComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
}

void RailDenizenComponent::UpdateRailNodeData(entity::EntityRef entity) {
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

}  // fpl_project
}  // fpl
