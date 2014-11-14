// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/test_login_utils.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/login/auth/mock_authenticator.h"
#include "chrome/browser/chromeos/login/auth/user_context.h"

namespace chromeos {

TestLoginUtils::TestLoginUtils(const UserContext& user_context)
    : expected_user_context_(user_context) {
}

TestLoginUtils::~TestLoginUtils() {}

void TestLoginUtils::PrepareProfile(
    const UserContext& user_context,
    bool has_cookies,
    bool has_active_session,
    Delegate* delegate) {
  if (user_context != expected_user_context_)
    NOTREACHED();
  // Profile hasn't been loaded.
  delegate->OnProfilePrepared(NULL);
}

void TestLoginUtils::DelegateDeleted(Delegate* delegate) {
}

scoped_refptr<Authenticator> TestLoginUtils::CreateAuthenticator(
    LoginStatusConsumer* consumer) {
  return new MockAuthenticator(consumer, expected_user_context_);
}

}  // namespace chromeos
