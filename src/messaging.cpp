// Copyright 2017 Google Inc. All rights reserved.
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

#include "messaging.h"

#include "fplbase/utilities.h"
#include "world.h"

namespace fpl {
namespace zooshi {

const char* kDisplayMessageKey = "display_message";
const char* kDefaultDisplayMessage = "Thanks for playing Zooshi!";

const char* kBonusApplyTypeKey = "bonus_apply_type";
const char* kBonusValueKey = "bonus_value";
const char* kBonusApplyCountKey = "bonus_apply_count";
const char* kBonusUniqueKeyKey = "bonus_unique_key";

MessageListener::MessageListener()
    : message_mutex_(SDL_CreateMutex()),
      has_pending_message_(false),
      display_message_(),
      bonus_apply_type_(BonusApplyType_Addition),
      bonus_value_(0.0f),
      bonus_apply_count_(0),
      bonus_unique_key_(0) {}

const char* GetString(const firebase::messaging::Message& message,
                      const char* key, const char* default_value) {
  auto it = message.data.find(key);
  if (it != message.data.end()) {
    return it->second.c_str();
  } else {
    return default_value;
  }
}

int GetInt(const firebase::messaging::Message& message, const char* key,
           int default_value) {
  auto it = message.data.find(key);
  if (it != message.data.end()) {
    return atoi(it->second.c_str());
  } else {
    return default_value;
  }
}

float GetFloat(const firebase::messaging::Message& message,
               const char* key, float default_value) {
  auto it = message.data.find(key);
  if (it != message.data.end()) {
    return static_cast<float>(atof(it->second.c_str()));
  } else {
    return default_value;
  }
}

void MessageListener::OnMessage(const firebase::messaging::Message& message) {
  SDL_LockMutex(message_mutex_);

  has_pending_message_ = true;

  display_message_ =
      GetString(message, kDisplayMessageKey, kDefaultDisplayMessage);

  int apply_type = GetInt(message, kBonusApplyTypeKey,
                          static_cast<int>(BonusApplyType_Addition));
  if (apply_type < 0 || apply_type >= static_cast<int>(BonusApplyType_Size)) {
    apply_type = 0;
  }
  bonus_apply_type_ = static_cast<BonusApplyType>(apply_type);
  bonus_value_ = GetFloat(message, kBonusValueKey, 0);
  bonus_apply_count_ = GetInt(message, kBonusApplyCountKey, 1);
  bonus_unique_key_ =
      GetInt(message, kBonusUniqueKeyKey, XpSystem::kNonUniqueKey);

  SDL_UnlockMutex(message_mutex_);

  fplbase::LogInfo("Received a message: %s", message.message_id.c_str());
}

void MessageListener::OnTokenReceived(const char* token) {
  fplbase::LogInfo("Messaging Token Received: %s", token);
}

std::string MessageListener::HandlePendingMessage(World* world) {
  SDL_LockMutex(message_mutex_);

  has_pending_message_ = false;
  if (bonus_value_ > 0) {
    fplbase::LogInfo("Messaging adding a bonus of %d, %f, %d, %d",
                     static_cast<int>(bonus_apply_type_), bonus_value_,
                     bonus_apply_count_, bonus_unique_key_);
    world->xp_system->AddBonus(bonus_apply_type_, bonus_value_,
                               bonus_apply_count_, bonus_unique_key_);
  }
  std::string cached = display_message_;
  SDL_UnlockMutex(message_mutex_);
  return cached;
}

void StartReceivingMessages(World* world) {
  firebase::messaging::SetListener(world->message_listener);
}

void StopReceivingMessages() {
  firebase::messaging::SetListener(nullptr);
}

}  // zooshi
}  // fpl
