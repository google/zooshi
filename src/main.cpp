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

#include "game.h"
#include "fplbase/utilities.h"

int FPL_main(int argc, char* argv[]) {
  fpl::zooshi::Game game;
  const char* binary_directory = argc > 0 ? argv[0] : "";
  if (!game.Initialize(binary_directory)) {
    fpl::LogError("FPL Game: init failed, exiting!");
    return 1;
  }

  game.Run();

  return 0;
}

MATHFU_DEFINE_GLOBAL_SIMD_AWARE_NEW_DELETE
