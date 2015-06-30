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
  // The basic idea here is to render interesting looking water, by additively
  // blending 3 copies of the same texture, that are scrolling at different
  // speeds, and have a slight sine-wave-based wobble.  Also, there is some
  // fake specular highlighting going on, in that whenever the green-value
  // of the summed color value is high enough, the whole thing just turns
  // white.
  // Most of these values are the results of eyeballing something that looks
  // good, rather than hard-and-fast math and ratios.

  const float flow_speed1 = 1.4;    // Speed of layer 1.
  const float flow_speed2 = 1.7;      // Speed of layer 2.
  const float flow_speed3 = 0.7;      // Speed of layer 3.
  const float scale = 0.01;           // Size of wobble displacement.
  const float time_scale = 5.0;       // Speed over time of wobble.
  const float coord_scale = 10.0;     // Wobble frequency.
  const float specular_cutoff = 1.70; // Minimum color total for specular
  const float specular_width = 0.05;  // 'width' of the specular gradient.

  vec2 wobble = sin(vTexCoord * coord_scale + time * time_scale) * scale;
  vec2 tc1 = vTexCoord * 0.6 + vec2(0, -time / flow_speed1) + wobble;
  vec2 tc2 = vTexCoord * 1.0 + vec2(0, -time / flow_speed2) + wobble;
  vec2 tc3 = vTexCoord * 1.2 + vec2(0, -time / flow_speed3) + wobble;

  lowp vec4 texture_color1 = texture2D(texture_unit_0, tc1);
  lowp vec4 texture_color2 = texture2D(texture_unit_0, tc2);
  lowp vec4 texture_color3 = texture2D(texture_unit_0, tc3);

  float specular = clamp(texture_color1.g + texture_color2.g + texture_color3.g,
                specular_cutoff, specular_cutoff + specular_width) -
                specular_cutoff;
  specular *= 20.0;   // Normalize by multiplying by 1.0/specular_width

  gl_FragColor = color * (texture_color1 + texture_color2 +
      texture_color3)/2.0f + vec4(specular, specular, specular, 1.0);
}
