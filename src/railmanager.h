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

#ifndef RAILMANAGER_H
#define RAILMANAGER_H

#include <unordered_map>
#include "components_generated.h"
#include "mathfu/glsl_mappings.h"
#include <memory>
#include "motive/math/compact_spline.h"
#include "rail_def_generated.h"

namespace fpl {
namespace fpl_project {

typedef const char* RailId;

class Rail {
 public:
  void Initialize(const RailDef* rail_def, float spline_granularity);

  /// Return vector of `positions` that is the rail evaluated every `delta_time`
  /// for the entire course of the rail. This calculation is much faster than
  /// calling PositionCalculatedSlowly() multiple times.
  void Positions(float delta_time,
                 std::vector<mathfu::vec3_packed>* positions) const;

  /// Return the rail position at `time`. This calculation is fairly slow
  /// so only use outside a loop. If you need a series of positions, consider
  /// calling Positions() above instead.
  mathfu::vec3 PositionCalculatedSlowly(float time) const;

  /// Length of the rail.
  float EndTime() const { return splines_[0].EndX(); }

  /// Internal structure representing the rails.
  const fpl::CompactSpline* splines() const { return splines_; }

 private:
  static const motive::MotiveDimension kDimensions = 3;

  fpl::CompactSpline splines_[kDimensions];
};

// Class for handling loading and storing of rails.
class RailManager {
 public:
  // Returns the data for the rail specified in the supplied filename.
  // If that data has already been loaded, it just returns the data directly.
  // Otherwise, it loads the data and caches it.
  Rail* GetRail(RailId rail_file);

  void Clear();

 private:
  std::unordered_map<RailId, std::unique_ptr<Rail>> rail_map;
};

}  // fpl_project
}  // fpl

#endif  // RAILMANAGER_H
