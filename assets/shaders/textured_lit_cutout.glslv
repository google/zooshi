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

#include "shaders/fplbase/phong_shading.glslf_h"

attribute vec4 aPosition;
attribute vec2 aTexCoord;
attribute vec3 aNormal;
varying vec2 vTexCoord;

uniform mat4 model_view_projection;

// Variables used in lighting:
varying lowp vec4 vColor;
uniform vec3 light_pos;    //in object space
uniform lowp vec4 color;

void main()
{
  vec4 position = model_view_projection * aPosition;
  vTexCoord = aTexCoord;

  gl_Position = position;

  // Calculate Phong shading:
  lowp vec4 shading_tint = CalculatePhong(position.xyz, aNormal, light_pos);

  // Apply shading tint:
  vColor = color * shading_tint;
}
