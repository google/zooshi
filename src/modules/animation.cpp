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

#include "modules/animation.h"

#include <string>

#include "breadboard/base_node.h"
#include "breadboard/event_system.h"
#include "component_library/animation.h"
#include "component_library/meta.h"
#include "entity/entity_manager.h"

namespace fpl {
namespace fpl_project {

using entity::EntityRef;
using fpl::component_library::AnimationComponent;
using fpl::component_library::GraphComponent;
using fpl::component_library::kAnimationCompleteEventId;

// Executes when the animation on the given entity is complete.
class AnimationCompleteNode : public breadboard::BaseNode {
 public:
  AnimationCompleteNode(GraphComponent* graph_component)
      : graph_component_(graph_component) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<EntityRef>();
    node_sig->AddOutput<void>();
    node_sig->AddListener(kAnimationCompleteEventId);
  }

  virtual void Initialize(breadboard::NodeArguments* args) {
    EntityRef entity = *args->GetInput<EntityRef>(0);
    if (entity) {
      args->BindBroadcaster(0, graph_component_->GetCreateBroadcaster(entity));
    }
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

void InitializeAnimationModule(breadboard::EventSystem* event_system,
                               GraphComponent* graph_component) {
  breadboard::Module* module = event_system->AddModule("animation");
  auto animation_complete_ctor = [graph_component]() {
    return new AnimationCompleteNode(graph_component);
  };
  module->RegisterNode<AnimationCompleteNode>("animation_complete",
                                              animation_complete_ctor);
}

}  // fpl_project
}  // fpl
