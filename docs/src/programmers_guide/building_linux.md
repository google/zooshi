Building for Linux    {#zooshi_guide_building_linux}
==================

# Version Requirements

[Zooshi][] is known to build with the following tool versions:

-   [autoconf][]: 2.69
-   [automake][]: 1.141
-   [cmake][]: 2.8.12
-   [cwebp][]: 0.4.0 (available from the [WebP Precompiled Utilities][] page)
-   [GLU][]: libglu1-mesa-dev 8.0.4
-   [ImageMagick][]: 6.7.7-10
-   [libtool][]: 2.4.2
-   [OpenGL][]: libglapi-mesa 8.0.4
-   [OSS Proxy Daemon][]: osspd 1.3.2
-   [Python][]: 2.7.6
-   [Ragel][]: 6.9

# Before Building    {#building_linux_prerequisites}

Prior to building, install the following components using the [Linux][]
distribution's package manager:

-    [autoconf][], [automake][], and [libtool][]
-    [cmake][] (You can also manually install from [cmake.org][].)
-    [cwebp][]
-    [GLU][] (`libglu1-mesa-dev`)
-    [ImageMagick][]
-    [OpenGL][] (`libglapi-mesa`)
-    [OSS Proxy Daemon][] (`osspd`)
-    [Python][]
-    [Ragel][]

For example, on Ubuntu:

    sudo apt-get install autoconf automake cmake imagemagick libglapi-mesa libglu1-mesa-dev libtool osspd python ragel webp

# Building

-   Generate makefiles from the [cmake][] project in the `zooshi` directory.
-   Execute `make` to build the game.

For example:

    cd zooshi
    cmake -G'Unix Makefiles'
    make

To perform a debug build:

    cd zooshi
    cmake -G'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug
    make

Build targets can be configured using options exposed in
`zooshi/CMakeLists.txt` by using cmake's `-D` option.
Build configuration set using the `-D` option is sticky across subsequent
builds.

For example, if a build is performed using:

    cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
    make

to switch to a release build CMAKE_BUILD_TYPE must be explicitly specified:

    cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
    make

# Executing the Game

After building the project, you can execute the game from the command line.
For example:

    ./bin/zooshi

<br>

  [autoconf]: http://www.gnu.org/software/autoconf/
  [automake]: http://www.gnu.org/software/automake/
  [cmake]: http://www.cmake.org/
  [cmake.org]: http://www.cmake.org/
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [GLU]: http://www.mesa3d.org/
  [ImageMagick]: http://imagemagick.org
  [libtool]: http://www.gnu.org/software/libtool/
  [Linux]: http://en.wikipedia.org/wiki/Linux
  [OpenGL]: http://www.mesa3d.org/
  [OSS Proxy Daemon]: http://sourceforge.net/projects/osspd/
  [Python]: http://www.python.org/download/releases/2.7/
  [Ragel]: http://www.colm.net/open-source/ragel/
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Zooshi]: @ref zooshi_guide_overview
