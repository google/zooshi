Building for Android    {#zooshi_guide_building_android}
====================

# Version Requirements

[Zooshi][] is known to build with the following tool versions:

-   [Android Studio][]: 2.2.3
-   [Android SDK][]: Android 7.1.1 Nougat (API Level 25)
-   [Android NDK][]: 13.1.3345770
-   [cwebp][]: 0.4.0 (downloaded from the [WebP Precompiled Utilities][])
-   [Python][]: 2.7.6

# Before Building

-   Install prerequisites for the developer machine's operating system.
    -   [Linux prerequisites][]
    -   [OS X prerequisites][]
    -   [Windows prerequisites][]
-   Install [fplutil prerequisites][].
-   Install [Android Studio][].
-   Install the `NDK`, `Google Play services`, and `Android Support Repository`
    packages from the [SDK Manager][]. You can access the SDK Manager through
    Android Studio.
-   Configure Gradle by creating a `local.properties` file in your Zooshi
    directory that gives the locations of the Android SDK and NDK. It should
    look something like this:

    sdk.dir=/usr/local/google/home/username/Android/Sdk
    ndk.dir=/usr/local/google/home/username/Android/Sdk/ndk-bundle

-   Download the [Google Play Games C++ SDK][] and unpack into Zooshi's
    `dependencies/gpg-cpp-sdk` directory, or set environment variable `GPG_SDK`
    to the install directory.
    -   For example, if you've fetched Zooshi to `~/zooshi/`, unpack
        the downloaded `gpg-cpp-sdk.v2.0.zip` to
        `~/zooshi/dependencies/gpg-cpp-sdk`.
-   Download the [Firebase C++ SDK][] and unpack into Zooshi's
    `dependencies/firebase_cpp_sdk` directory, or set environment variable
    `FIREBASE_SDK` to the install directory.
-   Register your Android app with Firebase.
    - Create a new app on the [Firebase console](https://firebase.google.com/console/), and attach
      your Android app to it.
      - You can use "com.google.fpl.zooshi" as the Package Name
        while you're testing.
      - To [generate a SHA1](https://developers.google.com/android/guides/client-auth)
        run this command on Mac and Linux,
        ```
        keytool -exportcert -list -v -alias androiddebugkey -keystore ~/.android/debug.keystore
        ```
        or this command on Windows,
        ```
        keytool -exportcert -list -v -alias androiddebugkey -keystore %USERPROFILE%\.android\debug.keystore
        ```
      - If keytool reports that you do not have a debug.keystore, you can
        [create one with](http://developer.android.com/tools/publishing/app-signing.html#signing-manually),
        ```
        keytool -genkey -v -keystore ~/.android/debug.keystore -storepass android -alias androiddebugkey -keypass android -dname "CN=Android Debug,O=Android,C=US"
        ```
    - Add the `google-services.json` file that you downloaded from Firebase
      console to the Zooshi directory. This file identifies your
      Android app to the Firebase backend.
    - For further details please refer to the
      [general instructions for setting up an Android app with Firebase](https://firebase.google.com/docs/android/setup).

# Building

The [Zooshi][] project has a `build.gradle` file, which contains details about
how to build an Android package (apk).

To build for Android:

-   Open a command line window.
-   Go to the [Zooshi][] directory.
-   Run `gradlew` to build the project.

# Installing the Game

You can use the [Android Debug Bridge][] (adb) to install the game.
This tool is in the [Android SDK][] directory. Attach an Android phone to your
desktop with a USB cable, and ensure [remote debugging][] is enabled on your
phone, then run:

    adb install build/outputs/apk/zooshi-debug.apk


<br>

  [Android]: https://www.android.com/
  [Android Debug Bridge]: https://developer.android.com/studio/command-line/adb.html
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [Android Studio]: https://developer.android.com/studio/install.html
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
  [Firebase C++ SDK]: https://firebase.google.com/docs/cpp/setup
  [remote debugging]: https://developers.google.com/web/tools/chrome-devtools/remote-debugging/
  [Zooshi]: @ref zooshi_guide_overview
