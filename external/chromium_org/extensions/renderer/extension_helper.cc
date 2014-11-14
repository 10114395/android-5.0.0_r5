// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extension_helper.h"

#include "base/lazy_instance.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/render_view_visitor.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_messages.h"
#include "extensions/renderer/console.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/messaging_bindings.h"
#include "extensions/renderer/user_script_scheduler.h"
#include "extensions/renderer/user_script_slave.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScopedUserGesture.h"
#include "third_party/WebKit/public/web/WebView.h"

using content::ConsoleMessageLevel;
using blink::WebConsoleMessage;
using blink::WebDataSource;
using blink::WebFrame;
using blink::WebLocalFrame;
using blink::WebURLRequest;
using blink::WebScopedUserGesture;
using blink::WebView;

namespace extensions {

namespace {
// Keeps a mapping from the frame pointer to a UserScriptScheduler object.
// We store this mapping per process, because a frame can jump from one
// document to another with adoptNode, and so having the object be a
// RenderViewObserver means it might miss some notifications after it moves.
typedef std::map<WebFrame*, UserScriptScheduler*> SchedulerMap;
static base::LazyInstance<SchedulerMap> g_schedulers =
    LAZY_INSTANCE_INITIALIZER;

// A RenderViewVisitor class that iterates through the set of available
// views, looking for a view of the given type, in the given browser window
// and within the given extension.
// Used to accumulate the list of views associated with an extension.
class ViewAccumulator : public content::RenderViewVisitor {
 public:
  ViewAccumulator(const std::string& extension_id,
                  int browser_window_id,
                  ViewType view_type)
      : extension_id_(extension_id),
        browser_window_id_(browser_window_id),
        view_type_(view_type) {
  }

  std::vector<content::RenderView*> views() { return views_; }

  // Returns false to terminate the iteration.
  virtual bool Visit(content::RenderView* render_view) OVERRIDE {
    ExtensionHelper* helper = ExtensionHelper::Get(render_view);
    if (!ViewTypeMatches(helper->view_type(), view_type_))
      return true;

    GURL url = render_view->GetWebView()->mainFrame()->document().url();
    if (!url.SchemeIs(kExtensionScheme))
      return true;
    const std::string& extension_id = url.host();
    if (extension_id != extension_id_)
      return true;

    if (browser_window_id_ != extension_misc::kUnknownWindowId &&
        helper->browser_window_id() != browser_window_id_) {
      return true;
    }

    views_.push_back(render_view);

    if (view_type_ == VIEW_TYPE_EXTENSION_BACKGROUND_PAGE)
      return false;  // There can be only one...
    return true;
  }

 private:
  // Returns true if |type| "isa" |match|.
  static bool ViewTypeMatches(ViewType type, ViewType match) {
    if (type == match)
      return true;

    // INVALID means match all.
    if (match == VIEW_TYPE_INVALID)
      return true;

    return false;
  }

  std::string extension_id_;
  int browser_window_id_;
  ViewType view_type_;
  std::vector<content::RenderView*> views_;
};

}  // namespace

// static
std::vector<content::RenderView*> ExtensionHelper::GetExtensionViews(
    const std::string& extension_id,
    int browser_window_id,
    ViewType view_type) {
  ViewAccumulator accumulator(extension_id, browser_window_id, view_type);
  content::RenderView::ForEach(&accumulator);
  return accumulator.views();
}

// static
content::RenderView* ExtensionHelper::GetBackgroundPage(
    const std::string& extension_id) {
  ViewAccumulator accumulator(extension_id, extension_misc::kUnknownWindowId,
                              VIEW_TYPE_EXTENSION_BACKGROUND_PAGE);
  content::RenderView::ForEach(&accumulator);
  CHECK_LE(accumulator.views().size(), 1u);
  if (accumulator.views().size() == 0)
    return NULL;
  return accumulator.views()[0];
}

ExtensionHelper::ExtensionHelper(content::RenderView* render_view,
                                 Dispatcher* dispatcher)
    : content::RenderViewObserver(render_view),
      content::RenderViewObserverTracker<ExtensionHelper>(render_view),
      dispatcher_(dispatcher),
      view_type_(VIEW_TYPE_INVALID),
      tab_id_(-1),
      browser_window_id_(-1) {
}

ExtensionHelper::~ExtensionHelper() {
}

bool ExtensionHelper::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ExtensionHelper, message)
    IPC_MESSAGE_HANDLER(ExtensionMsg_Response, OnExtensionResponse)
    IPC_MESSAGE_HANDLER(ExtensionMsg_MessageInvoke, OnExtensionMessageInvoke)
    IPC_MESSAGE_HANDLER(ExtensionMsg_DispatchOnConnect,
                        OnExtensionDispatchOnConnect)
    IPC_MESSAGE_HANDLER(ExtensionMsg_DeliverMessage, OnExtensionDeliverMessage)
    IPC_MESSAGE_HANDLER(ExtensionMsg_DispatchOnDisconnect,
                        OnExtensionDispatchOnDisconnect)
    IPC_MESSAGE_HANDLER(ExtensionMsg_ExecuteCode, OnExecuteCode)
    IPC_MESSAGE_HANDLER(ExtensionMsg_SetTabId, OnSetTabId)
    IPC_MESSAGE_HANDLER(ExtensionMsg_UpdateBrowserWindowId,
                        OnUpdateBrowserWindowId)
    IPC_MESSAGE_HANDLER(ExtensionMsg_NotifyRenderViewType,
                        OnNotifyRendererViewType)
    IPC_MESSAGE_HANDLER(ExtensionMsg_AddMessageToConsole,
                        OnAddMessageToConsole)
    IPC_MESSAGE_HANDLER(ExtensionMsg_AppWindowClosed,
                        OnAppWindowClosed)
    IPC_MESSAGE_HANDLER(ExtensionMsg_GrantContentScriptPermission,
                        OnGrantContentScriptPermission)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ExtensionHelper::DidFinishDocumentLoad(WebLocalFrame* frame) {
  dispatcher_->user_script_slave()->InjectScripts(
      frame, UserScript::DOCUMENT_END);

  SchedulerMap::iterator i = g_schedulers.Get().find(frame);
  if (i != g_schedulers.Get().end())
    i->second->DidFinishDocumentLoad();
}

void ExtensionHelper::DidFinishLoad(blink::WebLocalFrame* frame) {
  SchedulerMap::iterator i = g_schedulers.Get().find(frame);
  if (i != g_schedulers.Get().end())
    i->second->DidFinishLoad();
}

void ExtensionHelper::DidCreateDocumentElement(WebLocalFrame* frame) {
  dispatcher_->user_script_slave()->InjectScripts(
      frame, UserScript::DOCUMENT_START);
  SchedulerMap::iterator i = g_schedulers.Get().find(frame);
  if (i != g_schedulers.Get().end())
    i->second->DidCreateDocumentElement();

  dispatcher_->DidCreateDocumentElement(frame);
}

void ExtensionHelper::DidStartProvisionalLoad(blink::WebLocalFrame* frame) {
  SchedulerMap::iterator i = g_schedulers.Get().find(frame);
  if (i != g_schedulers.Get().end())
    i->second->DidStartProvisionalLoad();
}

void ExtensionHelper::DraggableRegionsChanged(blink::WebFrame* frame) {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      frame->document().draggableRegions();
  std::vector<DraggableRegion> regions;
  for (size_t i = 0; i < webregions.size(); ++i) {
    DraggableRegion region;
    region.bounds = webregions[i].bounds;
    region.draggable = webregions[i].draggable;
    regions.push_back(region);
  }
  Send(new ExtensionHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

void ExtensionHelper::FrameDetached(WebFrame* frame) {
  // This could be called before DidCreateDataSource, in which case the frame
  // won't be in the map.
  SchedulerMap::iterator i = g_schedulers.Get().find(frame);
  if (i == g_schedulers.Get().end())
    return;

  delete i->second;
  g_schedulers.Get().erase(i);

  dispatcher_->user_script_slave()->FrameDetached(frame);
}

void ExtensionHelper::DidMatchCSS(
    blink::WebLocalFrame* frame,
    const blink::WebVector<blink::WebString>& newly_matching_selectors,
    const blink::WebVector<blink::WebString>& stopped_matching_selectors) {
  dispatcher_->DidMatchCSS(
      frame, newly_matching_selectors, stopped_matching_selectors);
}

void ExtensionHelper::DidCreateDataSource(WebLocalFrame* frame,
                                          WebDataSource* ds) {
  // Check first if we created a scheduler for the frame, since this function
  // gets called for navigations within the document.
  if (g_schedulers.Get().count(frame))
    return;

  g_schedulers.Get()[frame] = new UserScriptScheduler(frame, dispatcher_);
}

void ExtensionHelper::OnExtensionResponse(int request_id,
                                          bool success,
                                          const base::ListValue& response,
                                          const std::string& error) {
  dispatcher_->OnExtensionResponse(request_id,
                                   success,
                                   response,
                                   error);
}

void ExtensionHelper::OnExtensionMessageInvoke(const std::string& extension_id,
                                               const std::string& module_name,
                                               const std::string& function_name,
                                               const base::ListValue& args,
                                               bool user_gesture) {
  dispatcher_->InvokeModuleSystemMethod(
      render_view(), extension_id, module_name, function_name, args,
      user_gesture);
}

void ExtensionHelper::OnExtensionDispatchOnConnect(
    int target_port_id,
    const std::string& channel_name,
    const base::DictionaryValue& source_tab,
    const ExtensionMsg_ExternalConnectionInfo& info,
    const std::string& tls_channel_id) {
  MessagingBindings::DispatchOnConnect(dispatcher_->script_context_set(),
                                       target_port_id,
                                       channel_name,
                                       source_tab,
                                       info,
                                       tls_channel_id,
                                       render_view());
}

void ExtensionHelper::OnExtensionDeliverMessage(int target_id,
                                                const Message& message) {
  MessagingBindings::DeliverMessage(
      dispatcher_->script_context_set(), target_id, message, render_view());
}

void ExtensionHelper::OnExtensionDispatchOnDisconnect(
    int port_id,
    const std::string& error_message) {
  MessagingBindings::DispatchOnDisconnect(
      dispatcher_->script_context_set(), port_id, error_message, render_view());
}

void ExtensionHelper::OnExecuteCode(
    const ExtensionMsg_ExecuteCode_Params& params) {
  WebView* webview = render_view()->GetWebView();
  WebFrame* main_frame = webview->mainFrame();
  if (!main_frame) {
    base::ListValue val;
    Send(new ExtensionHostMsg_ExecuteCodeFinished(routing_id(),
                                                  params.request_id,
                                                  "No main frame",
                                                  -1,
                                                  GURL(std::string()),
                                                  val));
    return;
  }

  // chrome.tabs.executeScript() only supports execution in either the top frame
  // or all frames.  We handle both cases in the top frame.
  SchedulerMap::iterator i = g_schedulers.Get().find(main_frame);
  if (i != g_schedulers.Get().end())
    i->second->ExecuteCode(params);
}

void ExtensionHelper::OnNotifyRendererViewType(ViewType type) {
  view_type_ = type;
}

void ExtensionHelper::OnSetTabId(int init_tab_id) {
  CHECK_EQ(tab_id_, -1);
  CHECK_GE(init_tab_id, 0);
  tab_id_ = init_tab_id;
}

void ExtensionHelper::OnUpdateBrowserWindowId(int window_id) {
  browser_window_id_ = window_id;
}

void ExtensionHelper::OnAddMessageToConsole(ConsoleMessageLevel level,
                                            const std::string& message) {
  console::AddMessage(render_view(), level, message);
}

void ExtensionHelper::OnAppWindowClosed() {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::Handle<v8::Context> v8_context =
      render_view()->GetWebView()->mainFrame()->mainWorldScriptContext();
  ScriptContext* script_context =
      dispatcher_->script_context_set().GetByV8Context(v8_context);
  if (!script_context)
    return;
  script_context->module_system()->CallModuleMethod("app.window",
                                                    "onAppWindowClosed");
}

void ExtensionHelper::OnGrantContentScriptPermission(int request_id) {
  dispatcher_->user_script_slave()->OnContentScriptGrantedPermission(
      render_view(), request_id);
}

}  // namespace extensions
