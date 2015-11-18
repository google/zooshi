Building for Android    {#zooshi_guide_building_android}
====================

# Version Requirements

[Zooshi][] is known to build with the following tool versions:

-   [Android NDK][]: android-ndk-r10e
-   [Android SDK][]: Android 5.0 (API Level 21)
-   [cwebp][]: 0.4.0 (downloaded from the [WebP Precompiled Utilities][])
-   [Python][]: 2.7.6

# Before Building

-   Install prerequisites for the developer machine's operating system.
    -   [Linux prerequisites][]
    -   [OS X prerequisites][]
    -   [Windows prerequisites][]
-   Install [fplutil prerequisites][].
-   Install the `Google Play services` package from the [SDK Manager][].
-   Download the [Google Play Games C++ SDK][] and unpack into Zooshi's
    `dependencies/gpg-cpp-sdk` directory.
    -   For example, if you've fetched Zooshi to `~/zooshi/`, unpack
        the downloaded `gpg-cpp-sdk.v2.0.zip` to
        `~/zooshi/dependencies/gpg-cpp-sdk`.

# Building

The [Zooshi][] project has an `AndroidManifest.xml` file, which contains
details about how to build an Android package (apk).

To build the project:

-   Open a command line window.
-   Go to the [Zooshi][] directory.
-   Run [build_all_android] to build the project.

For example:

    cd zooshi
    ./dependencies/fplutil/bin/build_all_android -E dependencies jni/libs

# Installing and Running the Game

Install the game using [build_all_android][].

For example, the following will install and run the game on a device attached
to the workstation with the serial number `ADA123123`.

    cd zooshi
    ./dependencies/fplutil/bin/build_all_android -E dependencies jni/libs -d ADA123123 -i -r -S

If only one device is attached to a workstation, the `-d` argument
(which selects a device) can be omitted.

<br>

  [Android]: https://www.android.com/
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [build_all_android]: http://google.github.io/fplutil/build_all_android.html
  [cmake]: https://cmake.org/
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [fplutil prerequisites]: http://google.github.io/fplutil/fplutil_prerequisites.html
  [Linux prerequisites]: @ref building_linux_prerequisites
  [OS X prerequisites]: @ref building_osx_prerequisites
  [Python]: https://www.python.org/download/releases/2.7/
  [SDK Manager]: https://developer.android.com/sdk/installing/adding-packages.html
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Windows prerequisites]: @ref building_windows_prerequisites
  [Google Play Games C++ SDK]: http://developers.google.com/games/services/downloads/
  [Zooshi]: @ref zooshi_guide_overview
