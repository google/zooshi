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
-   Create 5 achievement IDs and 1 leaderboard in the
    [Google Play Developer Console][].<br>
    5 achievements should be configured as follows:<br>
      -   "Sushi Chef", incremental achievement, 100 steps.<br>
      -   "Sushi Ninja", incremental achievement, 1000 steps.<br>
      -   "Sushi Samurai", incremental achievement, 10000 steps.<br>
      -   "A Delicious Cycle", regular achievement.<br>
      -   "All You Can Eat.", regular achievement.<br>
    -    Update IDs in  `src/rawassets/config.json` in the entry below.

~~~
    "gpg_config": {
      "leaderboards": [
      {"name": "LeaderboardMain", "id": "Enter the leaderboard ID here"},
      ],
      "achievements": [
      {"name": "ADeliciousCycle", "id": "Enter the achievement ID here"},
      {"name": "AllYouCanEat", "id": "Enter the achievement ID here"},
      {"name": "SushiChef", "id": "Enter the achievement ID here"},
      {"name": "SushiNinja", "id": "Enter the achievement ID here"},
      {"name": "SushiSamurai", "id": "Enter the achievement ID here"},
      ]
    }
~~~

<br>

  [Google Play Developer Console]: http://play.google.com/apps/publish/
  [Google Play Games Services]: http://developer.android.com/google/play-services/games.html
