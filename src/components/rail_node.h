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

#ifndef FPL_ZOOSHI_COMPONENTS_RAIL_NODE_H_
#define FPL_ZOOSHI_COMPONENTS_RAIL_NODE_H_

#include <string>
#include "components_generated.h"
#include "corgi/component.h"
#include "rail_def_generated.h"

namespace fpl {
namespace zooshi {

// Data for scene object components.
struct RailNodeData {
  RailNodeData()
      : ordering(0), total_time(-1), reliable_distance(-1), wraps(true) {}
  float ordering;
  std::string rail_name;
  float total_time;
  float reliable_distance;
  bool wraps;
};

class RailNodeComponent : public corgi::Component<RailNodeData> {
 public:
  RailNodeComponent() {}
  virtual ~RailNodeComponent() {}

  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::RailNodeComponent,
                         fpl::zooshi::RailNodeData)

#endif  // FPL_ZOOSHI_COMPONENTS_RAIL_NODE_H_
