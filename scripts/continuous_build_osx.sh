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

readonly cmake_path="../../../../prebuilts/cmake/darwin-x86_64/\
cmake-2.8.12.1-Darwin64-universal/CMake\ 2.8-12.app/Contents/bin/cmake"

# readlink works differently on osx than it does on linux. This function is a
# workaround
realpath() {
  [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

main() {
  local dist_dir=
  while getopts "d:h" options; do
     case ${options} in
       d ) dist_dir=${OPTARG};;
       h ) echo "Usage: $(basename $0) [-d distribution_directory]" >&2;
           exit 1;;
     esac
  done

  # Generate makefile and build the game.
  cd "$(dirname "$(realpath $0)")/.."
  "${cmake_path}" .
  make

  # Put everything into an archive and put that in the supplied dist_dir.
  tar -czvf zooshi-$(date +%Y%m%d).tar.gz bin/zooshi assets run.command
  if [[ -n "${dist_dir}" ]]; then
    cp zooshi-$(date +%Y%m%d).tar.gz ${dist_dir}
  fi
}

main "$@"

