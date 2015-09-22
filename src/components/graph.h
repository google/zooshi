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

#ifndef FPL_COMPONENTS_GRAPH_H_
#define FPL_COMPONENTS_GRAPH_H_

#include <memory>

#include "breadboard/event.h"
#include "breadboard/graph.h"
#include "breadboard/graph_factory.h"
#include "breadboard/graph_state.h"
#include "entity/component.h"
#include "graph_generated.h"

namespace fpl {
namespace fpl_project {

struct SerializableGraphState {
  std::string filename;
  std::unique_ptr<breadboard::GraphState> graph_state;
};

struct GraphData {
  std::vector<SerializableGraphState> graphs;
  breadboard::NodeEventBroadcaster broadcaster;
};

class GraphComponent : public entity::Component<GraphData> {
 public:
  // Once entities themselves have been initialized, initialize the graphs. This
  // must be done after because graphs may reference entities.
  void PostLoadFixup();

  // Gets the broadcaster for an entity, even if that entity does not yet have
  // one.
  breadboard::NodeEventBroadcaster* GetCreateBroadcaster(
      entity::EntityRef entity);

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void InitEntity(entity::EntityRef& /*entity*/) {}
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

 private:
  breadboard::GraphFactory* graph_factory_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::GraphComponent,
                              fpl::fpl_project::GraphData)

#endif  // FPL_COMPONENTS_GRAPH_H_
