Zooshi Input Controllers {#zooshi_guide_controllers}
===================

# Introduction {#controllers_introduction}

This document outlines the various controllers used to handle player input in
[Zooshi][].

# Overview {#controllers_overview}

Zooshi uses the concept of controllers, defined in the inputcontrollers folder,
to handle the various control schemes (mouse, device orientation, touchscreen,
gamepad, etc) that can be used to play the game. Primarily, they are in charge
of converting the input from the system into a facing vector, and requesting
sushi be thrown. The input is obtained from [fplbase][]'s **InputSystem**.

# Controller Types {#controllers_type}

There are several different types of controllers, each responsible for handling
a different sort of input.

## Android Cardboard Controller {#controllers_type_android}

Controller type that uses the [Cardboard SDK][] to drive the game. This
controller is used for both regular Android gameplay and Cardboard mode, using
the orientation of the device to control the camera.

## Gamepad Controller {#controllers_type_gamepad}

Controller type that uses an Android gamepad to drive the game, using the
**Gamepad** abstraction provided by fplbase.

## Mouse Controller {#controllers_type_mouse}

Controller type that uses a mouse to drive the game, which is used when playing
on desktop.

## Onscreen Controller {#controllers_type_onscreen}

Controller type that uses the touchscreen of Android, creating a joystick
based on touch, to drive the game. This control scheme is used by Android
devices that are not supported by the Cardboard SDK, and can also be accessed
through the Options menu in game.

<br>

  [Zooshi]: @ref zooshi_index
  [Cardboard SDK]: https://developers.google.com/cardboard/android/
  [fplbase]: https://google.github.io/fplbase/
