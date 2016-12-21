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

#include "xp_system.h"

#include <fplbase/utilities.h>

namespace fpl {
namespace zooshi {

const char* kCurrentXPKey = "zooshi.current_xp";

void XpSystem::Initialize(const Config* config) {
  config_ = config;
  xp_for_reward_ = config->xp_for_reward();
  current_xp_ = fplbase::LoadPreference(kCurrentXPKey, 0);
}

int XpSystem::ApplyBonuses(int base_xp, bool consume_bonuses) {
  // Apply the bonuses in the order of the enum.
  for (int i = BonusApplyType_Multiply; i < BonusApplyType_Size; ++i) {
    for (auto it = bonuses[i].begin(); it != bonuses[i].end();) {
      switch (i) {
        case BonusApplyType_Multiply:
          base_xp *= it->value;
          break;
        case BonusApplyType_Addition:
          base_xp += it->value;
          break;
        default:
          break;
      }
      // If after consuming the bonus the apply count is finished, remove the
      // bonus from the list.
      if (consume_bonuses && --(it->apply_count) == 0) {
        it = bonuses[i].erase(it);
      } else {
        ++it;
      }
    }
  }

  return base_xp;
}

bool XpSystem::GrantXP(int xp) {
  bool earned_reward = false;
  current_xp_ += xp;
  if (current_xp_ >= xp_for_reward_) {
    current_xp_ %= xp_for_reward_;
    earned_reward = true;
  }
  fplbase::SavePreference(kCurrentXPKey, current_xp_);
  return earned_reward;
}

void XpSystem::AddBonus(BonusApplyType type, float value, int apply_count,
                        int unique_key) {
  if (apply_count <= 0) apply_count = 1;
  if (unique_key != XpSystem::kNonUniqueKey) {
    // If a unique key is provided, remove any similar typed bonus with the same
    // key first.
    bonuses[type].remove_if([unique_key](const BonusData& data) {
      return data.unique_key == unique_key;
    });
  }
  bonuses[type].push_back(BonusData(value, apply_count, unique_key));
}

}  // zooshi
}  // fpl
