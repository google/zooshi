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
#include "breadboard/module_registry.h"
#include "components/attributes.h"
#include "corgi/entity_manager.h"
#include "corgi_component_library/graph.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using corgi::component_library::GraphComponent;
using corgi::EntityRef;

namespace fpl {
namespace zooshi {

// Returns the value of the given attribute.
class GetAttributeNode : public BaseNode {
 public:
  GetAttributeNode(AttributesComponent* attributes_component)
      : attributes_component_(attributes_component) {}
  virtual ~GetAttributeNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddInput<int>();
    node_sig->AddOutput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      auto entity = args->GetInput<EntityRef>(1);
      auto index = args->GetInput<int>(2);
      auto attributes_data = attributes_component_->GetComponentData(*entity);
      if (attributes_data) {
        args->SetOutput(0, attributes_data->attributes[*index]);
      }
    }
  }

 private:
  AttributesComponent* attributes_component_;
};

// Sets the value of the given attribute to the given value.
class SetAttributeNode : public BaseNode {
 public:
  SetAttributeNode(AttributesComponent* attributes_component)
      : attributes_component_(attributes_component) {}
  virtual ~SetAttributeNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddInput<int>();
    node_sig->AddInput<float>();
  }

  virtual void Execute(NodeArguments* args) {
    if (args->IsInputDirty(0)) {
      auto entity = args->GetInput<EntityRef>(1);
      auto index = args->GetInput<int>(2);
      auto value = args->GetInput<float>(3);
      auto attributes_data = attributes_component_->GetComponentData(*entity);
      if (attributes_data) {
        attributes_data->attributes[*index] = *value;
      }
    }
  }

 private:
  AttributesComponent* attributes_component_;
};

void InitializeAttributesModule(ModuleRegistry* module_registry,
                                AttributesComponent* attributes_component) {
  auto get_attribute_ctor = [attributes_component]() {
    return new GetAttributeNode(attributes_component);
  };
  auto set_attribute_ctor = [attributes_component]() {
    return new SetAttributeNode(attributes_component);
  };
  Module* module = module_registry->RegisterModule("attributes");
  module->RegisterNode<GetAttributeNode>("get_attribute", get_attribute_ctor);
  module->RegisterNode<SetAttributeNode>("set_attribute", set_attribute_ctor);
}

}  // zooshi
}  // fpl
