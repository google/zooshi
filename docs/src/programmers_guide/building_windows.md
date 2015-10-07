Building for Windows    {#zooshi_guide_building_windows}
====================

You can use [cmake][] to generate a [Visual Studio][] project for [Zooshi][]
on [Windows][].

# Version Requirements

[Zooshi][] is known to build with the following tool versions:

-   [cmake][]: 2.8.12
-   [cwebp][]: 0.4.0 (downloaded from the [WebP Precompiled Utilities][] page)
-   [DirectX SDK][]: 9.29.1962
-   [Python][]: 2.7.6
-   [Visual Studio][]: 2010 or 2012
-   [Windows][]: 7

# Before Building    {#building_windows_prerequisites}

Use [cmake][] to generate the [Visual Studio][] solution and project files.

The following example generates the [Visual Studio][] 2012 solution in the
`zooshi` directory:

    cd zooshi
    cmake -G "Visual Studio 11"

To generate a [Visual Studio][] 2010 solution, use this command:

    cd zooshi
    cmake -G "Visual Studio 10"

Running [cmake][] under [cygwin][] requires empty `TMP`, `TEMP`, `tmp` and
`temp` variables. To generate a [Visual Studio][] solution from a [cygwin][]
bash shell use:

    $ cd zooshi
    $ ( unset {temp,tmp,TEMP,TMP} ; cmake -G "Visual Studio 11" )


# Building with Visual Studio

-   Double-click on `zooshi/zooshi.sln` to open the solution.
-   Select `Build-->Build Solution` from the menu.

It's also possible to build from the command line using `msbuild` after using
`vsvars32.bat` to set up the [Visual Studio][] build environment. For example,
assuming [Visual Studio][] is installed in
`c:\Program Files (x86)\Microsoft Visual Studio 11.0`:

    cd zooshi
    "c:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\vsvars32.bat"
    cmake -G "Visual Studio 11"
    msbuild zooshi.sln

# Executing the Game with Visual Studio

-   Right-click on the `zooshi` project in the Solution Explorer
    pane, and select `Set as Startup Project`.
-   Select `Debug-->Start Debugging` from the menu.

<br>

  [cmake]: http://www.cmake.org
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [cygwin]: https://www.cygwin.com/
  [DirectX SDK]: http://www.microsoft.com/en-us/download/details.aspx?id=6812
  [Python]: http://www.python.org/download/releases/2.7/
  [Visual Studio]: http://www.visualstudio.com/
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Windows]: http://windows.microsoft.com/
  [Zooshi]: @ref zooshi_guide_overview
