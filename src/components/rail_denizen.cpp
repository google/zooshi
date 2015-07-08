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
#include "components/rail_node.h"
#include "components_generated.h"
#include "entity/component.h"
#include "event/event_manager.h"
#include "events/change_rail_speed.h"
#include "events/editor_event.h"
#include "events/parse_action.h"
#include "events/utilities.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "motive/init.h"
#include "rail_def_generated.h"

using mathfu::vec3;

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::RailDenizenComponent,
                            fpl::fpl_project::RailDenizenData)

namespace fpl {
namespace fpl_project {

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

void RailDenizenData::Initialize(const Rail& rail, float start_time) {
  motivator.SetSpline(SplinePlayback3f(rail.splines(), start_time, true));
}

void RailDenizenComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();

  engine_ = services->motive_engine();

  services->event_manager()->RegisterListener(EventSinkUnion_ChangeRailSpeed,
                                              this);
  services->event_manager()->RegisterListener(EventSinkUnion_EditorEvent, this);
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
    }

    // When the motivator has looped all the way back to the beginning of the
    // spline, the SplineTime returns back to 0. We can exploit this fact to
    // determine when a lap has been completed.
    motive::MotiveTime current_time = rail_denizen_data->motivator.SplineTime();
    if (current_time < rail_denizen_data->previous_time &&
        rail_denizen_data->on_new_lap) {
      rail_denizen_data->lap++;
      EventContext context;
      context.source = iter->entity;
      context.raft =
          entity_manager_->GetComponent<ServicesComponent>()->raft_entity();
      ParseAction(
          rail_denizen_data->on_new_lap, &context,
          entity_manager_->GetComponent<ServicesComponent>()->event_manager(),
          entity_manager_);
    }
    rail_denizen_data->previous_time = current_time;
  }
}

void RailDenizenComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto rail_denizen_def = static_cast<const RailDenizenDef*>(raw_data);
  RailDenizenData* data = AddEntity(entity);

  if (rail_denizen_def->rail_name() != nullptr)
    data->rail_name = rail_denizen_def->rail_name()->c_str();

  data->start_time = rail_denizen_def->start_time();
  data->on_new_lap = rail_denizen_def->on_new_lap();
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
  data->update_orientation = rail_denizen_def->update_orientation();
  data->enabled = rail_denizen_def->enabled();

  entity_manager_->AddEntityToComponent<TransformComponent>(entity);

  data->motivator.Initialize(motive::SmoothInit(), engine_);
  data->motivator.SetSplinePlaybackRate(data->spline_playback_rate);

  data->rail_id = rail_denizen_def->rail_file()->c_str();

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
    LogInfo("RailDenizen: Loading static rail file: '%s'", data->rail_id);
    data->Initialize(*rail_manager->GetRail(data->rail_id), data->start_time);
  }
  data->motivator.SetSplinePlaybackRate(data->spline_playback_rate);
}

entity::ComponentInterface::RawDataUniquePtr
RailDenizenComponent::ExportRawData(entity::EntityRef& entity) const {
  const RailDenizenData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  Vec3 rail_offset{data->rail_offset.x(), data->rail_offset.y(),
                   data->rail_offset.z()};
  mathfu::vec3 euler = data->rail_orientation.ToEulerAngles();
  Vec3 rail_orientation{euler.x(), euler.y(), euler.z()};
  Vec3 rail_scale{data->rail_scale.x(), data->rail_scale.y(),
                  data->rail_scale.z()};

  auto rail_name = fbb.CreateString(data->rail_id);

  auto schema_file = entity_manager_->GetComponent<ServicesComponent>()
                         ->component_def_binary_schema();
  auto schema =
      schema_file != nullptr ? reflection::GetSchema(schema_file) : nullptr;
  auto table_def =
      schema != nullptr ? schema->objects()->LookupByKey("ActionDef") : nullptr;
  auto on_new_lap =
      data->on_new_lap != nullptr && table_def != nullptr
          ? flatbuffers::Offset<ActionDef>(
                flatbuffers::CopyTable(
                    fbb, *schema, *table_def,
                    (const flatbuffers::Table&)(*data->on_new_lap))
                    .o)
          : 0;

  RailDenizenDefBuilder builder(fbb);

  builder.add_start_time(data->start_time);
  builder.add_initial_playback_rate(data->spline_playback_rate);
  if (on_new_lap.o != 0) {
    builder.add_on_new_lap(on_new_lap);
  }
  builder.add_rail_file(rail_name);
  builder.add_rail_offset(&rail_offset);
  builder.add_rail_orientation(&rail_orientation);
  builder.add_rail_scale(&rail_scale);
  builder.add_update_orientation(data->update_orientation);
  builder.add_enabled(data->enabled);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void RailDenizenComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent<TransformComponent>(entity);
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
    case EventSinkUnion_EditorEvent: {
      // TODO(jsimantov): Mail rail lookup more efficient. http://b/22355890
      auto* editor_event = event_payload.ToData<EditorEventPayload>();
      if (editor_event->action == EditorEventAction_EntityUpdated &&
          editor_event->entity) {
        const RailNodeData* node_data =
            entity_manager_->GetComponentData<RailNodeData>(
                editor_event->entity);
        if (node_data != nullptr) {
          const std::string& rail_name = node_data->rail_name;
          for (auto iter = begin(); iter != end(); ++iter) {
            const RailDenizenData* rail_denizen_data =
                GetComponentData(iter->entity);
            if (rail_denizen_data->rail_name == rail_name) {
              InitializeRail(iter->entity);
            }
          }
        }
      }
      break;
    }
    default: { assert(0); }
  }
}

}  // fpl_project
}  // fpl
