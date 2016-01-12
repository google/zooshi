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

#ifndef FPL_ZOOSHI_MODULES_UI_STRING_H_
#define FPL_ZOOSHI_MODULES_UI_STRING_H_

#include "breadboard/module_registry.h"
#include "components/render_3d_text.h"
#include "corgi_component_library/graph.h"

namespace fpl {
namespace zooshi {

void InitializeUiStringModule(breadboard::ModuleRegistry* module_registry,
                              Render3dTextComponent* render_3d_text_component);
}  // zooshi
}  // fpl

#endif  // FPL_ZOOSHI_MODULES_UI_STRING_H_
