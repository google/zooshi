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

#include "breadboard/module_registry.h"
#include "breadboard/base_node.h"
#include "mathfu/glsl_mappings.h"

using breadboard::BaseNode;
using breadboard::Module;
using breadboard::ModuleRegistry;
using breadboard::NodeArguments;
using breadboard::NodeSignature;
using mathfu::vec3;

namespace fpl {
namespace zooshi {

// Increment the given achievement count.
class IncrementAchievementNode : public BaseNode {
 public:
  IncrementAchievementNode(const Config* config, GPGManager* gpg_manager)
      : config_(config), gpg_manager_(gpg_manager) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();
    node_sig->AddInput<std::string>();
  }

  virtual void Execute(NodeArguments* args) {
    auto name = args->GetInput<std::string>(1);
    auto achievement =
        config_->gpg_config()->achievements()->LookupByKey(name->c_str());
    gpg_manager_->IncrementAchievement(achievement->id()->c_str());
  }

 private:
  const Config* config_;
  GPGManager* gpg_manager_;
};

// Grant specified achievement.
class GrantAchievementNode : public BaseNode {
 public:
  GrantAchievementNode(const Config* config, GPGManager* gpg_manager)
      : config_(config), gpg_manager_(gpg_manager) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<int32_t>();
    node_sig->AddInput<std::string>();
  }

  virtual void Execute(NodeArguments* args) {
    auto flag = args->GetInput<int32_t>(0);
    if (*flag > 0) {
      auto name = args->GetInput<std::string>(1);
      auto achievement =
          config_->gpg_config()->achievements()->LookupByKey(name->c_str());
      gpg_manager_->UnlockAchievement(achievement->id()->c_str());
    }
  }

 private:
  const Config* config_;
  GPGManager* gpg_manager_;
};

// Submit score to the specified leaderboard.
class SubmitScoreNode : public BaseNode {
 public:
  SubmitScoreNode(const Config* config, GPGManager* gpg_manager)
      : config_(config), gpg_manager_(gpg_manager) {}

  static void OnRegister(NodeSignature* node_sig) {
    node_sig->AddInput<void>();         // Pulse indicating a game clear status.
    node_sig->AddInput<std::string>();  // Leaderboard name.
    node_sig->AddInput<float>();        // Score value.
  }

  virtual void Execute(NodeArguments* args) {
    // Need the pulse check since input 2 can be active.
    if (args->IsInputDirty(0)) {
      auto name = args->GetInput<std::string>(1);
      auto score = static_cast<int64_t>(*args->GetInput<float>(2));

      auto leaderboard_config = config_->gpg_config()->leaderboards();
      auto leaderboard = leaderboard_config->LookupByKey(name->c_str());
      gpg_manager_->SubmitScore(leaderboard->id()->c_str(), score);
    }
  }

 private:
  const Config* config_;
  GPGManager* gpg_manager_;
};

void InitializeGpgModule(ModuleRegistry* module_registry,
                         const Config* config, GPGManager* gpg_manager) {
  Module* module = module_registry->RegisterModule("gpg");
  auto increment_achievement_ctor = [config, gpg_manager]() {
    return new IncrementAchievementNode(config, gpg_manager);
  };
  module->RegisterNode<IncrementAchievementNode>("increment_achievement",
                                                 increment_achievement_ctor);

  auto grant_achievement_ctor = [config, gpg_manager]() {
    return new GrantAchievementNode(config, gpg_manager);
  };
  module->RegisterNode<GrantAchievementNode>("grant_achievement",
                                             grant_achievement_ctor);

  auto submit_score_ctor = [config, gpg_manager]() {
    return new SubmitScoreNode(config, gpg_manager);
  };
  module->RegisterNode<SubmitScoreNode>("submit_score", submit_score_ctor);
}

}  // zooshi
}  // fpl
