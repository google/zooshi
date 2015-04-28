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

#ifndef ZOOSHI_GUI_H
#define ZOOSHI_GUI_H

#include "imgui/imgui.h"
#include "material_manager.h"

namespace fpl {
namespace gui {

#define IMGUI_TEST 0
void TestGUI(MaterialManager &matman, FontManager &fontman, InputSystem &input);

}  // gui
}  // fpl
#endif  // ZOOSHI_GUI_H
