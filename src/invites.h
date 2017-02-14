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

#ifndef ZOOSHI_INVITES_H_
#define ZOOSHI_INVITES_H_

#include <string>

#include "mathfu/internal/disable_warnings_begin.h"

#include "firebase/app.h"
#include "firebase/future.h"
#include "firebase/invites.h"

#include "mathfu/internal/disable_warnings_end.h"

namespace fpl {
namespace zooshi {

// Helper function to construct and start sending an invite with Firebase.
void SendInvite();
// Handler to be called after starting to send an invite to check the status.
// Returns true when finished (successfully or not).
// Upon returning true, did_send is set to whether an invite was sent or not.
// If did_send is true, first_sent is set to whether this was the first time
// an invite was sent.
bool UpdateSentInviteStatus(bool* did_send, bool* first_sent);

class InvitesListener : public firebase::invites::Listener {
 public:
  InvitesListener();

  // Function called when an invite is received by Firebase.
  void OnInviteReceived(const char* invitation_id, const char* deep_link,
                        bool is_strong_match) override;

  void OnInviteNotReceived() override;
  void OnErrorReceived(int error_code, const char* error_message) override;

  // Mark the pending invite as handled, which is preserved across play
  // sessions.
  void HandlePendingInvite();

  // Resets the system, allowing for a new invite to be processed.
  void Reset();

  // Does the system currently have an invite to be handled.
  bool has_pending_invite() { return received_invite_ && !invite_handled_; }
  // Did the system receive an invitation.
  bool received_invite() const { return received_invite_; }
  // The invitation id of the received invite.
  const std::string& invitation_id() const { return invitation_id_; }
  // The deep link of the received invite.
  const std::string& deep_link() const { return deep_link_; }

 private:
  bool received_invite_;
  std::string invitation_id_;
  std::string deep_link_;

  bool invite_handled_;
};

}  // zooshi
}  // fpl

#endif  // ZOOSHI_INVITES_H_
