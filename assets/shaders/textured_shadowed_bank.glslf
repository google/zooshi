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

// Special version of the textured_list shader used for the riverbank,
// which blends between two textures using vertex color alpha as the blend
// factor.

#include "shaders/fplbase/phong_shading.glslf_h"
#include "shaders/include/fog_effect.glslf_h"
#include "shaders/include/shadow_map.glslf_h"

varying mediump vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform sampler2D texture_unit_1;
varying lowp vec4 vColor;

uniform mediump mat4 shadow_mvp;
varying vec4 vShadowPosition;

// Variables used in lighting:
varying vec3 vPosition;
varying vec3 vNormal;
uniform vec3 light_pos;     // in object space
uniform vec3 camera_pos;    // in object space

void main()
{
  // Blend between the bank's two textures based on the vertex color's alpha.
  mediump vec4 texture_color =
    texture2D(texture_unit_0, vTexCoord) * (1.0 - vColor.a) +
    texture2D(texture_unit_1, vTexCoord) * vColor.a;

  // Apply the shadow map:
  mediump vec2 shadowmap_coords = CalculateShadowMapCoords(vShadowPosition);

  highp float light_dist = vShadowPosition.z / vShadowPosition.w;
  light_dist = (light_dist + 1.0) / 2.0;

  vec3 light_direction = CalculateLightDirection(vPosition, light_pos);

  // Apply shadows:
  texture_color = ApplyShadows(texture_color, shadowmap_coords, light_dist);

  // Apply the object tint:
  lowp vec4 final_color = vec4(vColor.rgb, 1) * texture_color;

  // Calculate Phong shading:
  lowp vec4 shading_tint = CalculatePhong(vPosition, vNormal, light_direction);

  // Calculate specular shading:
  shading_tint += CalculateSpecular(vPosition, vNormal, light_direction,
                                    camera_pos);

  // Apply the shading tint:
  final_color *= shading_tint;

  // Apply the fog:
  final_color = ApplyFog(final_color, vDepth, fog_roll_in_dist,
    fog_max_dist, fog_color,  fog_max_saturation);

  // Final result:
  gl_FragColor = final_color;
}
