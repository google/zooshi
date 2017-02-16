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

#include "invites.h"

#include "firebase/future.h"
#include "fplbase/utilities.h"

namespace fpl {
namespace zooshi {

const char* kInviteHandledKey = "zooshi:invite_handled";
const char* kInviteSentKey = "zooshi:invite_sent";

InvitesListener::InvitesListener()
    : received_invite_(false), invitation_id_(), deep_link_() {
  invite_handled_ = fplbase::LoadPreference(kInviteHandledKey, 0) != 0;
}

void SendInvite() {
  firebase::invites::Invite invite;
  invite.title_text = "Zooshi";
  invite.message_text = "Feed animals tasty sushi";
  invite.call_to_action_text = "Download it for FREE";
  firebase::invites::SendInvite(invite);
}

bool UpdateSentInviteStatus(bool* did_send, bool* first_sent) {
  auto future = firebase::invites::SendInviteLastResult();
  if (future.Status() == firebase::kFutureStatusComplete) {
    bool were_sent =
        future.Error() == 0 && future.Result()->invitation_ids.size() > 0;
    if (did_send) {
      *did_send = were_sent;
    }
    if (were_sent) {
      int invite_sent_count = fplbase::LoadPreference(kInviteSentKey, 0);
      if (first_sent) {
        *first_sent = invite_sent_count == 0;
      }
      fplbase::SavePreference(
          kInviteSentKey, static_cast<int32_t>(
              invite_sent_count + future.Result()->invitation_ids.size()));
    }
    return true;
  } else {
    return false;
  }
}

void InvitesListener::OnInviteReceived(const char* invitation_id,
                                       const char* deep_link,
                                       bool /*is_strong_match*/) {
  invitation_id_ = invitation_id ? invitation_id : "";
  deep_link_ = deep_link ? deep_link : "";
  received_invite_ = true;
  fplbase::LogInfo("Invitation received on start. ID: %s, Deep Link: %s",
                   invitation_id_.c_str(), deep_link_.c_str());
}

void InvitesListener::OnInviteNotReceived() {
  fplbase::LogInfo("No invitation received on start.");
}

void InvitesListener::OnErrorReceived(int error_code,
                                      const char* error_message) {
  fplbase::LogError("Error received while fetching invites: %d, %s", error_code,
                    error_message);
}

void InvitesListener::HandlePendingInvite() {
  if (!has_pending_invite()) return;
  invite_handled_ = true;
  fplbase::SavePreference(kInviteHandledKey, 1);
  if (!invitation_id_.empty()) {
    firebase::invites::ConvertInvitation(invitation_id_.c_str());
  }
}

void InvitesListener::Reset() {
  invite_handled_ = false;
  received_invite_ = false;
  fplbase::SavePreference(kInviteHandledKey, 0);
  fplbase::SavePreference(kInviteSentKey, 0);
}

}  // zooshi
}  // fpl
