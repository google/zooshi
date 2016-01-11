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

#include "shaders/include/shadow_map.glslf_h"

varying vec2 vTexCoord;
uniform sampler2D texture_unit_0;
varying highp vec4 vPosition;

// We add in a slight bias, to cut down on shadow acne.
// (i. e. rounding errors making front surfaces still appear
// to be in shadow because the depths being compared are too close)
uniform float bias;

// This shader renders the geometry normally, except instead of coloring
// according to the texture, it generates a depth map.  (Note - we're assuming
// that the output texture is GL_UNSIGNED_BYTE, so we have 8 bits per color
// channel.  Other formats will result in more shadow errors.)

void main() {
  // Normalize the depth.
  highp float depth = vPosition.z / vPosition.w;
  // Convert from a range of [-1, 1] to [0, 1]
  depth = (depth + 1.0) / 2.0;

  float one_minus_bias = 1.0 - bias;
  depth = depth * one_minus_bias + bias;

  highp vec4 compacted_depth = EncodeFloatRGBA(depth);


  //this unfortunately throws out some info, but we have to, to render.
  // TODO - figure out how to render things with 0 alpha.
  compacted_depth.a = 1.0;

  gl_FragColor = compacted_depth;
}
