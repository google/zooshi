Building for OS X    {#zooshi_guide_building_osx}
=================

You can use [cmake][] to generate an [Xcode][] project for [Zooshi][] on
[OS X][].

# Version Requirements

[Zooshi][] is known to build with the following tool versions:

-   [cmake][]: 2.8.12
-   [cwebp][]: 0.4.3 (download from the [WebP Precompiled Utilities][])
    -   For example: [cwebp 0.4.3 direct download][]
-   [ImageMagick][]: 6.9.2-6
-   [OS X][]: Yosemite (10.10.1)
-   [Python][]: 2.7.10
-   [Xcode][]: 6.4

# Before Building    {#building_osx_prerequisites}

-   Install [Xcode][].
    -   Be sure to open up `Xcode` after installing to accept the license.
-   Install [cmake][].
    -   Add the directory containing [cmake][]'s `bin` folder to the `PATH`
        variable.
        -   For example, if [cmake][] is installed in
            `/Applications/CMake.app`, the following line should be added to
            the user's bash resource file `~/.bash_profile`:<br>
            `export PATH="$PATH:/Applications/CMake.app/Contents/bin"`
-   Download the `libwebp` archive for [OS X][] from
    [WebP Precompiled Utilities][] (e.g. [cwebp 0.4.3 direct download][]).
    -   Unpack [cwebp][] to a directory on your system.
    -   Add the `bin` directory containing [cwebp][] to the `PATH` variable.
        (Similarly to [cmake][]).
-   Install [ImageMagick][].
    -   To install ImageMagick, you will need [MacPorts][] or [Homebrew][]. We
        will use [Homebrew][], for example:
        -   Open a `terminal` and run `ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`.
    -   Then use `brew` (or `port`) to install ImageMagick and its dependencies.
        For [Homebrew][]:
        -   Open a `terminal` and run `brew install imagemagick --with-librsvg`.

# Creating the Xcode Project

The [Xcode][] project is generated using [cmake][].

For example, the following generates the [Xcode][] project in the `zooshi`
directory.

~~~{.sh}
    cd zooshi
    cmake -G "Xcode"
~~~


# Building with Xcode

-   Open the `zooshi/zooshi.xcodeproj` project using [Xcode][] from the command
    line:
~~~{.sh}
   cd zooshi
   open zooshi.xcodeproj
~~~
    It's important to open the project from the command line so the modified
    `PATH` variable is available in [Xcode][].  Failure to launch from the
    command line can result in build errors due `Xcode` being unable to find
    tools (like `cwebp`).  An alternative would be to either install tools in
    system bin directories (e.g /usr/bin) or modify the system wide `PATH`
    variable (using [environment.plist][]) to include paths to all required
    tools.
-   Select `Product-->Build` from the menu.

You can also build the game from the command-line.

-   Run `xcodebuild` after generating the Xcode project to build all targets.
    -   You may need to force the `generated_includes` target to be built first.

For example, in the `zooshi` directory:

~~~{.sh}
    xcodebuild -target generated_includes
    xcodebuild
~~~

# Executing the Game with Xcode

-   Select `Zooshi` as the `Scheme`, for example `zooshi-->My Mac`,
    from the combo box to the right of the `Run` button.
-   Click the `Run` button.

You can also run the game from the command-line.

For example:

    ./bin/Debug/zooshi

<br>

  [cmake]: http://www.cmake.org
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [cwebp 0.4.3 direct download]: http://downloads.webmproject.org/releases/webp/libwebp-0.4.3-mac-10.9.tar.gz
  [Homebrew]: http://brew.sh/
  [ImageMagick]: http://www.imagemagick.org
  [MacPorts]: https://www.macports.org/
  [OS X]: http://www.apple.com/osx/
  [Python]: http://www.python.org/download/releases/2.7/
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Xcode]: http://developer.apple.com/xcode/
  [Xquartz]: http://xquartz.macosforge.org
  [Zooshi]: @ref zooshi_guide_overview
  [environment.plist]: https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPRuntimeConfig/Articles/EnvironmentVars.html
