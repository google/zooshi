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

#include "shaders/fplbase/skinning.glslv_h"

attribute mediump vec4 aPosition;
varying mediump vec4 vShadowPosition;
attribute mediump vec2 aTexCoord;
attribute vec3 aNormal;
varying mediump vec2 vTexCoord;
varying vec4 vColor;

uniform mediump mat4 model;
uniform mediump mat4 view_projection;
uniform mediump mat4 light_view_projection;
uniform mediump vec3 light_pos;    //in object space

// Variables used in lighting:
uniform vec4 ambient_material;
uniform vec4 diffuse_material;
uniform lowp vec4 color;

// Variables used by fog:
varying lowp float vDepth;

void main()
{
  vTexCoord = aTexCoord;
  vShadowPosition = light_view_projection * model * OneBoneSkinnedPosition(aPosition);
  vec4 position = view_projection * model * OneBoneSkinnedPosition(aPosition);

  vDepth = position.z * position.w;
  gl_Position = position;

  //float diffuse = max(0.0, dot(normalize(aNormal), normalize(light_pos)));
  vec3 light_vector = light_pos - aPosition.xyz;
  float diffuse = max(0.0, dot(normalize(aNormal), normalize(light_vector)));
  vColor = color * (diffuse_material * diffuse + ambient_material);
}
