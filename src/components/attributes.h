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

#ifndef FPL_COMPONENTS_ATTRIBUTES_H_
#define FPL_COMPONENTS_ATTRIBUTES_H_

#include "config_generated.h"
#include "attributes_generated.h"
#include "entity/component.h"
#include "entity/entity_manager.h"
#include "event_system/event_listener.h"

namespace fpl {

class InputSystem;
class MaterialManager;
class FontManager;

namespace event {

class EventManager;

}  // event
namespace fpl_project {

// Data for scene object components.
struct AttributesData {
 public:
  AttributesData() {}

  float attributes[AttributeDef_Size];
};

class AttributesComponent : public entity::Component<AttributesData>,
                            public event::EventListener {
 public:
  AttributesComponent() {}

  void Initialize(InputSystem* input_system, MaterialManager* material_manager,
                  FontManager* font_manager,
                  event::EventManager* event_manager);

  virtual void Init() {}
  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  virtual void InitEntity(entity::EntityRef& /*entity*/) {}
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  virtual void OnEvent(const event::EventPayload& event_payload);

 private:
  InputSystem* input_system_;
  MaterialManager* material_manager_;
  FontManager* font_manager_;
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::AttributesComponent,
                              fpl::fpl_project::AttributesData,
                              ComponentDataUnion_AttributesDef)

#endif  // FPL_COMPONENTS_ATTRIBUTES_H_

