Cardboard {#zooshi_guide_cardboard}
===================

#  Introduction {#cardboard_introduction}

This document outlines how the [Cardboard SDK for Android][] is used in
[Zooshi][], through the wrapper provided by [fplbase][].  For additional tips
on designing for Cardboard, be sure to check out
[Designing for Google Cardboard][].

# Overview {#cardboard_overview}

The Cardboard SDK provides various interfaces and classes to handle the
information that is needed to work with a Cardboard device, like head tracking
and detecting the magnet switch input.  To simplify the interaction with the
SDK, fplbase provides various helper functions and an abstraction layer for
managing the transformations, which Zooshi takes advantage of.

# Input {#cardboard_input}

Input from the Cardboard SDK, which includes the orientation of the device, as
well as both detecting the magnet switch or screen touches, is used in the
**AndroidCardboardController** (android_cardboard_controller.h/.cpp).  Note
that this controller is used not only in Cardboard mode, but also regular play
on Android devices that support the Cardboard SDK.

When updating, the controller uses the **HeadMountedDisplayInput** class,
provided by fplbase's **InputSystem**, which contains information about device
orientation, and if the Cardboard trigger was detected in the last frame.

Zooshi uses a coordinate system where Z is treated as up, with Y being
forward, while the Cardboard SDK and fplbase's wrapper class, treat Y as up,
with Z being forward.  However, this change in coordinate systems is easy to
handle, by just converting the values before use.

# Rendering {#cardboard_rendering}

Rendering for Cardboard has to be handled, as the view needs to be duplicated
for the left and right eye.  Along with that, the Cardboard SDK provides the
function `undistortTexture()` in [CardboardView][], which undistorts the
given texture, adjusting the image to make it look correct for cardboard.

fplbase's **Mesh** class provides a function that enables rendering for
multiple viewports, `RenderStereo(...)`.  Zooshi uses this through the
**RenderMesh** component, from [CORGI][]'s [component library][], with the
extra setup necessary occuring in states/states_common.cpp,
`RenderStereoscopic()`. This handles rendering the same mesh from the slightly
different transforms needed for Cardboard to display correctly.

To use the screen undistortion effects, fplbase provides wrapper functions to
call before and after rendering the world, **HeadMountedDisplayRenderStart**
and **HeadMountedDisplayRenderEnd**.  These calls should wrap any rendering
that is meant to be seen with both eyes, and can be seen in use in the same
`RenderStereoscopic()` function as above.

<br>

  [Zooshi]: @ref zooshi_index
  [Cardboard SDK for Android]: https://developers.google.com/cardboard/android/
  [Designing for Google Cardboard]: http://www.google.com/design/spec-vr/designing-for-google-cardboard/a-new-dimension.html
  [CardboardView]: https://developers.google.com/cardboard/android/latest/reference/com/google/vrtoolkit/cardboard/CardboardView
  [fplbase]: https://google.github.io/fplbase/
  [CORGI]: https://google.github.io/corgi
  [component library]: http://google.github.io/corgi/component_library.html

