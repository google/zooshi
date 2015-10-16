#!/bin/bash -eux
# Copyright 2014 Google Inc. All Rights Reserved.
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

: ${ANDROID_SDK_HOME:=$(cd $(dirname $(which android))/../../sdk; pwd)}

# Get the version of an Android SDK package in directory ${1}.
package_version() {
  if [[ $# -eq 1 ]]; then
    grep Revision ${1}/source.properties | sed 's/.*=//;s/\(\.0\)*$//'
  fi
}

# Updates package ${1} if the latest revision doesn't match the version in
# the directory ${2}.
update_package() {
  line="$(android list sdk -a -u | awk "/$1/ {print \$0; exit 0}")"
  version="$(echo "${line}" | awk '{ print $NF }')"
  if [[ "${version}" != "$(package_version "${2}")" ]]; then
    echo 'y' | android update sdk -u -a -t "$(echo "${line}" | \
                                              awk -F- '{print $1}')"
  fi
}

main() {
  local dist_dir=
  while getopts "o:d:" options; do
     case ${options} in
       d ) dist_dir=${OPTARG};;
       h ) echo "Usage: $(basename $0) [-d distribution_directory]" >&2;
           exit 1;;
     esac
  done

  declare -r latest_build_tools="$(\
      ls -d ${ANDROID_SDK_HOME}/build-tools/* | \
      grep 'build-tools/[0-9]*\.[0-9]*\.[0-9]*' | sort -nr | head -n1)"

  # Make sure we have all the latest tools installed.
  android list sdk -a -u
  update_package 'Android SDK Tools,' "${ANDROID_SDK_HOME}/tools/"
  update_package 'Android SDK Platform-tools,' \
    "${ANDROID_SDK_HOME}/platform-tools/"
  update_package 'Android SDK Build-tools,' "${latest_build_tools}"

  # Install the required SDK.
  APK_NEEDED=$(android list sdk -a -u | grep "SDK Platform" | grep "API 21")
  update_package $APK_NEEDED "${ANDROID_SDK_HOME}/platforms/android-21"

  update_package 'Google APIs, Android API 21,' \
    "${ANDROID_SDK_HOME}/add-ons/addon-google_apis-google-21"

  # Make sure Google Play services is installed.
  android list sdk -a -u
  update_package 'Google Play services,' \
    "${ANDROID_SDK_HOME}/extras/google/google_play_services/"
  # Log the contents of the SDK.
  find ${ANDROID_SDK_HOME} -name google-play-services_lib

  # Change to zooshi directory
  cd "$(dirname "$(readlink -f $0)")/.."

  # Clean the asset folder and intermediate asset folder
  rm -rf obj/assets
  find assets | grep -v -E \
    '(^assets$|^assets/shaders|^assets/sounds|^assets/about\.txt|^assets/licenses\.txt)' | \
    xargs -I@ rm -rf "@"

  # Build release.
  INSTALL=0 LAUNCH=0 ./build_install_run.sh --verbose -A ''
  if [[ -n "${dist_dir}" ]]; then
    # Archive unsigned release build.
    cp ./bin/zooshi-release-unsigned.apk ${dist_dir}/Zooshi.apk
    # Archive release build signed with the test key.
    cp ./apks/zooshi.apk ${dist_dir}/Zooshi-Test.apk
  fi

  # Build and archive the debug build.
  rm -rf bin
  INSTALL=0 LAUNCH=0 ./build_install_run.sh -T debug -f 'NDK_DEBUG=1' \
    --verbose -A ''
  if [[ -n "${dist_dir}" ]]; then
    cp ./bin/zooshi-debug.apk ${dist_dir}/Zooshi-Debug.apk
  fi
}

main "$@"
