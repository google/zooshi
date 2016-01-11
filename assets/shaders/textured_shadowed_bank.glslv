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

#include "shaders/include/fog_effect.glslf_h"
#include "shaders/include/shadow_map.glslf_h"

attribute vec4 aPosition;
attribute vec2 aTexCoord;
attribute vec3 aNormal;
attribute vec4 aColor;
varying vec2 vTexCoord;
varying vec4 vColor;
varying vec4 vShadowPosition;

uniform mat4 model_view_projection;
uniform vec3 light_pos;    //in object space
uniform mat4 light_view_projection;
uniform mat4 model;
uniform mat4 view_projection;

// Variables used in lighting:
uniform vec4 ambient_material;
uniform vec4 diffuse_material;

void main()
{
  vShadowPosition = light_view_projection * model * aPosition;
  vec4 position = model_view_projection * aPosition;
  vTexCoord = aTexCoord;

  vDepth = position.z * position.w;
  gl_Position = position;

  vec3 light_vector = light_pos - aPosition.xyz;
  float diffuse = max(0.0, dot(normalize(aNormal), normalize(light_vector)));
  // Store the light-calculated color, so we can use all of it except the alpha.
  vec4 color = vec4(aColor.rgb, 1) * (diffuse_material * diffuse + ambient_material);
  // Pass through the vertex color alpha.
  vColor = vec4(color.rgb, aColor.a);
}
