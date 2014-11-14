// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BROWSER_API_APP_RUNTIME_APP_RUNTIME_API_H_
#define APPS_BROWSER_API_APP_RUNTIME_APP_RUNTIME_API_H_

#include <string>
#include <vector>

class GURL;

namespace content {
class BrowserContext;
class WebContents;
}

namespace extensions {
class Extension;
}

namespace apps {

namespace file_handler_util {
struct GrantedFileEntry;
}

class AppEventRouter {
 public:
  // Dispatches the onLaunched event to the given app.
  static void DispatchOnLaunchedEvent(content::BrowserContext* context,
                                      const extensions::Extension* extension);

  // Dispatches the onRestarted event to the given app, providing a list of
  // restored file entries from the previous run.
  static void DispatchOnRestartedEvent(content::BrowserContext* context,
                                       const extensions::Extension* extension);

  // TODO(benwells): Update this comment, it is out of date.
  // Dispatches the onLaunched event to the given app, providing launch data of
  // the form:
  // {
  //   "intent" : {
  //     "type" : "chrome-extension://fileentry",
  //     "data" : a FileEntry,
  //     "postResults" : a null function,
  //     "postFailure" : a null function
  //   }
  // }

  // The FileEntries are created from |file_system_id| and |base_name|.
  // |handler_id| corresponds to the id of the file_handlers item in the
  // manifest that resulted in a match which triggered this launch.
  static void DispatchOnLaunchedEventWithFileEntries(
      content::BrowserContext* context,
      const extensions::Extension* extension,
      const std::string& handler_id,
      const std::vector<std::string>& mime_types,
      const std::vector<file_handler_util::GrantedFileEntry>& file_entries);

  // |handler_id| corresponds to the id of the url_handlers item
  // in the manifest that resulted in a match which triggered this launch.
  static void DispatchOnLaunchedEventWithUrl(
      content::BrowserContext* context,
      const extensions::Extension* extension,
      const std::string& handler_id,
      const GURL& url,
      const GURL& referrer_url);
};

}  // namespace apps

#endif  // APPS_BROWSER_API_APP_RUNTIME_APP_RUNTIME_API_H_
