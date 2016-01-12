// Copyright 2016 Google Inc. All rights reserved.
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

#include "modules/ui_string.h"

#include "breadboard/base_node.h"
#include "breadboard/module_registry.h"
#include "corgi/entity_manager.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using corgi::component_library::GraphComponent;
using corgi::EntityRef;

namespace fpl {
namespace zooshi {

/// @brief Sets a string as the 3D text to be rendered by the
/// `Render3dTextComponent` for a given entity.
class Set3dTextStringNode : public BaseNode {
 public:
  enum { kInputEntity, kInputString };

  Set3dTextStringNode(Render3dTextComponent* render_3d_text_component)
      : render_3d_text_component_(render_3d_text_component) {}

  virtual ~Set3dTextStringNode() {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<EntityRef>(
        kInputEntity, "Entity",
        "The entity whose 3D text string should be set.");
    node_sig->AddInput<std::string>(
        kInputString, "String",
        "The input string to set as the text to be rendered in 3D.");
  }

  virtual void Execute(NodeArguments* args) {
    const auto entity = args->GetInput<EntityRef>(kInputEntity);
    const auto text = args->GetInput<std::string>(kInputString);
    Render3dTextData* render_3d_text_data =
        render_3d_text_component_->GetComponentData(*entity);
    if (render_3d_text_data) {
      render_3d_text_data->text = *text;
    }
  }

 private:
  Render3dTextComponent* render_3d_text_component_;
};

void InitializeUiStringModule(ModuleRegistry* module_registry,
                              Render3dTextComponent* render_3d_text_component) {
  auto set_3d_text_string_node_ctor = [render_3d_text_component]() {
    return new Set3dTextStringNode(render_3d_text_component);
  };
  Module* module = module_registry->RegisterModule("ui_string");
  module->RegisterNode<Set3dTextStringNode>("set_3d_text_string",
                                            set_3d_text_string_node_ctor);
}

}  // zooshi
}  // fpl
