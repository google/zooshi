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

#ifndef FPL_ZOOSHI_COMPONENTS_LIGHT_H
#define FPL_ZOOSHI_COMPONENTS_LIGHT_H

#include "components_generated.h"
#include "config_generated.h"
#include "corgi/component.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace fpl {
namespace zooshi {

/// @brief Data for an entity registered with LightData component.
struct LightData {
  LightData()
      : shadow_intensity(0.0f),
        specular_exponent(0.0f),
        diffuse_color(mathfu::kZeros4f),
        ambient_color(mathfu::kZeros4f),
        specular_color(mathfu::kZeros4f) {}
  /// @brief Intensity of shadows. 1.0 = solid black, 0.0 = clear shadows.
  float shadow_intensity;

  /// @brief Phong exponent to control apparent shininess of the surface.
  float specular_exponent;

  /// @brief Color of diffuse light in RGB format.
  mathfu::vec4 diffuse_color;

  /// @brief Color of ambient light in RGB format.
  mathfu::vec4 ambient_color;

  /// @brief Color of specular light in RGB format.
  mathfu::vec4 specular_color;
};

/// @brief Component that controls the position of a light source and
/// determines how affected entities are shaded according to diffuse,
/// ambient, and specular light.
class LightComponent : public corgi::Component<LightData> {
 public:
  LightComponent() {}
  virtual ~LightComponent() {}

  /// @brief Renders an on-screen box to modify in SceneLab UI.
  void RenderLightBox();

  /// @brief Deserialize raw data from a FlatBuffer to create a new entity.
  ///
  /// @param[in,out] entity An EntityRef reference that points to the entity
  /// that is being added from the raw data.
  /// @param[in] raw_data A void pointer to the raw FlatBuffer data.
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);

  /// @brief Serialize data from an entity into a flat buffer to export as raw
  /// data.
  ///
  /// @param[in,out] entity An EntityRef reference that points to the entity
  /// which is being exported as raw data.
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;
};

}  // zooshi
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::zooshi::LightComponent, fpl::zooshi::LightData)

#endif  // FPL_ZOOSHI_COMPONENTS_LIGHT_H