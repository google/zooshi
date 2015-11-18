Modifying Assets {#zooshi_guide_assets}
================

### Overview

The game has many data driven components allowing designers the tweak the
game mechanics and content creators to add assets.

Game data is specified by [JSON][] files in `zooshi/src/rawassets` which are
converted to binary and read in-game using [FlatBuffers][].  Each [JSON][]
file in `zooshi/src/rawassets` is described by a [FlatBuffers schema][], as
shown in the table below. Each [FlatBuffers schema][] file contains
descriptions of each field (prefixed by `//`) in the associated [JSON][] files.

The following table maps each set of [JSON][] files in `src/rawassets` to its
corresponding [Flatbuffers][] schemas in:

| JSON File(s)        | Flatbuffers Schema                                     |
|---------------------|--------------------------------------------------------|
| `assets.json`       | `src/flatbufferschemas/assets.fbs`                     |
| `audio_config.json` | `dependencies/pindrop/schemas/audio_config.fbs`        |
| `buses.json`        | `dependencies/pindrop/schemas/buses.fbs`               |
| `config.json`       | `src/flatbufferschemas/config.fbs`                     |
| `entity_*.json`     | `src/flatbufferschemas/components.fbs`                 |
| `graphs.json`       | `src/flatbufferschemas/graphs.fbs`                     |
| `input_config.json` | `src/flatbufferschemas/input_config.fbs`               |
| `materials/*`       | `dependencies/fplbase/schemas/materials.fbs`           |
| `meshes/*`          | `dependencies/fplbase/schemas/mesh.fbs`                |
| `rails/*`           | `src/flatbufferschemas/rail_def.fbs`                   |
| `sound_banks/*`     | `dependencies/pindrop/schemas/sound_bank_def.fbs`      |
| `sounds/*`          | `dependencies/pindrop/schemas/sound_collection_def.fbs`|

*Note: One additional file, `src/rawassets/assets_meta.json` does not correspond
to a [FlatBuffers schema][]. It is parsed directly by `scripts/build_assets.py`*

Some additional assets in `zooshi/src/rawassets` that are used by the game
described in the following table.

| Asset            | Description                                             |
|------------------|---------------------------------------------------------|
| `anims/*.fbx`    | [FBX][] files used by [Motive][]'s animation pipeline.  |
| `fonts/*.ttf`    | [TTF][] files for the text fonts used in the game.      |
| `textures/*.png` | [PNG][] image files to be used as the game textures.    |

### Building

The game loads assets from the `assets` directory.  In order to convert data
into a format suitable for the game runtime, assets are built from a python
script which requires:

*   [Python][] to run the asset build script. [Windows][] users may need to
    install [Python][] and add the location of the python executable to the PATH
    variable.  For example in Windows 7:
    *   Right click on `My Computer`, select `Properties`.
    *   Select `Advanced system settings`.
    *   Click `Environment Variables`.
    *   Find and select `PATH`, click `Edit...`.
    *   Add `;%PATH_TO_PYTHONEXE%` where `%PATH_TO_PYTHONEXE%` is the location
        of the python executable.
*   `flatc` ([FlatBuffers compiler][]) to convert [JSON][] text files to binary
    files in the `zooshi/assets` directory.
*   [cwebp][] is required to convert `png` images to the [webp][] format.
    * Install [cwebp][] by downloading the libwebp archive for your operating
      system (see [WebP Precompiled Utilities][])
    * Unpack somewhere on your system and add the directory containing the
      [cwebp][] binary to the `PATH` variable.  See prerequisites for the
      your operating system configuration instructions:
        * [Linux prerequisites](@ref building_linux_prerequisites)
        * [OS X prerequisites](@ref building_osx_prerequisites)
        * [Windows prerequisites](@ref building_windows_prerequisites)

After modifying the data in the `zooshi/src/rawassets` directory, the assets
need to be rebuilt by running the following commands:

 - On Linux:

~~~{.sh}
    cd zooshi
    cmake -G'Unix Makefiles'
    make assets
~~~

 - On Mac, with [Xcode][]:

~~~{.sh}
   cd zooshi
   cmake -G'Xcode'
   xcodebuild -target assets
~~~

- On Windows, with [Visual Studio][]:

*Note: Use `Visual Studio 10` with `cmake` to build for [Visual Studio][] 2010.
Use `Visual Studio 11` for [Visual Studio][] 2012.*

~~~{.sh}
   cd zooshi
   cmake -G"Visual Studio 10"
   msbuild /t:assets zooshi.sln
~~~

<br>

Each file under `src/rawassets` will produce a corresponding output file in
the `assets` directory.  For example, after running the asset build,
`assets/config.zooconfig` will be generated from `src/rawassets/config.json`.

### Configuring Entity Prototypes

All of the Zooshi game entities are built from prototypes that are specified in
the `src/rawassets/entity_prototypes.json` file and are described by the
`src/flatbufferschemas/components.fbs` file.

The [CORGI][] [entity-component system][] library handles the creation and
management of all of the game entities. Specifically, the `EntityFactory` class,
in the CORGI [component library][], creates entities from these prototypes.

For instance, lets say you wanted to add a new patron to the game. For this
example, lets pretend it is a panda.

There is a `PatronDef` table in `components.fbs` that you can use as the base for
your new panda patron's prototype.

Here is a snippet taken from the `PatronDef` in `components.fbs`:

~~~{.c}
   table PatronDef {
     // The maximum distance the patron will move when trying to catch sushi.
     max_catch_distance:float = 5.0;

     // The time to take turning to face the raft, in seconds.
     time_to_face_raft:float = 0.2;

     // If true: when fed play eat, satisfied, disappear animations.
     // If false: when fed play satisfied, disappear animations.
     play_eating_animation:bool;
  }
~~~

You could then define your own patron in `entity_prototypes.json`:

~~~{.json}
  // Panda Patron
  {
    "component_list": [
      {
        "data_type: "MetaDef",
        "data" : {
          "entity_id": "PatronPanda"
        }
      },
      {
        "data_type": "PatronDef",
        "data": {
          "max_catch_distance": 10.0,
          "time_to_face_raft": 0.2,
          "play_eating_animation": true
        }
      }
    ]
  }
~~~

You'll notice that there are two component definitions for the entity in this
example: `MetaDef` and `PatronDef`. You can include any number of components
in your prototype in the `component_list` field.


<br>

  [component library]: http://google.github.io/corgi/component_library.html
  [CORGI]: http://google.github.io/corgi/
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [entity-component system]: https://en.wikipedia.org/wiki/Entity_component_system
  [FBX]: https://en.wikipedia.org/wiki/FBX
  [FlatBuffers]: http://google.github.io/flatbuffers/
  [FlatBuffers compiler]: http://google.github.io/flatbuffers/md__compiler.html
  [FlatBuffers schema]: http://google.github.io/flatbuffers/md__schemas.html
  [JSON]: http://json.org/
  [Motive]: http://google.github.io/motive/
  [TTF]: https://en.wikipedia.org/wiki/TrueType
  [PNG]: https://en.wikipedia.org/wiki/Portable_Network_Graphics
  [Python]: http://python.org/
  [Visual Studio]: http://www.visualstudio.com/
  [webp]: https://developers.google.com/speed/webp/
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Windows]: http://windows.microsoft.com/
  [Xcode]: http://developer.apple.com/xcode/
