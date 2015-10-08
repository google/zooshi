Google Play Games Services    {#zooshi_guide_play_games_services}
==========================

## Set Up Google Play Games Services

To use the [Google Play Games Services][] features in the game,
follow the steps below to set up [Google Play Games Services][] IDs:

-   Create an `App ID` with a new `package` name in the
    [Google Play Developer Console][].
    -   Replace `app_id` in `res/values/strings.xml` with the newly created
        one.
    -   Update the `package` name in `AndroidManifest.xml` and the java files.
        -   For example, rename `com.google.fpl.zooshi` to
            `com.mystudio.coolgame`.
-   Create 5 achievement IDs and 14 leaderboards in the
    [Google Play Developer Console][].

<br>

  [Google Play Developer Console]: http://play.google.com/apps/publish/
  [Google Play Games Services]: http://developer.android.com/google/play-services/games.html
