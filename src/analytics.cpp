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

#include "analytics.h"

#include "components/player.h"

namespace fpl {
namespace zooshi {

// Events and parameters used for when a patron is fed.
const char* kEventPatronFed = "patron_fed";
const char* kParameterPatronType = "patron_type";

// Events to track when gameplay starts and finishes.
const char* kEventGameplayStart = "gameplay_start";
const char* kEventGameplayFinished = "gameplay_finished";

// Parameter for tracking elapsed time since gameplay started, to be included
// with other events, such as gameplay finished to determine how long the
// gameplay session was.
const char* kParameterElapsedLevelTime = "elapsed_level_time";

// Parameter used to track the control scheme being used.
const char* kParameterControlScheme = "control_scheme";

const char* AnalyticsControlValue(const World* world) {
  if (world->is_in_cardboard()) return "VR";
  if (world->entity_manager
          .GetComponentData<PlayerData>(world->active_player_entity)
          ->input_controller()
          ->controller_type() == kControllerGamepad) {
    return "gamepad";
  }
#ifdef __ANDROID__
  if (world->onscreen_controller->enabled()) {
    return "onscreen";
  }
#endif  // __ANDROID__
  return "default";
}

firebase::analytics::Parameter AnalyticsControlParameter(const World* world) {
  return firebase::analytics::Parameter(kParameterControlScheme,
                                        AnalyticsControlValue(world));
}

}  // zooshi
}  // fpl
