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

#include "modules/entity.h"

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/meta.h"
#include "components/services.h"
#include "entity/entity_manager.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace fpl_project {

// Returns the entity representing the player.
class PlayerEntityNode : public breadboard::BaseNode {
 public:
  PlayerEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddOutput<entity::EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->player_entity());
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->player_entity());
  }

 private:
  ServicesComponent* services_component_;
};

// Returns the entity representing the raft.
class RaftEntityNode : public breadboard::BaseNode {
 public:
  RaftEntityNode(ServicesComponent* services_component)
      : services_component_(services_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddOutput<entity::EntityRef>();
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->raft_entity());
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    args->SetOutput(0, services_component_->raft_entity());
  }

 private:
  ServicesComponent* services_component_;
};

// Given an input string, return the named entity.
class EntityNode : public breadboard::BaseNode {
 public:
  EntityNode(component_library::MetaComponent* meta_component)
      : meta_component_(meta_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<std::string>();
    node_sig->AddOutput<entity::EntityRef>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto entity_id = args->GetInput<std::string>(0);
    entity::EntityRef entity =
        meta_component_->GetEntityFromDictionary(entity_id->c_str());
    assert(entity.IsValid());
    args->SetOutput(0, entity);
  }

 private:
  component_library::MetaComponent* meta_component_;
};

// Given an input string, return the named entity.
class DeleteEntityNode : public breadboard::BaseNode {
 public:
  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<entity::EntityRef>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto entity_ref = args->GetInput<entity::EntityRef>(0);
    entity_manager_->DeleteEntity(*entity_ref);
  }

 private:
  entity::EntityManager* entity_manager_;
};

void InitializeEntityModule(breadboard::EventSystem* event_system,
                            ServicesComponent* services_component,
                            component_library::MetaComponent* meta_component) {
  auto entity_ctor = [meta_component]() {
    return new EntityNode(meta_component);
  };
  auto player_entity_ctor = [services_component]() {
    return new PlayerEntityNode(services_component);
  };
  auto raft_entity_ctor = [services_component]() {
    return new RaftEntityNode(services_component);
  };
  breadboard::Module* module = event_system->AddModule("entity");
  module->RegisterNode<EntityNode>("entity", entity_ctor);
  module->RegisterNode<PlayerEntityNode>("player_entity", player_entity_ctor);
  module->RegisterNode<RaftEntityNode>("raft_entity", raft_entity_ctor);
}

}  // fpl_project
}  // fpl
