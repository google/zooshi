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

uniform mediump mat4 shadow_mvp;
uniform lowp vec4 color;
uniform lowp float shadow_intensity;

// The texture of the thing we're rendering:
uniform sampler2D texture_unit_0;
// The shadow map texture:
uniform sampler2D texture_unit_7;
// The position of the coordinate, in light-space
varying vec4 vShadowPosition;
// The texture coordinates
varying mediump vec2 vTexCoord;



// Decodes a packed RGBA value into a float.  (See EncodeFloatRGBA() in
// render_depth.glslf for an explanation of this packing.)
float DecodeFloatFromRGBA(vec4 rgba) {
  return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

// Accepts a texture (assumed to be the shadowmap) and a location, and
// returns the depth of the shadow map at that location.  (Note that the
// depth has been encoded into an RGBA value, and needs to be decoded first)
float ReadShadowMap(sampler2D texture, vec2 location) {
  return DecodeFloatFromRGBA(texture2D(texture_unit_7, location));
}

void main()
{
  mediump vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  if (texture_color.a < 0.01)
    discard;

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

  gl_FragColor = texture_color * color;
}
