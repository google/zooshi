#!/bin/bash -eu
# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Build, deploy, debug / execute a native Android package based upon
# NativeActivity.

: ${FPLUTIL:=$([[ -e $(dirname $0)/dependencies/fplutil ]] && \
               ( cd $(dirname $0)/dependencies/fplutil; pwd ) ||
               ( cd $(dirname $0)/../../libs/fplutil; pwd ))}
# Enable / disable apk installation.
: ${INSTALL=1}
# Enable / disable apk launch.
: ${LAUNCH=1}

# logcat filter which displays SDL, Play Games messages and system errors.
declare -r logcat_filter="\
  --adb_logcat_args \
  \"-s\" \
  SDL:V \
  SDL/APP:V \
  SDL/ERROR:V \
  SDL/SYSTEM:V \
  SDL/AUDIO:V \
  SDL/VIDEO:V \
  SDL/RENDER:V \
  SDL/INPUT:V \
  GamesNativeSDK:V \
  SDL_android:V \
  ValidateServiceOp:E \
  AndroidRuntime:E \*:E"

main() {
  local -r this_dir=$(cd $(dirname $0) && pwd)
  local additional_args=
  # Optionally enable installation of the built package.
  if [[ $((INSTALL)) -eq 1 ]]; then
    additional_args="${additional_args} -i"
  fi
  # Optionally enable execution of the built package.
  if [[ $((LAUNCH)) -eq 1 ]]; then
    additional_args="${additional_args} -r"
    additional_args="${additional_args} --adb_logcat_monitor \
                      ${logcat_filter}"
  fi
  # Build and sign with the test key if it's found.
  local -r key_dir=$(cd ${this_dir}/../../libraries/certs/zooshi && pwd)
  if [[ "${key_dir}" != "" ]]; then
    ${FPLUTIL}/bin/build_all_android.py \
      --apk_keypk8 ${key_dir}/zooshi.pk8 \
      --apk_keypem ${key_dir}/zooshi.x509.pem \
      -E dependencies google-play-services_lib \
      -S "$@" ${additional_args}
  else
    echo "${key_dir} not found, skipping signing step." >&2
    ${FPLUTIL}/bin/build_all_android.py \
      -E dependencies google-play-services_lib -S "$@" ${additional_args}
  fi
}

main "$@"
