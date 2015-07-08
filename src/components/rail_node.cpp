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

#include "components/rail_node.h"
#include "flatbuffers/flatbuffers.h"
#include "fplbase/utilities.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::fpl_project::RailNodeComponent,
                            fpl::fpl_project::RailNodeData)

namespace fpl {
namespace fpl_project {

void RailNodeComponent::AddFromRawData(entity::EntityRef& entity,
                                       const void* raw_data) {
  auto rail_node_def = static_cast<const RailNodeDef*>(raw_data);

  RailNodeData* data = AddEntity(entity);
  data->rail_name = rail_node_def->rail_name()->c_str();
  data->ordering = rail_node_def->ordering();
  if (rail_node_def->total_time())
    data->total_time = rail_node_def->total_time();
  if (rail_node_def->reliable_distance())
    data->reliable_distance = rail_node_def->reliable_distance();
}

entity::ComponentInterface::RawDataUniquePtr RailNodeComponent::ExportRawData(
    entity::EntityRef& entity) const {
  const RailNodeData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  auto rail_name = fbb.CreateString(data->rail_name);

  RailNodeDefBuilder builder(fbb);
  builder.add_rail_name(rail_name);
  builder.add_ordering(data->ordering);
  if (data->total_time != -1) {
    builder.add_total_time(data->total_time);
  }
  if (data->reliable_distance != -1) {
    builder.add_reliable_distance(data->reliable_distance);
  }

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

}  // fpl_project
}  // fpl
