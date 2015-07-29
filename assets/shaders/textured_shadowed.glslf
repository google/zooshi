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

#include "shaders/shadow_map_use.glslfi"

// Variables used in genereal rendering:
uniform lowp vec4 color;
// The texture of the thing we're rendering:
uniform sampler2D texture_unit_0;
// The texture coordinates
varying mediump vec2 vTexCoord;

// Variables used by shadow maps:
uniform mediump mat4 shadow_mvp;
uniform lowp float shadow_intensity;
// The position of the coordinate, in light-space
varying vec4 vShadowPosition;

// Variables used by fog:
varying lowp float vDepth;
uniform float fog_roll_in_dist;
uniform float fog_max_dist;
uniform vec4 fog_color;
// Saturation represents how much the object becomes saturated by the fog
// color at fog_max_dist.  If saturation is 1.0, at max_fog_dist, the object
// is entirely fog_color-colored.  At 0.0, even at max_fog_dist, the object's
// color is unchanged.
uniform float fog_max_saturation;

void main()
{
  mediump vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  if (texture_color.a < 0.01)
    discard;

  // Apply the shadow map:
  mediump vec2 shadowmap_coords = vShadowPosition.xy / vShadowPosition.w;
  shadowmap_coords = (shadowmap_coords + vec2(1.0, 1.0)) / 2.0;

  highp float light_dist = vShadowPosition.z / vShadowPosition.w;
  light_dist = (light_dist + 1.0) / 2.0;

  // Read the shadowmap texture, and compare that value (which represents the
  // distance from the light-source, to the first object it hit in this
  // direction) to our actual distance from the light, in light-space.  If we
  // are further away from the light-source than the foreground object rendered
  // on the shadowmap, then we are in shadow.
  // If we're outside of the bounds of the shadowmap, then we have no
  // information about whether we're in shadow or not, so just skip the whole
  // step, and render as though we're unshadowed.
  float shadow_dimness = 1.0;
  if (shadowmap_coords.x > 0.0 && shadowmap_coords.x < 1.0 &&
      shadowmap_coords.y > 0.0 && shadowmap_coords.y < 1.0) {
    if (ReadShadowMap(texture_unit_7, shadowmap_coords.xy) < light_dist) {
      shadow_dimness = 1.0 - shadow_intensity;
    }
    texture_color = vec4(texture_color.xyz * shadow_dimness, texture_color.a);
  }

  // Apply the object tint:
  texture_color = texture_color * color;

  // Apply the fog:
  float fog_factor = (clamp(vDepth, fog_roll_in_dist, fog_max_dist) -
      fog_roll_in_dist) / (fog_max_dist - fog_roll_in_dist);

  texture_color = mix(texture_color, fog_color, fog_factor *
      fog_max_saturation);

  // Final result:
  gl_FragColor = texture_color;
}
