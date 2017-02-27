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

#ifndef FPL_ZOOSHI_COMPONENTS_SHADOWCONTROLLER_H_
#define FPL_ZOOSHI_COMPONENTS_SHADOWCONTROLLER_H_

#include "components_generated.h"
#include "corgi/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"

namespace fpl {
namespace zooshi {

// Data for scene object components.
struct ShadowControllerData {
  corgi::EntityRef shadow_caster;
};

class ShadowControllerComponent
    : public corgi::Component<ShadowControllerData> {
 public:
  virtual ~ShadowControllerComponent() {}

  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::ShadowControllerComponent,
                         fpl::zooshi::ShadowControllerData)

#endif  // FPL_ZOOSHI_COMPONENTS_SHADOWCONTROLLER_H_
