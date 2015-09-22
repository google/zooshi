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

#include "modules/gpg.h"

#include "breadboard/event_system.h"
#include "breadboard/base_node.h"
#include "mathfu/glsl_mappings.h"

using mathfu::vec3;

namespace fpl {
namespace fpl_project {

// Increment the given achievement count.
class IncrementAchievementNode : public breadboard::BaseNode {
 public:
  IncrementAchievementNode(const Config* config, GPGManager* gpg_manager)
      : config_(config), gpg_manager_(gpg_manager) {}

  static void OnRegister(breadboard::NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<std::string>();
  }

  virtual void Execute(breadboard::NodeArguments* args) {
    auto name = args->GetInput<std::string>(1);
    auto achievement =
        config_->gpg_config()->achievements()->LookupByKey(name->c_str());
    gpg_manager_->IncrementAchievement(achievement->id()->c_str());
  }

 private:
  const Config* config_;
  GPGManager* gpg_manager_;
};

void InitializeGpgModule(breadboard::EventSystem* event_system,
                         const Config* config, GPGManager* gpg_manager) {
  breadboard::Module* module = event_system->AddModule("gpg");
  auto increment_achievement_ctor = [config, gpg_manager]() {
    return new IncrementAchievementNode(config, gpg_manager);
  };
  module->RegisterNode<IncrementAchievementNode>("increment_achievement",
                                                 increment_achievement_ctor);
}

}  // fpl_project
}  // fpl
