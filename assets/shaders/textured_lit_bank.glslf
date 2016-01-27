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

// Special version of the textured_list shader used for the riverbank,
// which blends between two textures using vertex color alpha as the blend
// factor.

#include "shaders/fplbase/phong_shading.glslf_h"
#include "shaders/include/fog_effect.glslf_h"

varying mediump vec2 vTexCoord;
uniform sampler2D texture_unit_0;
uniform sampler2D texture_unit_1;

// Variables used in lighting:
varying lowp vec4 vColor;

void main()
{
  // Blend between the bank's two textures based on the vertex color's alpha.
  lowp vec4 texture_color =
    texture2D(texture_unit_0, vTexCoord) * (1.0 - vColor.a) +
    texture2D(texture_unit_1, vTexCoord) * vColor.a;

  // Apply the object tint:
  lowp vec4 final_color = vec4(vColor.rgb, 1) * texture_color;

  // Apply the fog:
  final_color = ApplyFog(final_color, vDepth, fog_roll_in_dist,
    fog_max_dist, fog_color,  fog_max_saturation);

  // Final result:
  gl_FragColor = final_color;
}
