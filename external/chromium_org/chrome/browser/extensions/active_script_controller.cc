// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/active_script_controller.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/stl_util.h"
#include "chrome/browser/extensions/active_tab_permission_granter.h"
#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/location_bar_controller.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/sessions/session_id.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/permissions/permissions_data.h"
#include "ipc/ipc_message_macros.h"

namespace extensions {

ActiveScriptController::PendingRequest::PendingRequest() :
    page_id(-1) {
}

ActiveScriptController::PendingRequest::PendingRequest(
    const base::Closure& closure,
    int page_id)
    : closure(closure),
      page_id(page_id) {
}

ActiveScriptController::PendingRequest::~PendingRequest() {
}

ActiveScriptController::ActiveScriptController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      enabled_(FeatureSwitch::scripts_require_action()->IsEnabled()) {
  CHECK(web_contents);
}

ActiveScriptController::~ActiveScriptController() {
  LogUMA();
}

// static
ActiveScriptController* ActiveScriptController::GetForWebContents(
    content::WebContents* web_contents) {
  if (!web_contents)
    return NULL;
  TabHelper* tab_helper = TabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return NULL;
  LocationBarController* location_bar_controller =
      tab_helper->location_bar_controller();
  // This should never be NULL.
  DCHECK(location_bar_controller);
  return location_bar_controller->active_script_controller();
}

bool ActiveScriptController::RequiresUserConsentForScriptInjection(
    const Extension* extension) {
  CHECK(extension);
  if (!extension->permissions_data()->RequiresActionForScriptExecution(
          extension,
          SessionID::IdForTab(web_contents()),
          web_contents()->GetVisibleURL()) ||
      util::AllowedScriptingOnAllUrls(extension->id(),
                                      web_contents()->GetBrowserContext())) {
    return false;
  }

  // If the feature is not enabled, we automatically allow all extensions to
  // run scripts.
  if (!enabled_)
    permitted_extensions_.insert(extension->id());

  return permitted_extensions_.count(extension->id()) == 0;
}

void ActiveScriptController::RequestScriptInjection(
    const Extension* extension,
    int page_id,
    const base::Closure& callback) {
  CHECK(extension);
  PendingRequestList& list = pending_requests_[extension->id()];
  list.push_back(PendingRequest(callback, page_id));

  // If this was the first entry, notify the location bar that there's a new
  // icon.
  if (list.size() == 1u)
    LocationBarController::NotifyChange(web_contents());
}

void ActiveScriptController::OnActiveTabPermissionGranted(
    const Extension* extension) {
  RunPendingForExtension(extension);
}

void ActiveScriptController::OnAdInjectionDetected(
    const std::set<std::string>& ad_injectors) {
  // We're only interested in data if there are ad injectors detected.
  if (ad_injectors.empty())
    return;

  size_t num_preventable_ad_injectors =
      base::STLSetIntersection<std::set<std::string> >(
          ad_injectors, permitted_extensions_).size();

  UMA_HISTOGRAM_COUNTS_100(
      "Extensions.ActiveScriptController.PreventableAdInjectors",
      num_preventable_ad_injectors);
  UMA_HISTOGRAM_COUNTS_100(
      "Extensions.ActiveScriptController.UnpreventableAdInjectors",
      ad_injectors.size() - num_preventable_ad_injectors);
}

ExtensionAction* ActiveScriptController::GetActionForExtension(
    const Extension* extension) {
  if (!enabled_ || pending_requests_.count(extension->id()) == 0)
    return NULL;  // No action for this extension.

  ActiveScriptMap::iterator existing =
      active_script_actions_.find(extension->id());
  if (existing != active_script_actions_.end())
    return existing->second.get();

  linked_ptr<ExtensionAction> action(new ExtensionAction(
      extension->id(), ActionInfo::TYPE_PAGE, ActionInfo()));
  action->SetTitle(ExtensionAction::kDefaultTabId, extension->name());
  action->SetIsVisible(ExtensionAction::kDefaultTabId, true);

  const ActionInfo* action_info = ActionInfo::GetPageActionInfo(extension);
  if (!action_info)
    action_info = ActionInfo::GetBrowserActionInfo(extension);

  if (action_info && !action_info->default_icon.empty()) {
    action->set_default_icon(
        make_scoped_ptr(new ExtensionIconSet(action_info->default_icon)));
  }

  active_script_actions_[extension->id()] = action;
  return action.get();
}

LocationBarController::Action ActiveScriptController::OnClicked(
    const Extension* extension) {
  DCHECK(ContainsKey(pending_requests_, extension->id()));
  RunPendingForExtension(extension);
  return LocationBarController::ACTION_NONE;
}

void ActiveScriptController::OnNavigated() {
  LogUMA();
  permitted_extensions_.clear();
  pending_requests_.clear();
}

void ActiveScriptController::OnExtensionUnloaded(const Extension* extension) {
  PendingRequestMap::iterator iter = pending_requests_.find(extension->id());
  if (iter != pending_requests_.end())
    pending_requests_.erase(iter);
}

void ActiveScriptController::RunPendingForExtension(
    const Extension* extension) {
  DCHECK(extension);
  PendingRequestMap::iterator iter =
      pending_requests_.find(extension->id());
  if (iter == pending_requests_.end())
    return;

  content::NavigationEntry* visible_entry =
      web_contents()->GetController().GetVisibleEntry();
  // Refuse to run if there's no visible entry, because we have no idea of
  // determining if it's the proper page. This should rarely, if ever, happen.
  if (!visible_entry)
    return;

  int page_id = visible_entry->GetPageID();

  // We add this to the list of permitted extensions and erase pending entries
  // *before* running them to guard against the crazy case where running the
  // callbacks adds more entries.
  permitted_extensions_.insert(extension->id());
  PendingRequestList requests;
  iter->second.swap(requests);
  pending_requests_.erase(extension->id());

  // Clicking to run the extension counts as granting it permission to run on
  // the given tab.
  // The extension may already have active tab at this point, but granting
  // it twice is essentially a no-op.
  TabHelper::FromWebContents(web_contents())->
      active_tab_permission_granter()->GrantIfRequested(extension);

  // Run all pending injections for the given extension.
  for (PendingRequestList::iterator request = requests.begin();
       request != requests.end();
       ++request) {
    // Only run if it's on the proper page.
    if (request->page_id == page_id)
      request->closure.Run();
  }

  // Inform the location bar that the action is now gone.
  LocationBarController::NotifyChange(web_contents());
}

void ActiveScriptController::OnRequestContentScriptPermission(
    const std::string& extension_id,
    int page_id,
    int request_id) {
  if (!Extension::IdIsValid(extension_id)) {
    NOTREACHED() << "'" << extension_id << "' is not a valid id.";
    return;
  }

  const Extension* extension =
      ExtensionRegistry::Get(web_contents()->GetBrowserContext())
          ->enabled_extensions().GetByID(extension_id);
  // We shouldn't allow extensions which are no longer enabled to run any
  // scripts. Ignore the request.
  if (!extension)
    return;

  // If the request id is -1, that signals that the content script has already
  // ran (because this feature is not enabled). Add the extension to the list of
  // permitted extensions (for metrics), and return immediately.
  if (request_id == -1) {
    DCHECK(!enabled_);
    permitted_extensions_.insert(extension->id());
    return;
  }

  if (RequiresUserConsentForScriptInjection(extension)) {
    // This base::Unretained() is safe, because the callback is only invoked by
    // this object.
    RequestScriptInjection(
        extension,
        page_id,
        base::Bind(&ActiveScriptController::GrantContentScriptPermission,
                   base::Unretained(this),
                   request_id));
  } else {
    GrantContentScriptPermission(request_id);
  }
}

void ActiveScriptController::GrantContentScriptPermission(int request_id) {
  content::RenderViewHost* render_view_host =
      web_contents()->GetRenderViewHost();
  if (render_view_host) {
    render_view_host->Send(new ExtensionMsg_GrantContentScriptPermission(
                               render_view_host->GetRoutingID(),
                               request_id));
  }
}

bool ActiveScriptController::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ActiveScriptController, message)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_RequestContentScriptPermission,
                        OnRequestContentScriptPermission)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ActiveScriptController::LogUMA() const {
  UMA_HISTOGRAM_COUNTS_100(
      "Extensions.ActiveScriptController.ShownActiveScriptsOnPage",
      pending_requests_.size());

  // We only log the permitted extensions metric if the feature is enabled,
  // because otherwise the data will be boring (100% allowed).
  if (enabled_) {
    UMA_HISTOGRAM_COUNTS_100(
        "Extensions.ActiveScriptController.PermittedExtensions",
        permitted_extensions_.size());
    UMA_HISTOGRAM_COUNTS_100(
        "Extensions.ActiveScriptController.DeniedExtensions",
        pending_requests_.size());
  }
}

}  // namespace extensions
