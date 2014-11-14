// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_AD_INJECTION_CONSTANTS_H_
#define EXTENSIONS_COMMON_AD_INJECTION_CONSTANTS_H_

#include <string>

#include "base/basictypes.h"

namespace extensions {
namespace ad_injection_constants {

// The keys used when serializing arguments to activity action APIs.
namespace keys {

extern const char kType[];
extern const char kChildren[];
extern const char kSrc[];
extern const char kHref[];

}  // namespace keys

extern const char kHtmlIframeSrcApiName[];
extern const char kHtmlEmbedSrcApiName[];
extern const char kHtmlAnchorHrefApiName[];
extern const char kAppendChildApiSuffix[];

extern const size_t kMaximumChildrenToCheck;
extern const size_t kMaximumDepthToCheck;

// Returns true if the given |api| can potentially inject ads, and should
// therefore be examined.
bool ApiCanInjectAds(const std::string& api);

}  // namespace ad_injection_constants
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_AD_INJECTION_CONSTANTS_H_
