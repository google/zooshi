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

#include "shaders/fplbase/phong_shading.glslf_h"
#include "shaders/fplbase/skinning.glslv_h"

attribute vec4 aPosition;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;

uniform mediump mat4 model;
uniform mediump mat4 view_projection;
uniform mediump mat4 model_view_projection;

#ifdef FOG_EFFECT
// Variables used by fog:
varying lowp float vDepth;
#endif  // FOG_EFFECT

#ifdef PHONG_SHADING
// Variables used in lighting:
attribute vec3 aNormal;
varying vec3 vNormal;
varying vec3 vPosition;
#endif  // PHONG_SHADING

#ifdef SHADOW_EFFECT
uniform mediump mat4 light_view_projection;

varying mediump vec4 vShadowPosition;
#ifndef PHONG_SHADING
varying vec3 vPosition;
#endif  // PHONG_SHADING
#endif  // SHADOW_EFFECT

#ifdef NORMALS
#ifndef PHONG_SHADING
attribute vec3 aNormal;
varying vec3 vNormal;
#endif  // PHONG_SHADING
attribute vec3 aTangent;
varying vec3 vTangent;
varying vec3 vObjectSpacePosition;
varying vec3 vTangentSpaceLightVector;
varying vec3 vTangentSpaceCameraVector
uniform vec3 light_pos;   // in object space
uniform vec3 camera_pos;  // in object space
#endif  // NORMALS

#ifdef BANK
uniform lowp vec4 color;
varying lowp vec4 vColor;
#endif  // BANK

void main()
{
  #ifndef SKINNED
  vec4 model_position = aPosition;
  #else
  vec4 model_position = OneBoneSkinnedPosition(aPosition);
  #endif  // SKINNED

  vec4 position = model_view_projection * model_position;

  vTexCoord = aTexCoord;

  #ifdef FOG_EFFECT
  vDepth = position.z * position.w;
  #endif  // FOG_EFFECT

  #ifdef SHADOW_EFFECT
  vShadowPosition = light_view_projection * model * model_position;

  vPosition = position.xyz;
  #endif  // SHADOW_EFFECT

  #ifdef PHONG_SHADING
  vNormal = aNormal;
  vPosition = position.xyz;
  #endif  // PHONG_SHADING

  #ifdef NORMALS
  #ifndef PHONG_SHADING
  vNormal = aNormal;
  #endif  // PHONG_SHADING
  vTangent = aTangent;
  vObjectSpacePosition = aPosition.xyz;

  vec3 n = normalize(vNormal);
  vec3 t = normalize(vTangent);
  vec3 b = normalize(cross(n, t)) * aTangent.w;

  mat3 world_to_tangent_matrix = mat3(t, b, n);

  vec3 camera_vector = camera_pos - vObjectSpacePosition;
  vec3 light_vector = light_pos - vObjectSpacePosition;

  vTangentSpaceLightVector = world_to_tangent_matrix * light_vector;
  vTangentSpaceCameraVector = world_to_tangent_matrix * camera_vector;
  #endif  // NORMALS

  #ifdef BANK
  vColor = color;
  #endif  // BANK

  gl_Position = position;
}
