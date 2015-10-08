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

#include "railmanager.h"

#include <map>
#include "component_library/transform.h"
#include "components/rail_node.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"
#include "mathfu/constants.h"
#include "motive/math/spline_util.h"
#include "rail_def_generated.h"

namespace fpl {
namespace zooshi {

static const float kSplineGranularity = 10.0f;

using mathfu::vec3;
using mathfu::vec3_packed;

void Rail::Initialize(const RailDef *rail_def, float spline_granularity) {
  // Allocate temporary memory for the positions and derivative arrays.
  const int num_positions = static_cast<int>(rail_def->positions()->Length());
  std::vector<vec3_packed> positions;
  positions.resize(num_positions);

  // Load positions.
  for (int i = 0; i < num_positions; ++i) {
    positions[i] = LoadVec3(rail_def->positions()->Get(i));
  }
  InitializeFromPositions(positions, spline_granularity,
                          rail_def->reliable_distance(),
                          rail_def->total_time());
}

void Rail::InitializeFromPositions(const std::vector<vec3_packed> &positions,
                                   float spline_granularity,
                                   float reliable_distance, float total_time) {
  const int num_positions = positions.size();
  std::vector<float> times;
  std::vector<vec3_packed> derivatives;
  times.resize(num_positions);
  derivatives.resize(num_positions);

  // Calculate derivates and times from positions.
  CalculateConstSpeedCurveFromPositions<3>(&positions[0], num_positions,
                                           total_time, reliable_distance,
                                           &times[0], &derivatives[0]);

  // Get position extremes.
  vec3 position_min(std::numeric_limits<float>::infinity());
  vec3 position_max(-std::numeric_limits<float>::infinity());
  for (auto p = positions.begin(); p != positions.end(); ++p) {
    const vec3 position(*p);
    position_min = vec3::Min(position_min, position);
    position_max = vec3::Max(position_max, position);
  }

  // Initialize the compact-splines to have the best precision possible,
  // given the range limits.
  const float kRangeSafeBoundsPercent = 1.1f;
  for (motive::MotiveDimension i = 0; i < kDimensions; ++i) {
    splines_[i].Init(Range(position_min[i], position_max[i])
                         .Lengthen(kRangeSafeBoundsPercent),
                     spline_granularity);
  }

  // Initialize the splines. For now, the splines all have key points at the
  // same time values, but this is a limitation that we can (and should) lift
  // to maximize compression.
  for (int k = 0; k < num_positions; ++k) {
    const float t = times[k];
    const vec3 position(positions[k]);
    const vec3 derivative(derivatives[k]);
    for (motive::MotiveDimension i = 0; i < kDimensions; ++i) {
      splines_[i].AddNode(t, position[i], derivative[i]);
    }
  }
}

Rail *RailManager::GetRail(RailId rail_file) {
  if (rail_map.find(rail_file) == rail_map.end()) {
    // New rail, so we load it up:
    std::string rail_def_source;
    if (!LoadFile(rail_file, &rail_def_source)) {
      return nullptr;
    }
    const RailDef *rail_def = GetRailDef(rail_def_source.c_str());
    rail_map[rail_file] = std::unique_ptr<Rail>(new Rail());
    rail_map[rail_file]->Initialize(rail_def, kSplineGranularity);
  }
  return rail_map[rail_file].get();
}

Rail *RailManager::GetRailFromComponents(
    const char *rail_name, entity::EntityManager *entity_manager) {
  std::map<float, entity::EntityRef> rail_entities;

  auto *rail_component = entity_manager->GetComponent<RailNodeComponent>();
  for (auto i = rail_component->begin(); i != rail_component->end(); ++i) {
    if (i->data.rail_name == rail_name) {
      rail_entities[i->data.ordering] = i->entity;
    }
  }

  if (rail_entities.size() == 0) {
    LogInfo("RailManager: No RailNode entities with rail_name '%s' found",
            rail_name);
    return nullptr;  // invalid rail name
  }

  std::vector<vec3_packed> positions;
  positions.resize(rail_entities.size() + 1);
  const RailNodeData *first_data =
      rail_component->GetComponentData(rail_entities.begin()->second);
  // Extract the total time and reliable distance from the first-listed
  // RailNode for this rail name.
  float total_time = first_data->total_time;
  float reliable_distance = first_data->reliable_distance;

  auto *transform_component =
      entity_manager->GetComponent<component_library::TransformComponent>();
  // map will sort by the key
  int i = 0;
  for (auto iter = rail_entities.begin(); iter != rail_entities.end(); ++iter) {
    positions[i++] = transform_component->WorldPosition(iter->second);
  }
  // Repeat the first node at the end so we loop.
  positions[i] =
      transform_component->WorldPosition(rail_entities.begin()->second);

  // Cache this until we request the rail from these components again.
  rail_map[rail_name] = std::unique_ptr<Rail>(new Rail());
  rail_map[rail_name]->InitializeFromPositions(positions, kSplineGranularity,
                                               reliable_distance, total_time);
  return rail_map[rail_name].get();
}

void RailManager::Clear() { rail_map.clear(); }

}  // zooshi
}  // fpl
