// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_VIEWER_H_
#define COMPONENTS_DOM_DISTILLER_CORE_VIEWER_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"

namespace dom_distiller {

class DistilledArticleProto;
class DistilledPageProto;
class DomDistillerServiceInterface;
class ViewerHandle;
class ViewRequestDelegate;

namespace viewer {

// Returns a full HTML page based on the given |article_proto|. This is supposed
// to be displayed to the end user. The returned HTML should be considered
// unsafe, so callers must ensure rendering it does not compromise Chrome.
const std::string GetUnsafeArticleHtml(
    const DistilledArticleProto* article_proto);

// Returns the base Viewer HTML page based on the given |page_proto|. This is
// supposed to be displayed to the end user. The returned HTML should be
// considered unsafe, so callers must ensure rendering it does not compromise
// Chrome. The difference from |GetUnsafeArticleHtml| is that this can be used
// for displaying an in-flight distillation instead of waiting for the full
// article.
const std::string GetUnsafePartialArticleHtml(
    const DistilledPageProto* page_proto);

// Returns a JavaScript blob for updating a partial view request with additional
// distilled content. Meant for use when viewing a slow or long multi-page
// article. |is_last_page| indicates whether this is the last page of the
// article.
const std::string GetUnsafeIncrementalDistilledPageJs(
    const DistilledPageProto* page_proto,
    const bool is_last_page);

// Returns a JavaScript blob for controlling the "in-progress" indicator when
// viewing a partially-distilled page. |is_last_page| indicates whether this is
// the last page of the article (i.e. loading indicator should be removed).
const std::string GetToggleLoadingIndicatorJs(const bool is_last_page);

// Returns a full HTML page which displays a generic error.
const std::string GetErrorPageHtml();

// Returns the default CSS to be used for a viewer.
const std::string GetCss();

// Returns the default JS to be used for a viewer.
const std::string GetJavaScript();

// Based on the given path, calls into the DomDistillerServiceInterface for
// viewing distilled content based on the |path|.
scoped_ptr<ViewerHandle> CreateViewRequest(
    DomDistillerServiceInterface* dom_distiller_service,
    const std::string& path,
    ViewRequestDelegate* view_request_delegate);

}  // namespace viewer

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_VIEWER_H_
