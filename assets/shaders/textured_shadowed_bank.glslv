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
varying vec2 vTexCoord;
varying vec4 vShadowPosition;
varying lowp vec4 vColor;

uniform mat4 model_view_projection;
uniform mat4 light_view_projection;
uniform mat4 model;
uniform mat4 view_projection;

// Variables used in lighting:
varying vec3 vPosition;
varying vec3 vNormal;
uniform vec4 color;

void main()
{
  vShadowPosition = light_view_projection * model * aPosition;
  vec4 position = model_view_projection * aPosition;
  vTexCoord = aTexCoord;
  vNormal = aNormal;

  vDepth = position.z * position.w;
  gl_Position = position;
  vPosition = position.xyz;
  vColor = color;
}
