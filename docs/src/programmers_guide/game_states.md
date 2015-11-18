Game States {#zooshi_guide_gamestates}
===================

# Introduction {#gamestates_introduction}

This document outlines the state machine and the states used to control the
flow of [Zooshi][], which are located in the `states` directory.

# State Machine {#gamestates_statemachine}

Zooshi uses a state machine to control the flow between its various states,
including the menus, the game itself, and the in-game editor. Each state is in
control of the game when active, implementing an `AdvanceFrame` call to
advance the state every frame, and a `Render` call in order to render the
state to screen. In order to change states, changes are made to the
`next_state` variable, passed in during `AdvanceFrame`. Shared functions needed
by the states, such as rendering the world, and updating the camera, are
provided in `states_common.cpp`.

## Loading State {#gamestates_loading}

The initial state used while the game assets are being loaded. Note that the
assets are not queued by the state itself, but instead in `game.cpp`,
`InitializeAssets`. After the loading is complete, which is handled by
[fplbase][]'s **AssetManager**, the state transitions into the game menu state.

## Game Menu State {#gamestates_game_menu}

This state is used to manage the menus between games, powered by [FlatUI][],
which can be seen in `gui.cpp`. The menu manages the type of menu being
displayed, and based on the input from FlatUI, transitions between them, the
game, and exiting. This state also handles integration with
[Google Play Game Services][].

## Intro State {#gamestates_intro}

This state is used when entering [Cardboard gameplay][]. By introducing a state
between the menu and gameplay for Cardboard, it allows the user to place the
device in Cardboard before the start of the game. It also allows the
[Cardboard SDK][] to be oriented based on the direction the player is facing
when they leave the state, guaranteeing they will be facing in the correct
direction on game start.

## Gameplay State {#gamestates_gameplay}

This state is used to control the [gameplay][], updating and rendering the world
appropriately, for all types of [input controllers][], including Cardboard. The
state checks for the various state transitions that can occur from it, to the
pause state, game over, or the in-game editor.

## Pause State {#gamestates_pause}

This state is used for pausing from the gameplay state, with the menus again
using FlatUI, to either transition back into the game, or to the menu state.

## Gameover State {#gamestates_gameover}

This state is used when the game has finished, from not feeding enough patrons
in the previous lap. A brief cutscene plays, using the `EndGameEvent` on
patrons, after which it can return to the menu state. Note that when playing
in Cardboard, it instead restarts the gameplay state, as the menu does not
support being viewed while in Cardboard.

## Scene Lab State {#gamestates_scene_lab}

This state handles the in-game editor, which is powered by [Scene Lab][], and
can be entered by pressing **F10** from the gameplay state. This state updates
the Scene Lab, and upon exit returns back to the gameplay state, preserving
the changes that where made to the game.

<br>

  [Zooshi]: @ref zooshi_index
  [fplbase]: http://github.com/google/fplbase
  [FlatUI]: http://github.com/google/flatui
  [Google Play Game Services]: http://developer.android.com/google/play-services/games.html
  [Cardboard gameplay]: @ref zooshi_guide_gameplay_cardboard
  [Cardboard SDK]: https://developers.google.com/cardboard/android/
  [gameplay]: @ref zooshi_guide_gameplay
  [input controllers]: @ref zooshi_guide_controllers
  [Scene Lab]: http://github.com/google/scene_lab

