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

#ifndef ZOOSHI_XP_SYSTEM_H_
#define ZOOSHI_XP_SYSTEM_H_

#include "config_generated.h"

#include <list>

namespace fpl {
namespace zooshi {

enum BonusApplyType {
  BonusApplyType_Multiply = 0,
  BonusApplyType_Addition,
  BonusApplyType_Size,
};

class XpSystem {
 public:
  void Initialize(const Config* config);

  // Applies the tracked bonuses to the given xp value. If consume_bonuses is
  // true, this will consume one application of the bonuses.
  int ApplyBonuses(int base_xp, bool consume_bonuses);

  // Grants the given xp value, returns true if a reward should be given.
  // Note that bonuses are not applied to this value, and should be calculated
  // prior, should you want bonuses to be applied.
  // Note that if the threshold for a reward is passed multiple times, that
  // is not tracked.
  bool GrantXP(int xp);

  // Adds a bonus to be applied when calculating earned xp.
  // @param type How the bonus modifies the xp.
  // @param value The value to modify with.
  // @param apply_count The number of times to apply the bonus.
  // @param unique_key If not kNonUniqueKey, removes other bonuses of the
  //                   same type with same unique_key first.
  void AddBonus(BonusApplyType type, float value, int apply_count,
                int unique_key);

  // The current xp value that the player has.
  int current_xp() const { return current_xp_; }
  // The xp threshold needed before a reward should be given.
  int xp_for_reward() const { return xp_for_reward_; }
  // The amount of xp needed for the player to reach the next reward.
  int xp_until_reward() const { return xp_for_reward_ - current_xp_; }

  static const int kNonUniqueKey = UniqueBonusId_NonUnique;

 private:
  struct BonusData {
    BonusData(float value_, int apply_count_, int unique_key_)
        : value(value_), apply_count(apply_count_), unique_key(unique_key_) {}

    float value;
    int apply_count;
    int unique_key;
  };

  const Config* config_;
  int xp_for_reward_;
  int current_xp_;
  std::list<BonusData> bonuses[BonusApplyType_Size];
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_XP_SYSTEM_H_
