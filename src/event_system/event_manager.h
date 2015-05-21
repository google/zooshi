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

#ifndef FPL_EVENT_MANAGER_H_
#define FPL_EVENT_MANAGER_H_

#include <vector>

#include "event_system/event_listener.h"
#include "event_system/event_payload.h"
#include "event_system/event_registry.h"

namespace fpl {
namespace event {

// The event manager manages a listener queue for each event id. Any number of
// event types may be defined. When an event is broadcast using the
// `BroadcastEvent` function it will be sent to all EventListeners registered to
// that event id.
class EventManager {
 public:
  // Creates an event manager with the given number of event queues.
  // All event ids must be in the range [0, size).
  EventManager(int size) : listener_lists_(size) {}

  // Send an event with no payload to a specific EventListener.
  static void SendEvent(EventListener* listener, int event_id);

  // Send an event with the given payload to a specific EventListener.
  template <typename T>
  static void SendEvent(EventListener* listener, int event_id,
                        const T& payload) {
    assert(event_id == EventIdRegistry<T>::kEventId);
    listener->OnEvent(event_id, EventPayload(event_id, &payload));
  }

  // Broadcast an event with no payload to all events listeners registered to
  // `event_id`.
  void BroadcastEvent(int event_id);

  // Broadcast an event with the given payload to all events listeners
  // registered to `event_id`.
  template <typename T>
  void BroadcastEvent(int event_id, const T& event_data) {
    std::vector<EventListener*>& list = listener_lists_[event_id];
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
      EventManager::SendEvent(*iter, event_id, event_data);
    }
  }

  // Register a listener to with the given `event_id`. A listener may be
  // registered with any number event ids.
  void RegisterListener(int event_id, EventListener* listener);

  // Unregister a listener to with the given `event_id`. A listener may be
  // registered with any number event ids.
  void UnregisterListener(int event_id, EventListener* listener);

 private:
  std::vector<std::vector<EventListener*>> listener_lists_;
};

}  // event
}  // fpl

#endif  // FPL_EVENT_MANAGER_H_

