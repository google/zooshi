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

#include "unlockable_manager.h"

#include "fplbase/utilities.h"

namespace fpl {
namespace zooshi {

const int kBufferSize = 64;

void UnlockableManager::InitializeType(
    UnlockableType type,
    const flatbuffers::Vector<
        flatbuffers::Offset<fpl::zooshi::UnlockableConfig>>* config) {
  configs_[type] = config;
  unlockables_[type].resize(config->size());
  char buffer[kBufferSize];
  for (int i = 0; i < static_cast<int>(config->size()); ++i) {
    bool unlocked = true;
    if (!config->Get(i)->starts_unlocked()) {
      GetPreferenceString(buffer, kBufferSize, type, i);
      unlocked = fplbase::LoadPreference(buffer, 0);
    }
    unlockables_[type][i] = unlocked;
    if (!unlocked) {
      remaining_locked_[type]++;
      remaining_locked_total_++;
    }
  }
}

void UnlockableManager::Unlock(UnlockableType type, size_t index) {
  SetUnlock(type, index, true);
}

void UnlockableManager::SetUnlock(UnlockableType type, size_t index,
                                  bool unlocked) {
  if (unlockables_[type][index] != unlocked) {
    unlockables_[type][index] = unlocked;
    remaining_locked_[type] += unlocked ? -1 : 1;
    remaining_locked_total_ += unlocked ? -1 : 1;
    char buffer[kBufferSize];
    GetPreferenceString(buffer, kBufferSize, type, index);
    fplbase::SavePreference(buffer, unlocked ? 1 : 0);
  }
}

bool UnlockableManager::UnlockRandom(Unlockable* unlockable) {
  if (remaining_locked_total_ == 0) {
    return false;
  }

  int to_unlock = mathfu::RandomInRange<int>(0, remaining_locked_total_);
  int type = 0;
  for (; type < UnlockableType_Size; ++type) {
    if (to_unlock < remaining_locked_[type]) {
      break;
    } else {
      to_unlock -= remaining_locked_[type];
    }
  }
  // It should have fallen into one of the type buckets.
  assert(type < UnlockableType_Size);

  for (int i = 0; i < static_cast<int>(unlockables_[type].size()); ++i) {
    if (!unlockables_[type][i]) {
      if (to_unlock == 0) {
        Unlock(static_cast<UnlockableType>(type), i);
        if (unlockable != nullptr) {
          unlockable->type = static_cast<UnlockableType>(type);
          unlockable->index = i;
          unlockable->config = configs_[type]->Get(i);
        }
        return true;
      } else {
        --to_unlock;
      }
    }
  }
  // It should have found something to unlock.
  assert(false);
  return false;
}

void UnlockableManager::UnlockAll() {
  for (int i = 0; i < UnlockableType_Size; ++i) {
    for (int j = 0; j < static_cast<int>(unlockables_[i].size()); ++j) {
      Unlock(static_cast<UnlockableType>(i), j);
    }
  }
}

void UnlockableManager::LockAll() {
  for (int i = 0; i < UnlockableType_Size; ++i) {
    for (int j = 0; j < static_cast<int>(unlockables_[i].size()); ++j) {
      if (!configs_[i]->Get(j)->starts_unlocked()) {
        SetUnlock(static_cast<UnlockableType>(i), j, false);
      }
    }
  }
}

void UnlockableManager::GetPreferenceString(char* buffer, int buffer_len,
                                            UnlockableType type, size_t index) {
  snprintf(buffer, buffer_len, "unlockable.%s.%s", EnumNameUnlockableType(type),
           configs_[type]->Get(index)->name()->c_str());
}

}  // zooshi
}  // fpl
