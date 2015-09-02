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

#ifndef FPL_ZOOSHI_MODULES_ENTITY_H_
#define FPL_ZOOSHI_MODULES_ENTITY_H_

#include "event/event_system.h"
#include "entity/entity_manager.h"
#include "component_library/meta.h"

namespace fpl {
namespace fpl_project {

class ServicesComponent;

template <typename T>
class ComponentDataRef {
 public:
  ComponentDataRef() : component_(nullptr), entity_() {}

  ComponentDataRef(T* component, entity::EntityRef entity)
      : component_(component), entity_(entity) {}

  // Convenience accessors.
  typename T::value_type* GetComponentData() {
    return component_->GetComponentData(entity_);
  }
  const typename T::value_type* GetComponentData() const {
    return component_->GetComponentData(entity_);
  }

 private:
  T* component_;
  entity::EntityRef entity_;
};

void InitializeEntityModule(event::EventSystem* event_system,
                            ServicesComponent* services_component,
                            component_library::MetaComponent* meta_component);

}  // fpl_project
}  // fpl

#endif  // FPL_ZOOSHI_MODULES_ENTITY_H_
