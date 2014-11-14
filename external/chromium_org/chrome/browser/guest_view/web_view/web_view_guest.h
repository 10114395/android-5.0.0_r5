// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_GUEST_H_
#define CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_GUEST_H_

#include <vector>

#include "base/observer_list.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/guest_view/guest_view.h"
#include "chrome/browser/guest_view/web_view/javascript_dialog_helper.h"
#include "chrome/browser/guest_view/web_view/web_view_find_helper.h"
#include "chrome/browser/guest_view/web_view/web_view_permission_types.h"
#include "chrome/common/extensions/api/webview.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/notification_registrar.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#endif

namespace webview_api = extensions::api::webview;

class RenderViewContextMenu;

namespace extensions {
class ScriptExecutor;
class WebviewFindFunction;
}  // namespace extensions

namespace ui {
class SimpleMenuModel;
}  // namespace ui

// A WebViewGuest provides the browser-side implementation of the <webview> API
// and manages the dispatch of <webview> extension events. WebViewGuest is
// created on attachment. That is, when a guest WebContents is associated with
// a particular embedder WebContents. This happens on either initial navigation
// or through the use of the New Window API, when a new window is attached to
// a particular <webview>.
class WebViewGuest : public GuestView<WebViewGuest>,
                     public content::NotificationObserver {
 public:
  WebViewGuest(int guest_instance_id,
               content::WebContents* guest_web_contents,
               const std::string& embedder_extension_id);

  // For WebViewGuest, we create special guest processes, which host the
  // tag content separately from the main application that embeds the tag.
  // A <webview> can specify both the partition name and whether the storage
  // for that partition should be persisted. Each tag gets a SiteInstance with
  // a specially formatted URL, based on the application it is hosted by and
  // the partition requested by it. The format for that URL is:
  // chrome-guest://partition_domain/persist?partition_name
  static bool GetGuestPartitionConfigForSite(const GURL& site,
                                             std::string* partition_domain,
                                             std::string* partition_name,
                                             bool* in_memory);

  // Returns guestview::kInstanceIDNone if |contents| does not correspond to a
  // WebViewGuest.
  static int GetViewInstanceId(content::WebContents* contents);
  // Parses partition related parameters from |extra_params|.
  // |storage_partition_id| is the parsed partition ID and |persist_storage|
  // specifies whether or not the partition is in memory.
  static void ParsePartitionParam(const base::DictionaryValue* extra_params,
                                  std::string* storage_partition_id,
                                  bool* persist_storage);
  static const char Type[];

  // Request navigating the guest to the provided |src| URL.
  void NavigateGuest(const std::string& src);

  typedef std::vector<linked_ptr<webview_api::ContextMenuItem> > MenuItemVector;
  // Shows the context menu for the guest.
  // |items| acts as a filter. This restricts the current context's default
  // menu items to contain only the items from |items|.
  // |items| == NULL means no filtering will be applied.
  void ShowContextMenu(int request_id, const MenuItemVector* items);

  // Sets the frame name of the guest.
  void SetName(const std::string& name);

  // Set the zoom factor.
  void SetZoom(double zoom_factor);

  // GuestViewBase implementation.
  virtual void DidAttachToEmbedder() OVERRIDE;
  virtual void DidStopLoading() OVERRIDE;
  virtual void EmbedderDestroyed() OVERRIDE;
  virtual void GuestDestroyed() OVERRIDE;
  virtual bool IsDragAndDropEnabled() const OVERRIDE;
  virtual void WillAttachToEmbedder() OVERRIDE;
  virtual void WillDestroy() OVERRIDE;

  // WebContentsDelegate implementation.
  virtual bool AddMessageToConsole(content::WebContents* source,
                                   int32 level,
                                   const base::string16& message,
                                   int32 line_no,
                                   const base::string16& source_id) OVERRIDE;
  virtual void LoadProgressChanged(content::WebContents* source,
                                   double progress) OVERRIDE;
  virtual void CloseContents(content::WebContents* source) OVERRIDE;
  virtual void FindReply(content::WebContents* source,
                         int request_id,
                         int number_of_matches,
                         const gfx::Rect& selection_rect,
                         int active_match_ordinal,
                         bool final_update) OVERRIDE;
  virtual bool HandleContextMenu(
      const content::ContextMenuParams& params) OVERRIDE;
  virtual void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) OVERRIDE;
  virtual void RendererResponsive(content::WebContents* source) OVERRIDE;
  virtual void RendererUnresponsive(content::WebContents* source) OVERRIDE;
  virtual void RequestMediaAccessPermission(
      content::WebContents* source,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) OVERRIDE;
  virtual void CanDownload(content::RenderViewHost* render_view_host,
                           const GURL& url,
                           const std::string& request_method,
                           const base::Callback<void(bool)>& callback) OVERRIDE;
  virtual content::JavaScriptDialogManager*
      GetJavaScriptDialogManager() OVERRIDE;
  virtual content::ColorChooser* OpenColorChooser(
      content::WebContents* web_contents,
      SkColor color,
      const std::vector<content::ColorSuggestion>& suggestions) OVERRIDE;
  virtual void RunFileChooser(
      content::WebContents* web_contents,
      const content::FileChooserParams& params) OVERRIDE;
  virtual void AddNewContents(content::WebContents* source,
                              content::WebContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture,
                              bool* was_blocked) OVERRIDE;
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;
  virtual void WebContentsCreated(content::WebContents* source_contents,
                                  int opener_render_frame_id,
                                  const base::string16& frame_name,
                                  const GURL& target_url,
                                  content::WebContents* new_contents) OVERRIDE;

  // BrowserPluginGuestDelegate implementation.
  virtual void SizeChanged(const gfx::Size& old_size, const gfx::Size& new_size)
      OVERRIDE;
  virtual void RequestPointerLockPermission(
      bool user_gesture,
      bool last_unlocked_by_target,
      const base::Callback<void(bool)>& callback) OVERRIDE;
  // NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Returns the current zoom factor.
  double GetZoom();

  // Begin or continue a find request.
  void Find(const base::string16& search_text,
            const blink::WebFindOptions& options,
            scoped_refptr<extensions::WebviewFindFunction> find_function);

  // Conclude a find request to clear highlighting.
  void StopFinding(content::StopFindAction);

  // If possible, navigate the guest to |relative_index| entries away from the
  // current navigation entry.
  void Go(int relative_index);

  // Reload the guest.
  void Reload();

  typedef base::Callback<void(bool /* allow */,
                              const std::string& /* user_input */)>
      PermissionResponseCallback;
  int RequestPermission(
      WebViewPermissionType permission_type,
      const base::DictionaryValue& request_info,
      const PermissionResponseCallback& callback,
      bool allowed_by_default);

  // Requests Geolocation Permission from the embedder.
  void RequestGeolocationPermission(int bridge_id,
                                    const GURL& requesting_frame,
                                    bool user_gesture,
                                    const base::Callback<void(bool)>& callback);
  void CancelGeolocationPermissionRequest(int bridge_id);

  void RequestFileSystemPermission(const GURL& url,
                                   bool allowed_by_default,
                                   const base::Callback<void(bool)>& callback);

  enum PermissionResponseAction {
    DENY,
    ALLOW,
    DEFAULT
  };

  enum SetPermissionResult {
    SET_PERMISSION_INVALID,
    SET_PERMISSION_ALLOWED,
    SET_PERMISSION_DENIED
  };

  // Responds to the permission request |request_id| with |action| and
  // |user_input|. Returns whether there was a pending request for the provided
  // |request_id|.
  SetPermissionResult SetPermission(int request_id,
                                    PermissionResponseAction action,
                                    const std::string& user_input);

  // Overrides the user agent for this guest.
  // This affects subsequent guest navigations.
  void SetUserAgentOverride(const std::string& user_agent_override);

  // Stop loading the guest.
  void Stop();

  // Kill the guest process.
  void Terminate();

  // Clears data in the storage partition of this guest.
  //
  // Partition data that are newer than |removal_since| will be removed.
  // |removal_mask| corresponds to bitmask in StoragePartition::RemoveDataMask.
  bool ClearData(const base::Time remove_since,
                 uint32 removal_mask,
                 const base::Closure& callback);

  extensions::ScriptExecutor* script_executor() {
    return script_executor_.get();
  }

  // Called when file system access is requested by the guest content using the
  // asynchronous HTML5 file system API. The request is plumbed through the
  // <webview> permission request API. The request will be:
  // - Allowed if the embedder explicitly allowed it.
  // - Denied if the embedder explicitly denied.
  // - Determined by the guest's content settings if the embedder does not
  // perform an explicit action.
  // If access was blocked due to the page's content settings,
  // |blocked_by_policy| should be true, and this function should invoke
  // OnContentBlocked.
  static void FileSystemAccessedAsync(int render_process_id,
                                      int render_frame_id,
                                      int request_id,
                                      const GURL& url,
                                      bool blocked_by_policy);

  // Called when file system access is requested by the guest content using the
  // synchronous HTML5 file system API in a worker thread or shared worker. The
  // request is plumbed through the <webview> permission request API. The
  // request will be:
  // - Allowed if the embedder explicitly allowed it.
  // - Denied if the embedder explicitly denied.
  // - Determined by the guest's content settings if the embedder does not
  // perform an explicit action.
  // If access was blocked due to the page's content settings,
  // |blocked_by_policy| should be true, and this function should invoke
  // OnContentBlocked.
  static void FileSystemAccessedSync(int render_process_id,
                                     int render_frame_id,
                                     const GURL& url,
                                     bool blocked_by_policy,
                                     IPC::Message* reply_msg);

 private:
  virtual ~WebViewGuest();

  // A map to store the callback for a request keyed by the request's id.
  struct PermissionResponseInfo {
    PermissionResponseCallback callback;
    WebViewPermissionType permission_type;
    bool allowed_by_default;
    PermissionResponseInfo();
    PermissionResponseInfo(const PermissionResponseCallback& callback,
                           WebViewPermissionType permission_type,
                           bool allowed_by_default);
    ~PermissionResponseInfo();
  };

  static void RecordUserInitiatedUMA(const PermissionResponseInfo& info,
                                     bool allow);

  // Returns the top level items (ignoring submenus) as Value.
  static scoped_ptr<base::ListValue> MenuModelToValue(
      const ui::SimpleMenuModel& menu_model);

  void OnWebViewGeolocationPermissionResponse(
      int bridge_id,
      bool user_gesture,
      const base::Callback<void(bool)>& callback,
      bool allow,
      const std::string& user_input);

  void OnWebViewFileSystemPermissionResponse(
      const base::Callback<void(bool)>& callback,
      bool allow,
      const std::string& user_input);

  void OnWebViewMediaPermissionResponse(
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback,
      bool allow,
      const std::string& user_input);

  void OnWebViewDownloadPermissionResponse(
      const base::Callback<void(bool)>& callback,
      bool allow,
      const std::string& user_input);

  void OnWebViewPointerLockPermissionResponse(
      const base::Callback<void(bool)>& callback,
      bool allow,
      const std::string& user_input);

  void OnWebViewNewWindowResponse(int new_window_instance_id,
                                  bool allow,
                                  const std::string& user_input);

  static void FileSystemAccessedAsyncResponse(int render_process_id,
                                              int render_frame_id,
                                              int request_id,
                                              const GURL& url,
                                              bool allowed);

  static void FileSystemAccessedSyncResponse(int render_process_id,
                                             int render_frame_id,
                                             const GURL& url,
                                             IPC::Message* reply_msg,
                                             bool allowed);

  // WebContentsObserver implementation.
  virtual void DidCommitProvisionalLoadForFrame(
      int64 frame_id,
      const base::string16& frame_unique_name,
      bool is_main_frame,
      const GURL& url,
      content::PageTransition transition_type,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidFailProvisionalLoad(
      int64 frame_id,
      const base::string16& frame_unique_name,
      bool is_main_frame,
      const GURL& validated_url,
      int error_code,
      const base::string16& error_description,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidStartProvisionalLoadForFrame(
      int64 frame_id,
      int64 parent_frame_id,
      bool is_main_frame,
      const GURL& validated_url,
      bool is_error_page,
      bool is_iframe_srcdoc,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DocumentLoadedInFrame(
      int64 frame_id,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual bool OnMessageReceived(
      const IPC::Message& message,
      content::RenderFrameHost* render_frame_host) OVERRIDE;
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;
  virtual void UserAgentOverrideSet(const std::string& user_agent) OVERRIDE;
  virtual void RenderViewReady() OVERRIDE;

  // Informs the embedder of a frame name change.
  void ReportFrameNameChange(const std::string& name);

  // Called after the load handler is called in the guest's main frame.
  void LoadHandlerCalled();

  // Called when a redirect notification occurs.
  void LoadRedirect(const GURL& old_url,
                    const GURL& new_url,
                    bool is_top_level);

  void AddWebViewToExtensionRendererState();
  static void RemoveWebViewFromExtensionRendererState(
      content::WebContents* web_contents);

#if defined(OS_CHROMEOS)
  // Notification of a change in the state of an accessibility setting.
  void OnAccessibilityStatusChanged(
    const chromeos::AccessibilityStatusEventDetails& details);
#endif

  void InjectChromeVoxIfNeeded(content::RenderViewHost* render_view_host);

  // Bridge IDs correspond to a geolocation request. This method will remove
  // the bookkeeping for a particular geolocation request associated with the
  // provided |bridge_id|. It returns the request ID of the geolocation request.
  int RemoveBridgeID(int bridge_id);

  void LoadURLWithParams(const GURL& url,
                         const content::Referrer& referrer,
                         content::PageTransition transition_type,
                         content::WebContents* web_contents);

  void RequestNewWindowPermission(
      WindowOpenDisposition disposition,
      const gfx::Rect& initial_bounds,
      bool user_gesture,
      content::WebContents* new_contents);

  // Destroy unattached new windows that have been opened by this
  // WebViewGuest.
  void DestroyUnattachedWindows();

  // Requests resolution of a potentially relative URL.
  GURL ResolveURL(const std::string& src);

  // Notification that a load in the guest resulted in abort. Note that |url|
  // may be invalid.
  void LoadAbort(bool is_top_level,
                 const GURL& url,
                 const std::string& error_type);

  void OnUpdateFrameName(bool is_top_level, const std::string& name);

  // Creates a new guest window owned by this WebViewGuest.
  WebViewGuest* CreateNewGuestWindow(const content::OpenURLParams& params);

  bool HandleKeyboardShortcuts(const content::NativeWebKeyboardEvent& event);

  ObserverList<extensions::TabHelper::ScriptExecutionObserver>
      script_observers_;
  scoped_ptr<extensions::ScriptExecutor> script_executor_;

  content::NotificationRegistrar notification_registrar_;

  // A counter to generate a unique request id for a context menu request.
  // We only need the ids to be unique for a given WebViewGuest.
  int pending_context_menu_request_id_;

  // A counter to generate a unique request id for a permission request.
  // We only need the ids to be unique for a given WebViewGuest.
  int next_permission_request_id_;

  typedef std::map<int, PermissionResponseInfo> RequestMap;
  RequestMap pending_permission_requests_;

  // True if the user agent is overridden.
  bool is_overriding_user_agent_;

  // Main frame ID of last committed page.
  int64 main_frame_id_;

  // Set to |true| if ChromeVox was already injected in main frame.
  bool chromevox_injected_;

  // Stores the current zoom factor.
  double current_zoom_factor_;

  // Stores the window name of the main frame of the guest.
  std::string name_;

  // Handles find requests and replies for the webview find API.
  WebviewFindHelper find_helper_;

  // Handles the JavaScript dialog requests.
  JavaScriptDialogHelper javascript_dialog_helper_;

  friend void WebviewFindHelper::DispatchFindUpdateEvent(bool canceled,
                                                         bool final_update);

  // Holds the RenderViewContextMenu that has been built but yet to be
  // shown. This is .Reset() after ShowContextMenu().
  scoped_ptr<RenderViewContextMenu> pending_menu_;

#if defined(OS_CHROMEOS)
  // Subscription to receive notifications on changes to a11y settings.
  scoped_ptr<chromeos::AccessibilityStatusSubscription>
      accessibility_subscription_;
#endif

  std::map<int, int> bridge_id_to_request_id_map_;

  // Tracks the name, and target URL of the new window. Once the first
  // navigation commits, we no longer track this information.
  struct NewWindowInfo {
    GURL url;
    std::string name;
    bool changed;
    NewWindowInfo(const GURL& url, const std::string& name) :
        url(url),
        name(name),
        changed(false) {}
  };

  typedef std::map<WebViewGuest*, NewWindowInfo> PendingWindowMap;
  PendingWindowMap pending_new_windows_;

  DISALLOW_COPY_AND_ASSIGN(WebViewGuest);
};

#endif  // CHROME_BROWSER_GUEST_VIEW_WEB_VIEW_WEB_VIEW_GUEST_H_
