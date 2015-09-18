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

#include "component_library/physics.h"
#include "entity/entity_manager.h"
#include "event/event_system.h"
#include "event/base_node.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace fpl_project {

using fpl::component_library::CollisionData;
using fpl::component_library::PhysicsComponent;

// Returns the rail denizen component data of the given entity.
// This node takes no inputs, but is marked as dirty when a collision occurs.
class PhysicsCollisionNode : public event::BaseNode {
 public:
  PhysicsCollisionNode(PhysicsComponent* physics_component)
      : physics_component_(physics_component) {}

  static void OnRegister(event::NodeSignature* node_sig) {
    // Triggers when a collision occurs.
    node_sig->AddOutput<void>();

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

    // Responds to collision events.
    node_sig->AddListener(fpl::component_library::kCollisionEventId);
  }

  virtual void Initialize(event::NodeArguments* args) {
    args->BindBroadcaster(0, &physics_component_->broadcaster());
  }

  virtual void Execute(event::NodeArguments* args) {
    CollisionData& collision_data = physics_component_->collision_data();
    args->SetOutput(0);
    args->SetOutput(1, collision_data.this_entity);
    args->SetOutput(2, collision_data.this_position);
    args->SetOutput(3, collision_data.this_tag);
    args->SetOutput(4, collision_data.other_entity);
    args->SetOutput(5, collision_data.other_position);
    args->SetOutput(6, collision_data.other_tag);
  }

 private:
  PhysicsComponent* physics_component_;
};

void InitializePhysicsModule(event::EventSystem* event_system,
                             PhysicsComponent* physics_component) {
  event::Module* module = event_system->AddModule("physics");
  auto physics_ctor = [physics_component]() {
    return new PhysicsCollisionNode(physics_component);
  };
  module->RegisterNode<PhysicsCollisionNode>("collision", physics_ctor);
}

}  // fpl_project
}  // fpl
