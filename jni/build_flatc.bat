@rem Copyright 2013 Google Inc. All rights reserved.
@rem
@rem Licensed under the Apache License, Version 2.0 (the "License");
@rem you may not use this file except in compliance with the License.
@rem You may obtain a copy of the License at
@rem
@rem     http://www.apache.org/licenses/LICENSE-2.0
@rem
@rem Unless required by applicable law or agreed to in writing, software
@rem distributed under the License is distributed on an "AS IS" BASIS,
@rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@rem See the License for the specific language governing permissions and
@rem limitations under the License.
@echo off

setlocal enabledelayedexpansion

rem Path to cmake passed in by caller.
set cmake=%1

rem Newest and oldest version of Visual Studio that it's possible to select.
set visual_studio_version_max=13
set visual_studio_version_min=8

rem Determine the newest version of Visual Studio installed on this machine.
set visual_studio_version=
for /L %%a in (%visual_studio_version_max%,-1,%visual_studio_version_min%) do (
  echo Searching for Visual Studio %%a >&2
  reg query HKLM\SOFTWARE\Microsoft\VisualStudio\%%a.0 /ve 1>NUL 2>NUL
  if !ERRORLEVEL! EQU 0 (
    set visual_studio_version=%%a
    goto found_vs
  )
)
echo Unable to determine whether Visual Studio is installed. >&2
exit /B 1
:found_vs

rem Map Visual Studio version to cmake generator name.
if "%visual_studio_version%"=="8" (
  set cmake_generator=Visual Studio 8 2005
)
if "%visual_studio_version%"=="9" (
  set cmake_generator=Visual Studio 9 2008
)
if %visual_studio_version% GEQ 10 (
  set cmake_generator=Visual Studio %visual_studio_version%
)

rem Generate Visual Studio solution.
echo Generating solution for %cmake_generator%. >&2
%cmake% -G"%cmake_generator%" -Dzooshi_only_flatc=ON
if %ERRORLEVEL% NEQ 0 (
    exit /B %ERRORLEVEL%
)

rem Build flatc
python jni\msbuild.py obj\flatbuffers\flatc.vcxproj
if ERRORLEVEL 1 exit /B 1


