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

#ifndef COMPONENTS_TRANSFORM_H_
#define COMPONENTS_TRANSFORM_H_

#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"

namespace fpl {

// Data for scene object components.
struct TransformData {
 public:
  TransformData() : matrix(mathfu::mat4::Identity()) {}

  // Position, orientation, and scale (in world-space) of the object.
  // TODO: Store this using vectors and quats so scale, position, rotation, etc.
  // can be accessed and mutated individually. b/20554361
  mathfu::mat4 matrix;

  void set_transform(mathfu::vec3 position, mathfu::vec3 scale,
                     mathfu::quat orientation) {
    matrix = mathfu::mat4::FromTranslationVector(position) *
             mathfu::mat4::FromScaleVector(scale) *
             mathfu::mat4::FromRotationMatrix(orientation.ToMatrix());
  }

};

class TransformComponent : public entity::Component<TransformData> {
 public:
  TransformComponent() {}

  virtual void AddFromRawData(entity::EntityRef& /*entity*/,
                              const void* /*raw_data*/) {}

  virtual void InitEntity(entity::EntityRef& /*entity*/) {}
};

}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(TransformComponent, TransformData,
                              ComponentDataUnion_TransformDef)

#endif  // COMPONENTS_TRANSFORM_H_
