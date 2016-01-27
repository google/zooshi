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

// Textured Shadow Shader
// Renders geometry, taking into account a shadow map.
// Requires the model matrix, as well as the view/projection matrix for both
// the light source, and the camera.  (Also assumes that a shadowmap has already
// been generated and is being passed in part of texture_id_7.  7 was picked
// arbitrarily, because it doesn't conflict with any other channels, so we can
// just leave that texture binding for the whole rendering pass.)

#include "shaders/fplbase/phong_shading.glslf_h"
#include "shaders/include/shadow_map.glslf_h"
#include "shaders/include/fog_effect.glslf_h"

// Variables used in general rendering:
uniform lowp vec4 color;
// The texture of the thing we're rendering:
uniform sampler2D texture_unit_0;
// The texture coordinates
varying mediump vec2 vTexCoord;

// Variables used by shadow maps:
uniform mediump mat4 shadow_mvp;
// The position of the coordinate, in light-space
varying vec4 vShadowPosition;

// Variables used in lighting:
varying vec3 vPosition;
varying vec3 vNormal;
uniform vec3 light_pos;     // in object space
uniform vec3 camera_pos;    // in object space

void main()
{
  mediump vec4 texture_color = texture2D(texture_unit_0, vTexCoord);

  // Apply the shadow map:
  mediump vec2 shadowmap_coords = CalculateShadowMapCoords(vShadowPosition);

  highp float light_dist = vShadowPosition.z / vShadowPosition.w;
  light_dist = (light_dist + 1.0) / 2.0;
  vec3 light_direction = CalculateLightDirection(vPosition, light_pos);

  // Calculate Phong shading:
  lowp vec4 shading_tint = CalculatePhong(vPosition, vNormal, light_direction);

  // Calculate specular shading:
  shading_tint += CalculateSpecular(vPosition, vNormal, light_direction,
                                    camera_pos);

  // Apply the shading tint:
  texture_color *= shading_tint;

  // Apply shadows:
  texture_color = ApplyShadows(texture_color, shadowmap_coords, light_dist);

  // Apply the object tint:
  texture_color = texture_color * color;

  // Apply the fog:
  texture_color = ApplyFog(texture_color, vDepth, fog_roll_in_dist,
    fog_max_dist, fog_color,  fog_max_saturation);

  // Final result:
  gl_FragColor = texture_color;
}
