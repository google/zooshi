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

#ifndef ZOOSHI_ANALYTICS_H_
#define ZOOSHI_ANALYTICS_H_

#include "firebase/analytics.h"
#include "firebase/analytics/event_names.h"
#include "firebase/analytics/parameter_names.h"
#include "world.h"

namespace fpl {
namespace zooshi {

// Events and parameters used for when a patron is fed.
extern const char* kEventPatronFed;
extern const char* kParameterPatronType;

// Events to track when gameplay starts and finishes.
extern const char* kEventGameplayStart;
extern const char* kEventGameplayFinished;

// Parameter for tracking elapsed time since gameplay started, to be included
// with other events, such as gameplay finished to determine how long the
// gameplay session was.
extern const char* kParameterElapsedLevelTime;

// Parameter used to track the control scheme being used.
extern const char* kParameterControlScheme;

// Helper function to get the value used with the control scheme parameter.
const char* AnalyticsControlValue(const World* world);
// Helper function to create the parameter defining the controller being used.
firebase::analytics::Parameter AnalyticsControlParameter(const World* world);

}  // zooshi
}  // fpl

#endif  // ZOOSHI_ANALYTICS_H_
