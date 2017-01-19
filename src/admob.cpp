// Copyright 2016 Google Inc. All rights reserved.
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

#include "admob.h"
#include "firebase/app.h"
#include "firebase/future.h"
#include "fplbase/utilities.h"
#include "world.h"

namespace fpl {
namespace zooshi {

namespace rewarded_video = firebase::admob::rewarded_video;

// Zooshi specific ad units that only serve test ads.
// In order to create your own ads, you'll need your own AdMob account.
// More information can be found at:
// https://support.google.com/admob/answer/2773509
const char* kAdMobAppID = "ca-app-pub-3940256099942544~1891588914";
const char* kRewardedVideoAdUnit = "ca-app-pub-3940256099942544/4705454513";

AdMobHelper::~AdMobHelper() {
  if (rewarded_video_available()) {
    rewarded_video::Destroy();
  }
}

void AdMobHelper::Initialize(const firebase::App& app) {
  firebase::admob::Initialize(app, kAdMobAppID);
  rewarded_video_status_ = kAdMobStatusInitializing;
  rewarded_video::Initialize().OnCompletion(
      [](const firebase::Future<void>& completed_future, void* void_helper) {
        AdMobHelper* helper = static_cast<AdMobHelper*>(void_helper);
        if (completed_future.Error()) {
          fplbase::LogError("Failed to initialize rewarded video: %s",
                            completed_future.ErrorMessage());
          helper->rewarded_video_status_ = kAdMobStatusError;
        } else {
          rewarded_video::SetListener(&helper->listener_);
          helper->LoadNewRewardedVideo();
        }
      },
      this);
}

void AdMobHelper::LoadNewRewardedVideo() {
  rewarded_video_status_ = kAdMobStatusLoading;
  // TODO(amaurice) Fill in request with information.
  firebase::admob::AdRequest request = {};
  rewarded_video::LoadAd(kRewardedVideoAdUnit, request)
      .OnCompletion(
          [](const firebase::Future<void>& completed_future,
             void* void_helper) {
            AdMobHelper* helper = static_cast<AdMobHelper*>(void_helper);
            if (completed_future.Error()) {
              fplbase::LogError("Failed to load rewarded video: %s",
                                completed_future.ErrorMessage());
              helper->rewarded_video_status_ = kAdMobStatusError;
            } else {
              helper->rewarded_video_status_ = kAdMobStatusAvailable;
            }
          },
          this);
}

void AdMobHelper::ShowRewardedVideo() {
  if (rewarded_video_status_ != kAdMobStatusAvailable) {
    fplbase::LogError("Unable to show rewarded video, not available");
    return;
  }

  rewarded_video_status_ = kAdMobStatusShowing;
  listener_.set_expecting_state_change(true);
  // TODO(amaurice) Fill in request with information.
  rewarded_video::Show(fplbase::AndroidGetActivity())
      .OnCompletion(
          [](const firebase::Future<void>& completed_future,
             void* void_helper) {
            AdMobHelper* helper = static_cast<AdMobHelper*>(void_helper);
            if (completed_future.Error()) {
              fplbase::LogError("Failed to show rewarded video: %s",
                                completed_future.ErrorMessage());
              helper->rewarded_video_status_ = kAdMobStatusError;
              helper->listener_.set_expecting_state_change(false);
            }
          },
          this);
}

bool AdMobHelper::CheckShowRewardedVideo() {
  // If still Loading, wait until it is finished.
  if (rewarded_video_status_ == kAdMobStatusLoading) {
    return false;
  } else if (rewarded_video_status_ == kAdMobStatusShowing) {
    if (!listener_.expecting_state_change() &&
        listener_.presentation_state() ==
            rewarded_video::kPresentationStateHidden) {
      rewarded_video_status_ = kAdMobStatusAvailable;
      return true;
    }
    return false;
  } else {
    // If we are not showing a rewarded video, return true.
    return true;
  }
}

void AdMobHelper::ApplyRewardedVideoBonus(XpSystem* xp_system) {
  if (!listener_.earned_reward()) return;

  // We add the bonus, but do not clear it out, so that we can track that we
  // still have one.
  xp_system->AddBonus(BonusApplyType_Addition, listener_.reward_item().amount,
                      1, UniqueBonusId_AdMobRewardedVideo);
}

RewardedVideoListener::RewardedVideoListener()
    : earned_reward_(false),
      expecting_state_change_(false),
      presentation_state_(rewarded_video::kPresentationStateHidden),
      reward_item_() {}

void RewardedVideoListener::OnPresentationStateChanged(
    rewarded_video::PresentationState state) {
  presentation_state_ = state;
  expecting_state_change_ = false;
}

void RewardedVideoListener::OnRewarded(rewarded_video::RewardItem reward) {
  earned_reward_ = true;
  reward_item_ = reward;
  fplbase::LogInfo("Rewarded Video: Earned Reward: %s: %f",
                   reward.reward_type.c_str(), reward.amount);
}

}  // zooshi
}  // fpl
