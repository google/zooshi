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

#ifndef COMPONENTS_FAMILY_H_
#define COMPONENTS_FAMILY_H_

#include "components_generated.h"
#include "entity/entity_manager.h"
#include "entity/component.h"
#include "intrusive_list.h"

namespace fpl {
namespace fpl_project {

// Data for scene object components.
struct FamilyData {
  FamilyData() : owner(), parent(), children(), child_node_() {}

  // A reference to the entity that owns this entity data.
  entity::EntityRef owner;

  // A reference to the parent entity.
  entity::EntityRef parent;

  // The list of children.
  IntrusiveList children;

  INTRUSIVE_GET_NODE_ACCESSOR(child_node_, child_node);
  INTRUSIVE_LIST_NODE_GET_CLASS_ACCESSOR(FamilyData, child_node_,
                                         GetInstanceFromChildNode)

 private:
  IntrusiveListNode child_node_;
};

class FamilyComponent : public entity::Component<FamilyData> {
 public:
  FamilyComponent(entity::EntityFactoryInterface* entity_factory)
      : entity_factory_(entity_factory) {}

  virtual void AddFromRawData(entity::EntityRef& parent, const void* raw_data);

  virtual void InitEntity(entity::EntityRef& entity);

 private:
  entity::EntityFactoryInterface* entity_factory_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::FamilyComponent,
                              fpl::fpl_project::FamilyData,
                              ComponentDataUnion_FamilyDef)

#endif  // COMPONENTS_FAMILY_H_

