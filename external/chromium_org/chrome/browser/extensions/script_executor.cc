// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/script_executor.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "chrome/browser/extensions/active_script_controller.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_messages.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"

namespace base {
class ListValue;
}  // namespace base

namespace extensions {

namespace {

const char* kRendererDestroyed = "The tab was closed.";

// A handler for a single injection request. On creation this will send the
// injection request to the renderer, and it will be destroyed after either the
// corresponding response comes from the renderer, or the renderer is destroyed.
class Handler : public content::WebContentsObserver {
 public:
  Handler(ObserverList<TabHelper::ScriptExecutionObserver>* script_observers,
          content::WebContents* web_contents,
          const ExtensionMsg_ExecuteCode_Params& params,
          const ScriptExecutor::ExecuteScriptCallback& callback)
          : content::WebContentsObserver(web_contents),
            script_observers_(AsWeakPtr(script_observers)),
            extension_id_(params.extension_id),
            request_id_(params.request_id),
            callback_(callback) {
    content::RenderViewHost* rvh = web_contents->GetRenderViewHost();
    rvh->Send(new ExtensionMsg_ExecuteCode(rvh->GetRoutingID(), params));
  }

  virtual ~Handler() {}

  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE {
    // Unpack by hand to check the request_id, since there may be multiple
    // requests in flight but only one is for this.
    if (message.type() != ExtensionHostMsg_ExecuteCodeFinished::ID)
      return false;

    int message_request_id;
    PickleIterator iter(message);
    CHECK(message.ReadInt(&iter, &message_request_id));

    if (message_request_id != request_id_)
      return false;

    IPC_BEGIN_MESSAGE_MAP(Handler, message)
      IPC_MESSAGE_HANDLER(ExtensionHostMsg_ExecuteCodeFinished,
                          OnExecuteCodeFinished)
    IPC_END_MESSAGE_MAP()
    return true;
  }

  virtual void WebContentsDestroyed() OVERRIDE {
    base::ListValue val;
    callback_.Run(kRendererDestroyed, -1, GURL(std::string()), val);
    delete this;
  }

 private:
  void OnExecuteCodeFinished(int request_id,
                             const std::string& error,
                             int32 on_page_id,
                             const GURL& on_url,
                             const base::ListValue& script_result) {
    if (script_observers_.get() && error.empty()) {
      TabHelper::ScriptExecutionObserver::ExecutingScriptsMap id_map;
      id_map[extension_id_] = std::set<std::string>();
      FOR_EACH_OBSERVER(TabHelper::ScriptExecutionObserver, *script_observers_,
                        OnScriptsExecuted(web_contents(),
                                          id_map,
                                          on_page_id,
                                          on_url));
    }

    callback_.Run(error, on_page_id, on_url, script_result);
    delete this;
  }

  base::WeakPtr<ObserverList<TabHelper::ScriptExecutionObserver> >
      script_observers_;
  std::string extension_id_;
  int request_id_;
  ScriptExecutor::ExecuteScriptCallback callback_;
};

}  // namespace

ScriptExecutor::ScriptExecutor(
    content::WebContents* web_contents,
    ObserverList<TabHelper::ScriptExecutionObserver>* script_observers)
    : next_request_id_(0),
      web_contents_(web_contents),
      script_observers_(script_observers) {
  CHECK(web_contents_);
}

ScriptExecutor::~ScriptExecutor() {}

void ScriptExecutor::ExecuteScript(const std::string& extension_id,
                                   ScriptExecutor::ScriptType script_type,
                                   const std::string& code,
                                   ScriptExecutor::FrameScope frame_scope,
                                   ScriptExecutor::MatchAboutBlank about_blank,
                                   UserScript::RunLocation run_at,
                                   ScriptExecutor::WorldType world_type,
                                   ScriptExecutor::ProcessType process_type,
                                   const GURL& webview_src,
                                   const GURL& file_url,
                                   bool user_gesture,
                                   ScriptExecutor::ResultType result_type,
                                   const ExecuteScriptCallback& callback) {
  // Don't execute if the extension has been unloaded.
  const Extension* extension =
      ExtensionRegistry::Get(web_contents_->GetBrowserContext())
          ->enabled_extensions().GetByID(extension_id);
  if (!extension)
    return;

  // Don't execute if there's no visible entry. If this is the case, then our
  // permissions checking is useless (because we can't evaluate the URL).
  // TODO(rdevlin.cronin): This might be better somewhere higher up the
  // callstack, but we know it's caught here.
  content::NavigationEntry* visible_entry =
      web_contents_->GetController().GetVisibleEntry();
  if (!visible_entry)
    return;

  scoped_ptr<ExtensionMsg_ExecuteCode_Params> params(
      new ExtensionMsg_ExecuteCode_Params());
  params->request_id = next_request_id_++;
  params->extension_id = extension_id;
  params->is_javascript = (script_type == JAVASCRIPT);
  params->code = code;
  params->all_frames = (frame_scope == ALL_FRAMES);
  params->match_about_blank = (about_blank == MATCH_ABOUT_BLANK);
  params->run_at = static_cast<int>(run_at);
  params->in_main_world = (world_type == MAIN_WORLD);
  params->is_web_view = (process_type == WEB_VIEW_PROCESS);
  params->webview_src = webview_src;
  params->file_url = file_url;
  params->wants_result = (result_type == JSON_SERIALIZED_RESULT);
  params->user_gesture = user_gesture;

  ActiveScriptController* active_script_controller =
      ActiveScriptController::GetForWebContents(web_contents_);
  if (active_script_controller &&
      active_script_controller->RequiresUserConsentForScriptInjection(
          extension)) {
    // The base::Unretained(this) is safe, because this and the
    // ActiveScriptController are both attached to the TabHelper. Thus, if the
    // ActiveScriptController is still alive to invoke the callback, this is
    // alive, too.
    active_script_controller->RequestScriptInjection(
        extension,
        visible_entry->GetPageID(),
        base::Closure(base::Bind(&ScriptExecutor::ExecuteScriptHelper,
                                 base::Unretained(this),
                                 base::Passed(params.Pass()),
                                 callback)));
  } else {
    ExecuteScriptHelper(params.Pass(), callback);
  }
}

void ScriptExecutor::ExecuteScriptHelper(
    scoped_ptr<ExtensionMsg_ExecuteCode_Params> params,
    const ExecuteScriptCallback& callback) {
  // Handler handles IPCs and deletes itself on completion.
  new Handler(script_observers_, web_contents_, *params, callback);
}

}  // namespace extensions
