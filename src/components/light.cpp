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

#include "components/light.h"
#include "components/services.h"
#include "flatbuffers/flatbuffers.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/utilities.h"

CORGI_DEFINE_COMPONENT(fpl::zooshi::LightComponent, fpl::zooshi::LightData)

namespace fpl {
namespace zooshi {

using fplbase::ColorRGBA;
using fplbase::Vec4ToColorRGBA;
using mathfu::vec3;
using scene_lab::SceneLab;

void LightComponent::AddFromRawData(corgi::EntityRef& entity,
                                    const void* raw_data) {
  auto light_def = static_cast<const LightDef*>(raw_data);

  LightData* data = AddEntity(entity);
  data->shadow_intensity = light_def->shadow_intensity();
  data->diffuse_color = LoadColorRGBA(light_def->diffuse_color());
  data->ambient_color = LoadColorRGBA(light_def->ambient_color());
  data->specular_color = LoadColorRGBA(light_def->specular_color());
  data->specular_exponent = light_def->specular_exponent();
}

corgi::ComponentInterface::RawDataUniquePtr LightComponent::ExportRawData(
    const corgi::EntityRef& entity) const {
  const LightData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;

  LightDefBuilder builder(fbb);
  builder.add_shadow_intensity(data->shadow_intensity);
  ColorRGBA diffuse = Vec4ToColorRGBA(data->diffuse_color);
  builder.add_diffuse_color(&diffuse);
  ColorRGBA ambient = Vec4ToColorRGBA(data->ambient_color);
  builder.add_ambient_color(&ambient);
  ColorRGBA specular = Vec4ToColorRGBA(data->specular_color);
  builder.add_specular_color(&specular);
  builder.add_specular_exponent(data->specular_exponent);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

}  // zooshi
}  // fpl
