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

#ifndef COMPONENTS_RAIL_NODE_H_
#define COMPONENTS_RAIL_NODE_H_

#include <string>
#include "components_generated.h"
#include "rail_def_generated.h"
#include "entity/component.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct RailNodeData {
  RailNodeData() : ordering(0), total_time(-1), reliable_distance(-1) {}
  float ordering;
  std::string rail_name;
  float total_time;
  float reliable_distance;
};

class RailNodeComponent : public entity::Component<RailNodeData> {
 public:
  RailNodeComponent() {}

  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::RailNodeComponent,
                              fpl::fpl_project::RailNodeData)

#endif  // COMPONENTS_RAIL_NODE_H_
