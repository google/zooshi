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

#include "remote_config.h"

#include "firebase/future.h"
#include "firebase/remote_config.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace zooshi {

// Ordinarily a longer cache time would be recommended, but for development
// a shorter one is helpful.
const int kRemoteConfigCacheTime = 0;

const char* kConfigRewardedVideoLocation = "rewarded_video_location";

void InitializeRemoteConfig(const firebase::App& app) {
  firebase::remote_config::Initialize(app);

  // Set the default values.
  static const firebase::remote_config::ConfigKeyValue defaults[] = {
      {kConfigRewardedVideoLocation, "0"},  // 0 = kRewardedVideoLocationPregame
  };

  size_t default_count = sizeof(defaults) / sizeof(defaults[0]);
  firebase::remote_config::SetDefaults(defaults, default_count);

  firebase::remote_config::Fetch(kRemoteConfigCacheTime).OnCompletion(
      [](const firebase::Future<void>& completed_future, void* /*data*/) {
        firebase::remote_config::ActivateFetched();
      },
      nullptr);
}

}  // zooshi
}  // fpl
