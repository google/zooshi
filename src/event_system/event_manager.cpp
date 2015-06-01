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

#include <algorithm>

#include "event_system/event_manager.h"
#include "event_system/event_payload.h"
#include "event_system/event_registry.h"

namespace fpl {
namespace event {

void EventManager::RegisterListener(int event_id, EventListener* listener) {
  std::vector<EventListener*>& list = listener_lists_[event_id];
  list.push_back(listener);
}

void EventManager::UnregisterListener(int event_id, EventListener* listener) {
  std::vector<EventListener*>& list = listener_lists_[event_id];
  list.erase(std::remove(list.begin(), list.end(), listener), list.end());
}

}  // event
}  // fpl

