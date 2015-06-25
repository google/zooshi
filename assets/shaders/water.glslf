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
uniform float time;
void main()
{
  const float flow_speed1 = 2.0;    // Speed of water flowing down river.
  const float flow_speed2 = 1.7;    // Speed of water flowing down river.
  const float flow_speed3 = 0.7;    // Speed of water flowing down river.
  const float scale = 0.01;        // Size of wobble displacement.
  const float time_scale = 5.0;    // Speed over time of wobble.
  const float coord_scale = 10.0;  // Wobble frequency.

  vec2 wobble = sin(vTexCoord * coord_scale + time * time_scale) * scale;
  vec2 tc1 = vTexCoord * 0.6 + vec2(0, -time / flow_speed1) + wobble;
  vec2 tc2 = vTexCoord * 1.0 + vec2(0, -time / flow_speed2) + wobble;
  vec2 tc3 = vTexCoord * 1.2 + vec2(0, -time / flow_speed3) + wobble;

  lowp vec4 texture_color1 = texture2D(texture_unit_0, tc1);
  lowp vec4 texture_color2 = texture2D(texture_unit_0, tc2);
  lowp vec4 texture_color3 = texture2D(texture_unit_0, tc3);

  gl_FragColor = color * (texture_color1 + texture_color2 + texture_color3)/2.0f;
}
