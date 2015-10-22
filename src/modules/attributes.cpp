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

#include "breadboard/base_node.h"
#include "breadboard/event.h"
#include "breadboard/event_system.h"
#include "component_library/graph.h"
#include "components/attributes.h"
#include "entity/entity_manager.h"

using fpl::component_library::GraphComponent;

namespace fpl {
namespace zooshi {

// Returns the attributes component data of the given entity.
class AttributesNode : public breadboard::BaseNode {
 public:
  AttributesNode(AttributesComponent* attributes_component,
                 GraphComponent* graph_component)
      : attributes_component_(attributes_component),
        graph_component_(graph_component) {
    (void)graph_component_;
  }

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<AttributesDataRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) { Run(args); }

  virtual void Execute(breadboard::NodeArguments* args) { Run(args); }

 private:
  void Run(breadboard::NodeArguments* args) {
    auto entity = args->GetInput<entity::EntityRef>(0);
    args->SetOutput(0, AttributesDataRef(attributes_component_, *entity));
  }

  AttributesComponent* attributes_component_;
  GraphComponent* graph_component_;
};

// Returns the value of the given attribute.
class GetAttributeNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<AttributesDataRef>();
    node_sig->AddInput<int>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto attributes_ref = args->GetInput<AttributesDataRef>(0);
    auto index = args->GetInput<int>(1);
    auto attributes_data = attributes_ref->GetComponentData();
    if (attributes_data) {
      args->SetOutput(0, attributes_data->attributes[*index]);
    }
  }
};

// Sets the value of the given attribute to the given value.
class SetAttributeNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<AttributesDataRef>();
    node_sig->AddInput<int>();
    node_sig->AddInput<float>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto attributes_ref = args->GetInput<AttributesDataRef>(0);
    auto index = args->GetInput<int>(1);
    auto value = args->GetInput<float>(2);
    auto attributes_data = attributes_ref->GetComponentData();
    if (attributes_data) {
      attributes_data->attributes[*index] = *value;
    }
  }
};

void InitializeAttributesModule(breadboard::EventSystem* event_system,
                                AttributesComponent* attributes_component,
                                GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("attributes");
  auto attributes_ctor = [attributes_component, graph_component]() {
    return new AttributesNode(attributes_component, graph_component);
  };
  module->RegisterNode<AttributesNode>("attributes", attributes_ctor);
  module->RegisterNode<GetAttributeNode>("get_attribute");
  module->RegisterNode<SetAttributeNode>("set_attribute");
}

}  // zooshi
}  // fpl
