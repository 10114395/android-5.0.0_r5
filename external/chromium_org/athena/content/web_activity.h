// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATHENA_CONTENT_PUBLIC_WEB_ACTIVITY_H_
#define ATHENA_CONTENT_PUBLIC_WEB_ACTIVITY_H_

#include "athena/activity/public/activity.h"
#include "athena/activity/public/activity_view_model.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class BrowserContext;
class WebContents;
}

namespace views {
class WebView;
}

namespace athena {

class WebActivity : public Activity,
                    public ActivityViewModel,
                    public content::WebContentsObserver {
 public:
  WebActivity(content::BrowserContext* context, const GURL& gurl);
  virtual ~WebActivity();

 protected:
  // Activity:
  virtual athena::ActivityViewModel* GetActivityViewModel() OVERRIDE;

  // ActivityViewModel:
  virtual void Init() OVERRIDE;
  virtual SkColor GetRepresentativeColor() OVERRIDE;
  virtual base::string16 GetTitle() OVERRIDE;
  virtual views::View* GetContentsView() OVERRIDE;

  // content::WebContentsObserver:
  virtual void TitleWasSet(content::NavigationEntry* entry,
                           bool explicit_set) OVERRIDE;
  virtual void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& candidates) OVERRIDE;

 private:
  content::BrowserContext* browser_context_;
  content::WebContents* web_contents_;
  const GURL url_;
  views::WebView* web_view_;

  DISALLOW_COPY_AND_ASSIGN(WebActivity);
};

}  // namespace athena

#endif  // ATHENA_CONTENT_WEB_ACTIVITY_H_
