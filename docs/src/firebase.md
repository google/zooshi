Firebase {#zooshi_guide_firebase}
===================

#  Introduction {#firebase_introduction}

This document outlines how the [Firebase C++ SDK][] is used in
[Zooshi][]. For additional samples on how to integrate Firebase, be sure
to check out [Firebase Samples][].

# Overview {#firebase_overview}

Firebase makes it easy to add backend services and analytics to your mobile
games on iOS and Android. With the Firebase C++ SDK, you can access Firebase
services directly in your C++ code, without having to write any Java or
Swift (or Objective-C) code.

# AdMob {#firebase_admob}

[Firebase AdMob][] is used to offer [Rewarded Video][] ads to the user,
in exchange for bonus XP granted upon playing the level. The relevant code can
be found in (admob.h/.cpp).

# Analytics {#firebase_analytics}

[Firebase Analytics][] is used to collect analytics from various events, such
as when a level is started, or when a patron is fed. The relevant code can
be found in (analytics.h/.cpp).

# Cloud Messaging {#firebase_messaging}

[Firebase Cloud Messaging][] is used to deliver messages to users, which can
call users into playing, and can include rewards. The relevant code can be
found in (messaging.h/.cpp).

# Invites {#firebase_invites}

[Firebase Invites][] lets users invite others to try Zooshi with app referals
via email, which gets rewarded with unlockables. The relevant code can be found
in (invites.h/.cpp).

# Remote Config {#firebase_remote_config}

[Firebase Remote Config][] is used to allow modification of menu items within
Zooshi without needing to push new versions, through the Firebase console.
The relevant code can be found in (remote_config.h/.cpp).

<br>

  [Zooshi]: @ref zooshi_index
  [Firebase C++ SDK]: https://firebase.google.com/docs/cpp/setup
  [Firebase Samples]: https://firebase.google.com/docs/samples
  [Firebase AdMob]: https://firebase.google.com/docs/admob/cpp/quick-start
  [Rewarded Video]: https://firebase.google.com/docs/admob/cpp/rewarded-video
  [Firebase Analytics]: https://firebase.google.com/docs/analytics/cpp/start
  [Firebase Cloud Messaging]: https://firebase.google.com/docs/cloud-messaging/cpp/client
  [Firebase Invites]: https://firebase.google.com/docs/invites/cpp
  [Firebase Remote Config]: https://firebase.google.com/docs/remote-config/use-config-cpp

