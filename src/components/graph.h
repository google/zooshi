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

  SerializableGraphState() {}
  SerializableGraphState(SerializableGraphState&& other) {
    *this = std::move(other);
  }

  SerializableGraphState& operator=(SerializableGraphState&& other) {
    filename = std::move(other.filename);
    graph_state = std::move(other.graph_state);
    return *this;
  }

#ifdef _MSC_VER
  // Provide fake versions of copy contructor and operator to let VS2012
  // compile and link.
  //
  // These should never be called, since unique_ptr is not copyable.
  // However, Visual Studio 2012 not only calls them, but links them in,
  // when this class is in an std::pair (as it is when in an std::map).
  //
  // The asserts never get hit here. These functions are not actually called.
  SerializableGraphState(const SerializableGraphState&) { assert(false); }
  SerializableGraphState& operator=(const SerializableGraphState&) {
    assert(false);
    return *this;
  }
#endif // _MSC_VER
};

struct GraphData {
  std::vector<SerializableGraphState> graphs;
  breadboard::NodeEventBroadcaster broadcaster;

  GraphData() {}
  GraphData(GraphData&& other) {
    *this = std::move(other);
  }

  GraphData& operator=(GraphData&& other) {
    graphs = std::move(other.graphs);
    broadcaster = std::move(other.broadcaster);
    return *this;
  }

 private:
  GraphData(GraphData&);
  GraphData& operator=(GraphData&);
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
