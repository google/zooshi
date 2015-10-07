Zooshi   {#zooshi_readme}
======

[Zooshi][] is a game in which players travel on a raft down an endless river
and toss sushi into the mouths of well-dressed animal patrons.

## Motivation

[Zooshi][] serves as a demonstration of how to build cross-platform
games using a suite of open source game technologies from
Fun Propulsion Labs at [Google][] such as [Breadboard][],
[Entity System][], [FlatBuffers][], [FlatUI][], [fplbase][], [fplutil][],
[Motive][], [WebP][], and [World Editor][].

[Zooshi][] also demonstrates how to use the [Google Cardboard][] API, which
is integrated into [fplbase][].

## Downloading

[Zooshi][] can be downloaded from:
   * [GitHub][] (source)
   * [GitHub Releases Page][] (source)
   * [Google Play][]
     (binary for Android)

**Important**: [Zooshi][] uses submodules to reference other components it
depends upon, so download the source from [GitHub][] using:

~~~{.sh}
    git clone --recursive https://github.com/google/zooshi.git
~~~

## Documentation

See our documentation for how to [Build and Run Zooshi][] and for a
[Programmer's Guide][] that details the overall structure of the game and all
of it's subsystems.

To contribute the this project see [CONTRIBUTING][].

For applications on Google Play that are derived from this application, usage
is tracked.
This tracking is done automatically using the embedded version string
(kVersion), and helps us continue to optimize it. Aside from
consuming a few extra bytes in your application binary, it shouldn't affect
your application at all. We use this information to let us know if Zooshi
is useful and if we should continue to invest in it. Since this is open
source, you are free to remove the version string but we would appreciate if
you would leave it in.

  [Android]: https://www.android.com/
  [Breadboard]: http://google.github.io/breadboard
  [Build and Run Zooshi]: http://google.github.io/zooshi/zooshi_guide_building.html
  [CONTRIBUTING]: http://github.com/google/zooshi/blob/master/CONTRIBUTING
  [Flatbuffers]: https://google.github.io/flatbuffers/
  [FlatUI]: http://google.github.io/flatui
  [fplbase]: http://google.github.io/fplbase
  [fplutil]: https://google.github.io/fplutil/
  [GitHub]: http://github.com/google/zooshi
  [GitHub Releases Page]: http://github.com/google/zooshi/releases
  [Google]: https://google.com
  [Google Cardboard]: https://www.google.com/get/cardboard/
  [Google Play]: https://play.google.com/store/apps/details?id=com.google.fpl.zooshi
  [Mathfu]: https://google.github.io/mathfu/
  [Motive]: http://google.github.io/motive/
  [Pie Noon]: http://google.github.io/pienoon/index.html
  [Programmer's Guide]: http://google.github.io/zooshi/zooshi_guide_overview.html
  [WebP]: https://developers.google.com/speed/webp/?hl=en
  [Zooshi]: http://google.github.io/zooshi/
