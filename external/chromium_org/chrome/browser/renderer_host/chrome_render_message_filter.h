// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
#define CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "third_party/WebKit/public/web/WebCache.h"

class CookieSettings;
class GURL;
class Profile;

namespace chrome_browser_net {
class Predictor;
}

namespace extensions {
class InfoMap;
}

// This class filters out incoming Chrome-specific IPC messages for the renderer
// process on the IPC thread.
class ChromeRenderMessageFilter : public content::BrowserMessageFilter {
 public:
  ChromeRenderMessageFilter(int render_process_id, Profile* profile);

  class V8HeapStatsDetails {
   public:
    V8HeapStatsDetails(size_t v8_memory_allocated,
                       size_t v8_memory_used)
        : v8_memory_allocated_(v8_memory_allocated),
          v8_memory_used_(v8_memory_used) {}
    size_t v8_memory_allocated() const { return v8_memory_allocated_; }
    size_t v8_memory_used() const { return v8_memory_used_; }
   private:
    size_t v8_memory_allocated_;
    size_t v8_memory_used_;
  };

  // content::BrowserMessageFilter methods:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void OverrideThreadForMessage(
      const IPC::Message& message,
      content::BrowserThread::ID* thread) OVERRIDE;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<ChromeRenderMessageFilter>;

  virtual ~ChromeRenderMessageFilter();

  void OnDnsPrefetch(const std::vector<std::string>& hostnames);
  void OnPreconnect(const GURL& url);
  void OnResourceTypeStats(const blink::WebCache::ResourceTypeStats& stats);
  void OnUpdatedCacheStats(const blink::WebCache::UsageStats& stats);
  void OnV8HeapStats(int v8_memory_allocated, int v8_memory_used);

  void OnAllowDatabase(int render_frame_id,
                       const GURL& origin_url,
                       const GURL& top_origin_url,
                       const base::string16& name,
                       const base::string16& display_name,
                       bool* allowed);
  void OnAllowDOMStorage(int render_frame_id,
                         const GURL& origin_url,
                         const GURL& top_origin_url,
                         bool local,
                         bool* allowed);
  void OnRequestFileSystemAccessSync(int render_frame_id,
                                     const GURL& origin_url,
                                     const GURL& top_origin_url,
                                     IPC::Message* message);
  void OnRequestFileSystemAccessAsync(int render_frame_id,
                                      int request_id,
                                      const GURL& origin_url,
                                      const GURL& top_origin_url);
  void OnAllowIndexedDB(int render_frame_id,
                        const GURL& origin_url,
                        const GURL& top_origin_url,
                        const base::string16& name,
                        bool* allowed);
#if defined(ENABLE_PLUGINS)
  void OnIsCrashReportingEnabled(bool* enabled);
#endif

  const int render_process_id_;

  // The Profile associated with our renderer process.  This should only be
  // accessed on the UI thread!
  Profile* profile_;
  // The Predictor for the associated Profile. It is stored so that it can be
  // used on the IO thread.
  chrome_browser_net::Predictor* predictor_;

  // Used to look up permissions at database creation time.
  scoped_refptr<CookieSettings> cookie_settings_;

  DISALLOW_COPY_AND_ASSIGN(ChromeRenderMessageFilter);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
