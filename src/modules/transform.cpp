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

#include "modules/transform.h"

#include <string>

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/transform.h"
#include "entity/entity_manager.h"

using fpl::component_library::TransformData;
using fpl::component_library::TransformComponent;
using fpl::entity::EntityRef;

namespace fpl {
namespace zooshi {

// Returns the transform component data of the given entity.
class TransformNode : public breadboard::BaseNode {
 public:
  TransformNode(TransformComponent* transform_component)
      : transform_component_(transform_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<EntityRef>();
    node_sig->AddOutput<TransformDataRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto entity = args->GetInput<EntityRef>(1);
    args->SetOutput(0, TransformDataRef(transform_component_, *entity));
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }

 private:
  TransformComponent* transform_component_;
};

// Returns the child at the given index.
class ChildNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<TransformDataRef>();
    node_sig->AddInput<int>();
    node_sig->AddOutput<EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto transform_ref = args->GetInput<TransformDataRef>(0);
    if (transform_ref->entity()) {
      auto child_index = *args->GetInput<int>(1);
      TransformData* transform_data = transform_ref->GetComponentData();
      // Find the child at the given index.
      auto iter = transform_data->children.begin();
      while (child_index && iter != transform_data->children.end()) {
        ++iter;
        --child_index;
      }
      args->SetOutput(0, iter != transform_data->children.end() ? iter->owner
                                                                : EntityRef());
    } else {
      args->SetOutput(0, EntityRef());
    }
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }
};

void InitializeTransformModule(breadboard::EventSystem* event_system,
                               TransformComponent* transform_component) {
  breadboard::Module* module = event_system->AddModule("transform");
  auto transform_ctor = [transform_component]() {
    return new TransformNode(transform_component);
  };
  module->RegisterNode<TransformNode>("transform", transform_ctor);
  module->RegisterNode<ChildNode>("child");
}

}  // zooshi
}  // fpl
