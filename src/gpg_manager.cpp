// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "game.h"

#ifdef USING_GOOGLE_PLAY_GAMES
#include "gpg/gpg.h"
#endif
#include "gpg_manager.h"

using fplbase::LogError;
using fplbase::LogInfo;

namespace fpl {

GPGManager::GPGManager() {
#ifdef USING_GOOGLE_PLAY_GAMES
  state_ = kStart;
  do_ui_login_ = false;
  delayed_login_ = false;
#endif
}

#ifdef USING_GOOGLE_PLAY_GAMES
pthread_mutex_t GPGManager::events_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GPGManager::achievements_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GPGManager::players_mutex_ = PTHREAD_MUTEX_INITIALIZER;
#endif

bool GPGManager::Initialize(bool ui_login) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)ui_login;
  return false;
#else
  state_ = kStart;
  do_ui_login_ = ui_login;
  event_data_initialized_ = false;
  achievement_data_initialized_ = false;
  player_data_.reset(nullptr);

  /*
  // This code is here because we may be able to do this part of the
  // initialization here in the future, rather than relying on JNI_OnLoad below.
  auto env = reinterpret_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  JavaVM *vm = nullptr;
  auto ret = env->GetJavaVM(&vm);
  assert(ret >= 0);
  gpg::AndroidInitialization::JNI_OnLoad(vm);
  */
  gpg::AndroidPlatformConfiguration platform_configuration;
  platform_configuration.SetActivity(
      reinterpret_cast<jobject>(fplbase::AndroidGetActivity()));

  // Creates a games_services object that has lambda callbacks.
  game_services_ =
      gpg::GameServices::Builder()
          .SetDefaultOnLog(gpg::LogLevel::VERBOSE)
          .SetOnAuthActionStarted([this](gpg::AuthOperation op) {
             state_ =
                 state_ == kAuthUILaunched ? kAuthUIStarted : kAutoAuthStarted;
             LogInfo("GPG: Sign in started! (%d)", state_);
           })
          .SetOnAuthActionFinished([this](gpg::AuthOperation op,
                                          gpg::AuthStatus status) {
             LogInfo("GPG: Sign in finished with a result of %d (%d)", status,
                     state_);
             if (op == gpg::AuthOperation::SIGN_IN) {
               state_ = status == gpg::AuthStatus::VALID
                            ? kAuthed
                            : ((state_ == kAuthUIStarted ||
                                state_ == kAuthUILaunched)
                                   ? kAuthUIFailed
                                   : kAutoAuthFailed);
               if (state_ == kAuthed) {
                 // If we just logged in, go fetch our data!
                 FetchPlayer();
                 FetchEvents();
                 FetchAchievements();
               }
             } else if (op == gpg::AuthOperation::SIGN_OUT) {
               state_ = kStart;
               LogInfo("GPG: SIGN OUT finished with a result of %d", status);
             } else {
               LogInfo("GPG: unknown auth op %d", op);
             }
           })
          .Create(platform_configuration);

  if (!game_services_) {
    LogError("GPG: failed to create GameServices!");
    return false;
  }

  LogInfo("GPG: created GameServices");
  return true;
#endif
}

// Called every frame from the game, to see if there's anything to be done
// with the async progress from gpg
void GPGManager::Update() {
#ifdef USING_GOOGLE_PLAY_GAMES
  assert(game_services_);
  switch (state_) {
    case kStart:
    case kAutoAuthStarted:
      // Nothing to do, waiting.
      break;
    case kAutoAuthFailed:
    case kManualSignBackIn:
      // Need to explicitly ask for user  login.
      if (do_ui_login_) {
        LogInfo("GPG: StartAuthorizationUI");
        game_services_->StartAuthorizationUI();
        state_ = kAuthUILaunched;
        do_ui_login_ = false;
      } else {
        LogInfo("GPG: skipping StartAuthorizationUI");
        state_ = kAuthUIFailed;
      }
      break;
    case kAuthUILaunched:
    case kAuthUIStarted:
      // Nothing to do, waiting.
      break;
    case kAuthUIFailed:
      // Both auto and UI based auth failed, I guess at this point we give up.
      if (delayed_login_) {
        // Unless the user expressed desire to try log in again while waiting
        // for this state.
        delayed_login_ = false;
        state_ = kManualSignBackIn;
        do_ui_login_ = true;
      }
      break;
    case kAuthed:
      // We're good. TODO: Now start actually using gpg functionality...
      break;
  }
#endif
}

bool GPGManager::LoggedIn() {
#ifndef USING_GOOGLE_PLAY_GAMES
  return false;
#else
  assert(game_services_);
  if (state_ < kAuthed) {
    return false;
  }
  return true;
#endif
}

void GPGManager::ToggleSignIn() {
#ifndef USING_GOOGLE_PLAY_GAMES
  return;
#else
  delayed_login_ = false;
  if (state_ == kAuthed) {
    LogInfo("GPG: Attempting to log out...");
    game_services_->SignOut();
  } else if (state_ == kStart || state_ == kAuthUIFailed) {
    LogInfo("GPG: Attempting to log in...");
    state_ = kManualSignBackIn;
    do_ui_login_ = true;
  } else {
    LogInfo("GPG: Ignoring log in/out in state %d", state_);
    delayed_login_ = true;
  }
#endif
}

void GPGManager::IncrementEvent(const char *event_id, uint64_t score) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)event_id;
  (void)score;
#else
  if (!LoggedIn()) return;
  game_services_->Events().Increment(event_id, score);
#endif
}

// This is still somewhat game-specific.  (Because it assumes that your
// leaderboards are tied to events.)  TODO: clean this up further later.
void GPGManager::ShowLeaderboards(const GPGIds *ids, size_t id_len) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)ids;
  (void)id_len;
#else
  if (!LoggedIn()) return;
  LogInfo("GPG: launching leaderboard UI");
  // First, get all current event counts from GPG in one callback,
  // which allows us to conveniently update and show the leaderboards without
  // having to deal with multiple callbacks.
  game_services_->Events().FetchAll([id_len, ids, this](
      const gpg::EventManager::FetchAllResponse &far) {
    for (auto it = far.data.begin(); it != far.data.end(); ++it) {
      // Look up leaderboard id from corresponding event id.
      const char *leaderboard_id = nullptr;
      for (size_t i = 0; i < id_len; i++) {
        if (ids[i].event == it->first) {
          leaderboard_id = ids[i].leaderboard;
        }
      }
      assert(leaderboard_id);
      if (leaderboard_id) {
        game_services_->Leaderboards().SubmitScore(leaderboard_id,
                                                   it->second.Count());
        LogInfo("GPG: submitted score %llu for id %s", it->second.Count(),
                leaderboard_id);
      }
    }
    game_services_->Leaderboards().ShowAllUI([](const gpg::UIStatus &status) {
      LogInfo("GPG: Leaderboards UI FAILED, UIStatus is: %d", status);
    });
  });
#endif
}

void GPGManager::ShowLeaderboards(const char *id) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)id;
#else
  if (!LoggedIn()) return;
  LogInfo("GPG: launching leaderboards UI");
  game_services_->Leaderboards().ShowUI(id, [](const gpg::UIStatus &status) {
    LogInfo("GPG: Leaderboards UI FAILED, UIStatus is: %d", status);
  });
#endif
}

// Submit score to specified leaderboard.
void GPGManager::SubmitScore(std::string leaderboard_id, int64_t score) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)leaderboard_id;
  (void)score;
#else
  if (!LoggedIn()) return;
  game_services_->Leaderboards().SubmitScore(leaderboard_id, score);
  LogInfo("GPG: submitted score %llu for id %s", score, leaderboard_id.c_str());
#endif
}

// Submit score to specified leaderboard.
int64_t GPGManager::CurrentPlayerHighScore(std::string leaderboard_id) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)leaderboard_id;
  return 0;
#else
  if (!LoggedIn()) return 0;
  auto response = game_services_->Leaderboards().FetchScoreSummaryBlocking(
      leaderboard_id, gpg::LeaderboardTimeSpan::ALL_TIME,
      gpg::LeaderboardCollection::PUBLIC);
  auto score = response.data.CurrentPlayerScore().Value();
  LogInfo("GPG: player score %llu for leaderboard id %s", score,
          leaderboard_id.c_str());
  return score;
#endif
}

// Unlocks a given achievement.
void GPGManager::UnlockAchievement(std::string achievement_id) {
  if (LoggedIn()) {
#ifndef USING_GOOGLE_PLAY_GAMES
    (void)achievement_id;
#else
    game_services_->Achievements().Unlock(achievement_id);
    LogInfo("GPG: unlock achievment for id %s", achievement_id.c_str());
#endif
  }
}

// Increments an incremental achievement.
void GPGManager::IncrementAchievement(std::string achievement_id) {
  if (LoggedIn()) {
#ifndef USING_GOOGLE_PLAY_GAMES
    (void)achievement_id;
#else
    game_services_->Achievements().Increment(achievement_id);
#endif
  }
}

// Increments an incremental achievement by an amount.
void GPGManager::IncrementAchievement(std::string achievement_id,
                                      uint32_t steps) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)achievement_id;
  (void)steps;
#else
  if (LoggedIn()) {
    game_services_->Achievements().Increment(achievement_id, steps);
  }
#endif
}

// Reveals a given achievement.
void GPGManager::RevealAchievement(std::string achievement_id) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)achievement_id;
#else
  if (LoggedIn()) {
    game_services_->Achievements().Reveal(achievement_id);
  }
#endif
}

// Updates local player stats with values from the server:
void GPGManager::FetchEvents() {
#ifdef USING_GOOGLE_PLAY_GAMES
  if (!LoggedIn() || event_data_state_ == kPending) return;
  event_data_state_ = kPending;

  game_services_->Events()
      .FetchAll([this](const gpg::EventManager::FetchAllResponse &far) mutable {

         pthread_mutex_lock(&events_mutex_);

         if (IsSuccess(far.status)) {
           event_data_state_ = kComplete;
           event_data_initialized_ = true;
         } else {
           event_data_state_ = kFailed;
         }

         event_data_ = far.data;
         pthread_mutex_unlock(&events_mutex_);
       });
#endif
}

bool GPGManager::IsAchievementUnlocked(std::string achievement_id) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)achievement_id;
  return false;
#else
  if (!achievement_data_initialized_) {
    return false;
  }
  bool ret = false;
  pthread_mutex_lock(&achievements_mutex_);
  for (unsigned int i = 0; i < achievement_data_.size(); i++) {
    if (achievement_data_[i].Id() == achievement_id) {
      ret = achievement_data_[i].State() == gpg::AchievementState::UNLOCKED;
      break;
    }
  }
  pthread_mutex_unlock(&achievements_mutex_);
  return ret;
#endif
}

uint64_t GPGManager::GetEventValue(std::string event_id) {
#ifndef USING_GOOGLE_PLAY_GAMES
  (void)event_id;
  return 0;
#else
  if (!event_data_initialized_) {
    return 0;
  }
  pthread_mutex_lock(&achievements_mutex_);
  uint64_t result = event_data_[event_id].Count();
  pthread_mutex_unlock(&achievements_mutex_);
  return result;
#endif
}

// Updates local player achievements with values from the server:
void GPGManager::FetchAchievements() {
#ifdef USING_GOOGLE_PLAY_GAMES
  if (!LoggedIn() || achievement_data_state_ == kPending) return;
  achievement_data_state_ = kPending;

  game_services_->Achievements().FetchAll(
      [this](const gpg::AchievementManager::FetchAllResponse &far) mutable {

        pthread_mutex_lock(&achievements_mutex_);

        if (IsSuccess(far.status)) {
          achievement_data_state_ = kComplete;
          achievement_data_initialized_ = true;
        } else {
          achievement_data_state_ = kFailed;
        }

        achievement_data_ = far.data;
        pthread_mutex_unlock(&achievements_mutex_);
      });
#endif
}

void GPGManager::ShowAchievements() {
#ifdef USING_GOOGLE_PLAY_GAMES
  if (!LoggedIn()) return;
  LogInfo("GPG: launching achievement UI");
  game_services_->Achievements().ShowAllUI([](const gpg::UIStatus &status) {
    LogInfo("GPG: Achievement UI FAILED, UIStatus is: %d", status);
  });
#endif
}

void GPGManager::FetchPlayer() {
#ifdef USING_GOOGLE_PLAY_GAMES
  game_services_->Players().FetchSelf(
      [this](const gpg::PlayerManager::FetchSelfResponse &fsr) mutable {

        pthread_mutex_lock(&players_mutex_);
        if (IsSuccess(fsr.status)) {
          gpg::Player *player_data = new gpg::Player(fsr.data);
          LogInfo("GPG: got player info. ID = %s, name = %s, avatar=%s",
                  player_data->Id().c_str(), player_data->Name().c_str(),
                  player_data->AvatarUrl(gpg::ImageResolution::HI_RES).c_str());
          player_data_.reset(player_data);
        } else {
          LogError("GPG: failed to get player info");
          player_data_.reset(nullptr);
        }
        pthread_mutex_unlock(&players_mutex_);
      });
#endif
}

}  // fpl

#ifdef __ANDROID__
extern "C" JNIEXPORT jint GPG_JNI_OnLoad(JavaVM *vm, void *reserved) {
  fplbase::LogInfo("main: JNI_OnLoad called");

#ifdef USING_GOOGLE_PLAY_GAMES
  gpg::AndroidInitialization::JNI_OnLoad(vm);
#endif

  return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL
Java_com_google_fpl_fplbase_FPLActivity_nativeOnActivityResult(
    JNIEnv *env, jobject thiz, jobject activity, jint request_code,
    jint result_code, jobject data) {
#ifdef USING_GOOGLE_PLAY_GAMES
  gpg::AndroidSupport::OnActivityResult(env, activity, request_code,
                                        result_code, data);
#endif

  fplbase::LogInfo("GPG: nativeOnActivityResult");
}
#endif
