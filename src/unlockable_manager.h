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

#ifndef ZOOSHI_PROGRESSION_H_
#define ZOOSHI_PROGRESSION_H_

#include <assert.h>
#include <stdint.h>
#include <vector>

#include "config_generated.h"
#include "unlockables_generated.h"

namespace fpl {
namespace zooshi {

// Data about the unlockable.
struct Unlockable {
  // The type of the unlockable.
  UnlockableType type;
  // The index of the unlockable.
  size_t index;
};

// Tracks the unlockables of the game.
class UnlockableManager {
 public:
  // Initialize the given type with the provided config data.
  void InitializeType(
      UnlockableType type,
      const flatbuffers::Vector<
          flatbuffers::Offset<fpl::zooshi::UnlockableConfig>>* config);

  // Returns if the unlockable at the given type and index is unlocked.
  bool is_unlocked(UnlockableType type, size_t index) const {
    return unlockables_[type].at(index);
  }
  // The remaining number of locked values for a type.
  int remaining_locked(UnlockableType type) { return remaining_locked_[type]; }
  // The total remaining number of locked values for all types.
  int remaining_locked_total() { return remaining_locked_total_; }

  // Unlocks the unlockable at the given type and index.
  void Unlock(UnlockableType type, size_t index);
  // Unlocks a random unlockable, and returns the information about it.
  bool UnlockRandom(Unlockable* unlockable);
  // Unlocks all unlockables to be available.
  void UnlockAll();
  // Locks all unlockables back to the default settings.
  void LockAll();

 private:
  void GetPreferenceString(char* buffer, int buffer_len, UnlockableType type,
                           size_t index);
  void SetUnlock(UnlockableType type, size_t index, bool unlocked);

  // The cached configuration value for each type.
  const flatbuffers::Vector<flatbuffers::Offset<fpl::zooshi::UnlockableConfig>>*
      configs_[UnlockableType_Size];
  // Tracks each unlockable.
  std::vector<bool> unlockables_[UnlockableType_Size];
  // The remaining number of locked values per type.
  int remaining_locked_[UnlockableType_Size];
  // The total of the above array.
  int remaining_locked_total_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_PROGRESSION_H_
