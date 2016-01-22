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
// been generated and is being passed in as part of texture_id_7)

attribute mediump vec4 aPosition;
varying mediump vec4 vShadowPosition;
attribute mediump vec2 aTexCoord;
varying mediump vec2 vTexCoord;
varying lowp float vDepth;
attribute vec3 aNormal;

uniform mediump mat4 model;
uniform mediump mat4 view_projection;
uniform mediump mat4 light_view_projection;

// Variables used in lighting:
varying vec3 vPosition;
varying vec3 vNormal;

void main()
{
  vTexCoord = aTexCoord;
  vNormal = aNormal;
  // vShadowPosition represents the position of the point, from the point of
  // view of the light-source.
  vShadowPosition = light_view_projection * model * aPosition;
  vec4 position = view_projection * model * aPosition;
  vPosition = position.xyz;
  vDepth = position.z * position.w;
  gl_Position = position;
}
