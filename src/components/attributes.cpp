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

#include "components/attributes.h"

#include "breadboard/event.h"
#include "components/services.h"
#include "corgi_component_library/graph.h"
#include "flatui/flatui.h"
#include "fplbase/asset_manager.h"
#include "fplbase/utilities.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::AttributesComponent,
                       fpl::zooshi::AttributesData)

namespace fpl {
namespace zooshi {

void AttributesComponent::Init() {
  ServicesComponent* services =
      entity_manager_->GetComponent<ServicesComponent>();
  input_system_ = services->input_system();
  font_manager_ = services->font_manager();
  asset_manager_ = services->asset_manager();
}

void AttributesComponent::AddFromRawData(corgi::EntityRef& entity,
                                         const void* /*raw_data*/) {
  AddEntity(entity);
}

corgi::ComponentInterface::RawDataUniquePtr AttributesComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  if (GetComponentData(entity) == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  fbb.Finish(CreateAttributesDef(fbb));
  return fbb.ReleaseBufferPointer();
}

}  // zooshi
}  // fpl
