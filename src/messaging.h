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

#ifndef ZOOSHI_MESSAGING_H_
#define ZOOSHI_MESSAGING_H_

#include <string>

#include "mathfu/internal/disable_warnings_begin.h"

#include "firebase/messaging.h"

#include "mathfu/internal/disable_warnings_end.h"

#include "SDL_thread.h"
#include "xp_system.h"

namespace fpl {
namespace zooshi {

struct World;

class MessageListener : public firebase::messaging::Listener {
 public:
  MessageListener();

  virtual void OnMessage(const firebase::messaging::Message& message);

  virtual void OnTokenReceived(const char* token);

  std::string HandlePendingMessage(World* world);

  bool has_pending_message() const { return has_pending_message_; }

 private:
  SDL_mutex* message_mutex_;
  bool has_pending_message_;

  std::string display_message_;

  // Data related to applying a bonus when handling the message.
  BonusApplyType bonus_apply_type_;
  float bonus_value_;
  int bonus_apply_count_;
  int bonus_unique_key_;
};

void StartReceivingMessages(World* world);
void StopReceivingMessages();

}  // zooshi
}  // fpl

#endif  // ZOOSHI_MESSAGING_H_
