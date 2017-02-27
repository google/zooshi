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

#ifndef ZOOSHI_REMOTE_CONFIG_H_
#define ZOOSHI_REMOTE_CONFIG_H_

#include "firebase/app.h"

namespace fpl {
namespace zooshi {

// The lookup key for where to place the AdMob rewarded video.
extern const char* kConfigRewardedVideoLocation;

// The lookup keys for various menu labels.
extern const char* kConfigMenuPlayGame;
extern const char* kConfigMenuSendInvite;
extern const char* kConfigMenuOfferVideo;

void InitializeRemoteConfig(const firebase::App& app);

}  // zooshi
}  // fpl

#endif  // ZOOSHI_REMOTE_CONFIG_H_
