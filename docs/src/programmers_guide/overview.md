Overview    {#zooshi_guide_overview}
========

## Downloading

[Zooshi][] can be downloaded from [GitHub][] or the [GitHub Releases][] page.

~~~{.sh}
    git clone --recursive https://github.com/google/zooshi.git
~~~

## Subsystems

[Zooshi][] utilizes the following subsystems:

   * [Breadboard][] for handling events.
   * [CORGI][] for an [entity-component system][].
   * [FlatBuffers][] for serialization.
   * [FlatUI][] for GUI menus.
   * [fplbase][] for rendering and asset management.
   * [fplutil][] to build, run, profile, archive, and deploy the game.
   * [Motive][] for animation.
   * [Pindrop][] for audio.
   * [Scene Lab][] as a world-editor for in-game object configuration.
   * [WebP][] for image compression.

## Source Layout

The following table describes the directory structure of the game:


| Path                                    | Description                                     |
|-----------------------------------------|-------------------------------------------------|
| `zooshi` base directory                 | Project build files and run script.             |
| `assets`                                | Assets loaded by the game.                      |
| `jni`                                   | Top-level Android NDK makefile.                 |
| `jni/libs/gpg`                          | Android NDK makefile for Play Games Services.   |
| `jni/libs/src`                          | Android NDK makefile for the game.              |
| `res`                                   | Android specific resource files.                |
| `scripts`                               | Asset build makefiles.                          |
| `src`                                   | Game's source.                                  |
| `src/components`                        | Source for the [CORGI][] game Components.       |
| `src/flatbufferschemas`                 | Game schemas for [FlatBuffers][] data.          |
| `src/inputcontrollers`                  | Source for handling game input.                 |
| `src/modules`                           | Source for [Breadboard][] game modules.         |
| `src/rawassets`                         | JSON [FlatBuffers][] used as game data.         |
| `src/states`                            | Source for the game's state machine.            |
| `src_java/com/google/fpl/zooshi/Zooshi` | fplbase derived Android activity (entry point). |


<br>

   [Breadboard]: http://github.com/google/breadboard
   [CORGI]: http://github.com/google/corgi
   [entity-component system]: https://en.wikipedia.org/wiki/Entity_component_system
   [FlatBuffers]: http://github.com/google/flatbuffers
   [FlatUI]: http://github.com/google/flatui
   [fplbase]: http://github.com/google/fplbase
   [fplutil]: http://github.com/google/fplutil
   [GitHub]: http://github.com/google/zooshi
   [GitHub Releases]: http://github.com/google/zooshi/releases
   [Motive]: http://github.com/google/motive
   [Pindrop]: http://github.com/google/pindrop
   [Scene Lab]: http://github.com/google/scene_lab
   [WebP]: https://developers.google.com/speed/webp/
   [Zooshi]: @ref zooshi_index
