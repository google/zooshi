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

#include <vector>
#include <set>
#include "components_generated.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix_4x4.h"
#include "fplutil/intrusive_list.h"

namespace fpl {
namespace fpl_project {

struct TransformData;

// Data for scene object components.
struct TransformData {
  TransformData()
      : position(mathfu::kZeros3f),
        scale(mathfu::kOnes3f),
        orientation(mathfu::quat::identity),
        owner(),
        parent(),
        child_node(),
        children() {}

  mathfu::vec3 position;
  mathfu::vec3 scale;
  mathfu::quat orientation;
  mathfu::mat4 world_transform;

  // A reference to the entity that owns this component data.
  entity::EntityRef owner;

  // A reference to the parent entity.
  entity::EntityRef parent;

  // Child IDs we will need to export.
  std::set<std::string> child_ids;

  // Child IDs we will be linking up on the next update; we couldn't link
  // them before because they may not have been loaded.
  std::vector<std::string> pending_child_ids;

  // The list of children.
  intrusive_list_node child_node;
  intrusive_list<TransformData, &TransformData::child_node> children;

  // We construct the matrix by hand here, because we know that it will
  // always be a composition of rotation, scale, and translation, so we
  // can do things a bit more cleanly than 3 4x4 matrix multiplications.
  mathfu::mat4 GetTransformMatrix() {
    // Start with rotation:
    mathfu::mat3 rot = orientation.ToMatrix();

    // Break it up into columns:
    mathfu::vec4 c0 = mathfu::vec4(rot[0], rot[3], rot[6], 0);
    mathfu::vec4 c1 = mathfu::vec4(rot[1], rot[4], rot[7], 0);
    mathfu::vec4 c2 = mathfu::vec4(rot[2], rot[5], rot[8], 0);
    mathfu::vec4 c3 = mathfu::vec4(0, 0, 0, 1);

    // Apply scale:
    c0 *= scale.x();
    c1 *= scale.y();
    c2 *= scale.z();

    // Apply translation:
    c3[0] = position.x();
    c3[1] = position.y();
    c3[2] = position.z();

    // Compose and return result:
    return mathfu::mat4(c0, c1, c2, c3);
  }
};

class TransformComponent : public entity::Component<TransformData> {
 public:
  mathfu::vec3 WorldPosition(entity::EntityRef entity);
  mathfu::quat WorldOrientation(entity::EntityRef entity);
  mathfu::mat4 WorldTransform(entity::EntityRef entity);

  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(entity::EntityRef& entity) const;

  virtual void InitEntity(entity::EntityRef& entity);
  virtual void CleanupEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  void AddChild(entity::EntityRef& child, entity::EntityRef& parent);
  void RemoveChild(entity::EntityRef& entity);

  void PostLoadFixup();

  // Take any pending child IDs and set up the child links.
  void UpdateChildLinks(entity::EntityRef& entity);

 private:
  void UpdateWorldPosition(entity::EntityRef& entity, mathfu::mat4 transform);
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::TransformComponent,
                              fpl::fpl_project::TransformData,
                              ComponentDataUnion_TransformDef)

#endif  // COMPONENTS_TRANSFORM_H_
