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

#include "modules/physics.h"

#include <string>

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/physics.h"
#include "entity/entity_manager.h"

namespace fpl {
namespace fpl_project {

using fpl::component_library::CollisionData;
using fpl::component_library::GraphComponent;
using fpl::component_library::PhysicsComponent;
using fpl::component_library::kCollisionEventId;
using fpl::entity::EntityRef;

class OnCollisionNode : public breadboard::BaseNode {
 public:
  OnCollisionNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kCollisionEventId);
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    auto entity = args->GetInput<EntityRef>(0);
    args->BindBroadcaster(0, graph_component_->GetCreateBroadcaster(*entity));
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    Initialize(args);
    if (args->IsListenerDirty(0)) {
      args->SetOutput(0);
    }
  }

 private:
  GraphComponent* graph_component_;
};

class CollisionDataNode : public breadboard::BaseNode {
 public:
  CollisionDataNode(PhysicsComponent* physics_component)
      : physics_component_(physics_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    // Fetch the collision data when triggered
    node_sig->AddInput<void>();

    // One of the entities involved in the collision, the location of the
    // entity, and an arbitrary tag.
    node_sig->AddOutput<entity::EntityRef>();
    node_sig->AddOutput<mathfu::vec3>();
    node_sig->AddOutput<std::string>();

    // The other entity involved in the collision, the location of the entity,
    // and an arbitrary tag.
    node_sig->AddOutput<entity::EntityRef>();
    node_sig->AddOutput<mathfu::vec3>();
    node_sig->AddOutput<std::string>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    CollisionData& collision_data = physics_component_->collision_data();
    args->SetOutput(0, collision_data.this_entity);
    args->SetOutput(1, collision_data.this_position);
    args->SetOutput(2, collision_data.this_tag);
    args->SetOutput(3, collision_data.other_entity);
    args->SetOutput(4, collision_data.other_position);
    args->SetOutput(5, collision_data.other_tag);
  }

  virtual void Execute(breadboard::NodeArguments* args) { Initialize(args); }

 private:
  PhysicsComponent* physics_component_;
};

void InitializePhysicsModule(breadboard::EventSystem* event_system,
                             PhysicsComponent* physics_component,
                             GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("physics");
  auto on_collision_ctor = [graph_component]() {
    return new OnCollisionNode(graph_component);
  };
  auto collision_data_ctor = [physics_component]() {
    return new CollisionDataNode(physics_component);
  };
  module->RegisterNode<OnCollisionNode>("on_collision", on_collision_ctor);
  module->RegisterNode<CollisionDataNode>("collision_data",
                                          collision_data_ctor);
}

}  // fpl_project
}  // fpl
