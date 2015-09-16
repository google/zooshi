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

#ifndef COMPONENTS_LAP_DEPENDENT_H_
#define COMPONENTS_LAP_DEPENDENT_H_

#include "components_generated.h"
#include "entity/component.h"
#include "entity/entity_manager.h"
#include "events_generated.h"

namespace fpl {
namespace fpl_project {

// Data for lap dependent components.
struct LapDependentData {
  LapDependentData() : min_lap(0.0f), max_lap(0.0f), currently_active(false) {}
  float min_lap;
  float max_lap;
  bool currently_active;
};

class LapDependentComponent : public entity::Component<LapDependentData> {
 public:

  virtual void Init();
  virtual void AddFromRawData(entity::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const entity::EntityRef& entity) const;
  virtual void InitEntity(entity::EntityRef& entity);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);

  void ActivateAllEntities();
  void DeactivateAllEntities();

 private:
  void ActivateEntity(entity::EntityRef& entity);
  void DeactivateEntity(entity::EntityRef& entity);
};

}  // fpl_project
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::fpl_project::LapDependentComponent,
                              fpl::fpl_project::LapDependentData)

#endif  // COMPONENTS_LAP_DEPENDENT_H_
