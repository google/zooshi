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

// Variables used in lighting:
varying lowp vec4 vColor;

void main()
{
  lowp vec4 texture_color = texture2D(texture_unit_0, vTexCoord);
  if (texture_color.a < 0.1) discard;

  // Apply the object tint:
  lowp vec4 final_color = vColor * texture_color;

  // Final result:
  gl_FragColor = final_color;
}
