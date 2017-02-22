Zooshi
======

Zooshi    {#zooshi_index}
======

[Zooshi][] is a game in which players travel on a raft down an endless river
and toss sushi into the mouths of well-dressed animal patrons. It is written
in cross-platform C++.

\htmlonly
<iframe width="560" height="315"
    src="https://www.youtube.com/embed/3B5KCE3AC6Y"
    frameborder="0" allowfullscreen>
</iframe>
\endhtmlonly

## Motivation

[Zooshi][] demonstrates a quick and fun game that primarily targets
[Google Cardboard][] using native C++ APIs. The game shows how to use
several open source technology components developed at Google.
<table style="border: 0; padding: 3em">
<tr>
  <td>
    <img src="info_panel_breadboard.png" style="height: 20em"/>
    [Breadboard][], an event system that allows game components to communicate
    with each other, without being tightly coupled.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_corgi.png" style="height: 20em"/>
    [CORGI][], a simple, yet flexible, [entity-component system][] that
    decouples the systems within the game.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_flatbuffers.png" style="height: 20em"/>
    [FlatBuffers][], a fast serialization system is used to store the game's
    data. The game configuration data is stored in JSON files which are
    converted to FlatBuffer binary files using the flatc compiler.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_flatui.png" style="height: 20em"/>
    [FlatUI][], an immediate-mode GUI designed specifically with games in mind.
    The game uses FlatUI to generate all of the menus in the game.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_fplbase.png" style="height: 20em"/>
    [fplbase][], is as a thin renderer and asset management library used by the
    game.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_google_play_games.png" style="height: 20em"/>
    [Google Play Games Services][] is used to share scores and reward players
    with achievements.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_mathfu_motive.png" style="height: 20em"/>
    [MathFu][], a geometry math library optimized for ARM and x86 processors.
    The game uses MathFu data types for two and three dimensional vectors, and
    for the 4x4 matrices used by the [fplbase][] rendering system, and also by
    the [Motive][] animation system.

    [Motive][], a memory efficient and performant animation library. The game
    uses Motive for all of the animation.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_pindrop.png" style="height: 20em"/>
    [Pindrop][], a simple audio engine designed with games in mind. It handles
    all the audio of the game.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_scene_lab.png" style="height: 20em"/>
    [Scene Lab][], an in-game world editor. This allows you to freeze the
    gameplay of the game and enter into an `edit` mode that allows you to move
    in-game objects, change object properties, etc.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_webp.png" style="height: 20em"/>
    [WebP][], an image compression technology, is used to compress textures
    which reduces the size of the final game package and ultimately reduces
    download time.
  </td>
</tr>
</table>

In addition, [fplutil][] is used to build, deploy, and run the game,
build and archive the game, and profile the game's CPU performance.

## Functionality

[Zooshi][] is a cross-platform, open-source game that supports:

   * Bluetooth controllers
   * Touch controls
   * Google Play Games Services sign-in and leaderboards
   * Android devices
      - Phones and tablets
      - Virtual reality via a [Google Cardboard][] device

## Supported Platforms

[Zooshi][] has been tested on the following platforms:

   * [Nexus Player][], an [Android TV][] device
   * [Android][] phones and tablets
   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

We use [SDL][] as our cross platform layer.
The game is written entirely in C++, with the exception of one Java file used
only on Android builds. The game can be compiled using [Linux][], [OS X][] or
[Windows][].

## Download

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

## Feedback and Reporting Bugs

   * Discuss Zooshi with other developers and users on the
     [Zooshi Google Group][].
   * File issues on the [Zooshi Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of
     **zooshi**.

<br>

  [Android]: http://www.android.com
  [Android TV]: http://www.android.com/tv/
  [Breadboard]: https://google.github.io/breadboard/
  [CORGI]: https://google.github.io/corgi/
  [entity-component system]: https://en.wikipedia.org/wiki/Entity_component_system
  [Flatbuffers]: https://google.github.io/flatbuffers/
  [FlatUI]: https://google.github.io/flatui/
  [fplbase]: https://google.github.io/fplbase/
  [fplutil]: https://google.github.io/fplutil/
  [GitHub]: http://github.com/google/zooshi
  [GitHub Releases Page]: http://github.com/google/zooshi/releases
  [Google Cardboard]: https://www.google.com/get/cardboard/
  [Google Play]: https://play.google.com/store/apps/details?id=com.google.fpl.zooshi
  [Google Play Games Services]: https://developer.android.com/google/play-services/games.html
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [Mathfu]: https://google.github.io/mathfu/
  [Motive]: https://google.github.io/motive/
  [Nexus Player]: http://www.google.com/nexus/player/
  [OS X]: http://www.apple.com/osx/
  [Pindrop]: http://google.github.io/pindrop/
  [Scene Lab]: https://google.github.io/scene_lab/
  [SDL]: https://www.libsdl.org/
  [stackoverflow.com]: http://stackoverflow.com/search?q=zooshi
  [WebP]: https://developers.google.com/speed/webp/?hl=en
  [Windows]: http://windows.microsoft.com
  [Zooshi]: @ref zooshi_index
  [Zooshi Google Group]: http://groups.google.com/group/zooshi
  [Zooshi Issues Tracker]: http://github.com/google/zooshi/issues
