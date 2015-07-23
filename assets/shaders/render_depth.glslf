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

varying vec2 vTexCoord;
uniform sampler2D texture_unit_0;
varying highp vec4 vPosition;

// This shader renders the geometry normally, except instead of coloring
// according to the texture, it generates a depth map.  (Note - we're assuming
// that the output texture is GL_UNSIGNED_BYTE, so we have 8 bits per color
// channel.  Other formats will result in more shadow errors.)

// Problem:  We want the outputted depth value to be as precise as possible.
// Unfortunately, GLES just gives us 4 channels (RGBA), each of which is
// only 8 bits of precision.  (Probably)
// Solution:  Encode a float into 4 separate 8-bit floats!
// We make a few assumptions here.  The big one is that the number we are
// encoding is also between 0 and 1.  Basically, we divide our number by various
// powers of 2^8 - 1, and trim them down to 8 bits of precision via some clever
// application of swizzling.
// The most significant bit in this case is stored in the R channel, and the
// least is stored in the A channel.
highp vec4 EncodeFloatRGBA(float v) {
  highp vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
  return enc;
}

void main() {
  // We only record the depth if it's mostly opaque:
  lowp vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  if (texture_color.a < 0.9)
    discard;

  // Normalize the depth.
  highp float depth = vPosition.z / vPosition.w;
  // Convert from a range of [-1, 1] to [0, 1]
  depth = (depth + 1.0) / 2.0;

  // We add in a slight bias, to cut down on shadow acne.
  // (i. e. rounding errors making front surfaces still appear
  // to be in shadow because the depths being compared are too close)
  const float bias = 0.007;
  const float one_minus_bias = 1.0 - bias;

  depth = depth * one_minus_bias + bias;

  highp vec4 compacted_depth = EncodeFloatRGBA(depth);


  //this unfortunately throws out some info, but we have to, to render.
  // TODO - figure out how to render things with 0 alpha.
  compacted_depth.a = 1.0;

  gl_FragColor = compacted_depth;
}

