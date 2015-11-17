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

varying mediump vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform lowp vec4 color;
uniform highp float river_offset;
uniform highp float texture_repeats;
void main() {
  highp float tex_v = vTexCoord.y;

  // This is basically equivalent to tex_v = fract(tex_v * texture_repeats),
  // except that it makes us less likely to get math errors on devices with
  // low floating point percision.
  tex_v = fract(fract(((tex_v / 16.0) * (texture_repeats / 16.0))) * 256.0);

  highp vec2 tc = vec2(vTexCoord.x, tex_v + fract(-river_offset));

  lowp vec4 texture_color = texture2D(texture_unit_0, tc);

  gl_FragColor = color * texture_color;
}
