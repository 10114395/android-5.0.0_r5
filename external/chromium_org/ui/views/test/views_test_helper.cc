// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/views_test_helper.h"

namespace views {

#if !defined(USE_AURA)
// static
ViewsTestHelper* ViewsTestHelper::Create(base::MessageLoopForUI* message_loop,
                                         ui::ContextFactory* context_factory) {
  return new ViewsTestHelper;
}
#endif

ViewsTestHelper::ViewsTestHelper() {
}

ViewsTestHelper::~ViewsTestHelper() {
}

void ViewsTestHelper::SetUp() {
}

void ViewsTestHelper::TearDown() {
}

gfx::NativeView ViewsTestHelper::GetContext() {
  return NULL;
}

}  // namespace views
