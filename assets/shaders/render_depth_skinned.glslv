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

// This shader renders the geometry normally, except instead of coloring
// according to the texture, it generates a depth map.
// TODO(kelseymayfield): Edit above comment to be more specific

#define SKINNED

#include "shaders/fplbase/skinning.glslv_h"

attribute vec4 aPosition;
varying mediump vec4 vPosition;
uniform mat4 model_view_projection;
attribute vec2 aTexCoord;
varying vec2 vTexCoord;

void main()
{
  vTexCoord = aTexCoord;
  vPosition = model_view_projection * OneBoneSkinnedPosition(aPosition);
  gl_Position = vPosition;
}
