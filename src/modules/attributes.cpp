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

#include "modules/attributes.h"

#include <string>

#include "components/attributes.h"
#include "components/graph.h"
#include "entity/entity_manager.h"
#include "event/event.h"
#include "event/event_system.h"
#include "event/base_node.h"
#include "modules/event_ids.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

// Returns the attributes component data of the given entity.
class AttributesNode : public event::BaseNode {
 public:
  AttributesNode(AttributesComponent* attributes_component,
                 GraphComponent* graph_component)
      : attributes_component_(attributes_component),
        graph_component_(graph_component),
        listener_(this) {}

  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<AttributesDataRef>();
  }

  virtual void Initialize(event::Inputs* in, event::Outputs* out) {
    Run(in, out);
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) { Run(in, out); }

 private:
  void Run(event::Inputs* in, event::Outputs* out) {
    auto entity = in->Get<entity::EntityRef>(0);
    if (entity->IsValid()) {
      graph_component_->RegisterListener(kEventIdEntityAttributesChanged,
                                         entity, &listener_);
    }
    out->Set(0, AttributesDataRef(attributes_component_, *entity));
  }

  AttributesComponent* attributes_component_;
  GraphComponent* graph_component_;
  event::NodeEventListener listener_;
};

// Returns the position from the given transfrom data.
class GetAttributeNode : public event::BaseNode {
 public:
  static void OnRegister(event::NodeSignature* node_sig) {
    node_sig->AddInput<AttributesDataRef>();
    node_sig->AddInput<int>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(event::Inputs* in, event::Outputs* out) {
    auto attributes_ref = in->Get<AttributesDataRef>(0);
    auto index = in->Get<int>(1);
    auto attributes_data = attributes_ref->GetComponentData();
    if (attributes_data) {
      out->Set(0, attributes_data->attribute(*index));
    }
  }
};

void InitializeAttributesModule(event::EventSystem* event_system,
                                AttributesComponent* attributes_component,
                                GraphComponent* graph_component) {
  event::Module* module = event_system->AddModule("attributes");
  auto attributes_ctor = [attributes_component, graph_component]() {
    return new AttributesNode(attributes_component, graph_component);
  };
  module->RegisterNode<AttributesNode>("attributes", attributes_ctor);
  module->RegisterNode<GetAttributeNode>("get_attribute");
}

}  // fpl_project
}  // fpl
