// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "class_requires_finalization_mixin.h"

namespace WebCore {

void Mixin::trace(Visitor* visitor)
{
    visitor->trace(m_onHeap);
}

void NeedsFinalizer::trace(Visitor* visitor)
{
    visitor->trace(m_obj);
    Mixin::trace(visitor);
}

void HasFinalizer::trace(Visitor* visitor)
{
    visitor->trace(m_obj);
    Mixin::trace(visitor);
}

}
