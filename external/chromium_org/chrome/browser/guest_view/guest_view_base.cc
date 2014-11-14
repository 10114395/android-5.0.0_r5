// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/guest_view/guest_view_base.h"

#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/guest_view/guest_view_constants.h"
#include "chrome/browser/guest_view/guest_view_manager.h"
#include "chrome/browser/guest_view/web_view/web_view_guest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/content_settings.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/event_router.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

using content::WebContents;

namespace {

typedef std::map<WebContents*, GuestViewBase*> WebContentsGuestViewMap;
static base::LazyInstance<WebContentsGuestViewMap> webcontents_guestview_map =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

GuestViewBase::Event::Event(const std::string& name,
                            scoped_ptr<base::DictionaryValue> args)
    : name_(name), args_(args.Pass()) {
}

GuestViewBase::Event::~Event() {
}

scoped_ptr<base::DictionaryValue> GuestViewBase::Event::GetArguments() {
  return args_.Pass();
}

// This observer ensures that the GuestViewBase destroys itself when its
// embedder goes away.
class GuestViewBase::EmbedderWebContentsObserver : public WebContentsObserver {
 public:
  explicit EmbedderWebContentsObserver(GuestViewBase* guest)
      : WebContentsObserver(guest->embedder_web_contents()),
        guest_(guest) {
  }

  virtual ~EmbedderWebContentsObserver() {
  }

  // WebContentsObserver implementation.
  virtual void WebContentsDestroyed() OVERRIDE {
    guest_->embedder_web_contents_ = NULL;
    guest_->EmbedderDestroyed();
    guest_->Destroy();
  }

 private:
  GuestViewBase* guest_;

  DISALLOW_COPY_AND_ASSIGN(EmbedderWebContentsObserver);
};

GuestViewBase::GuestViewBase(int guest_instance_id)
    : embedder_web_contents_(NULL),
      embedder_render_process_id_(0),
      browser_context_(NULL),
      guest_instance_id_(guest_instance_id),
      view_instance_id_(guestview::kInstanceIDNone),
      initialized_(false),
      weak_ptr_factory_(this) {
}

void GuestViewBase::Init(WebContents* guest_web_contents,
                         const std::string& embedder_extension_id) {
  if (initialized_)
    return;
  initialized_ = true;
  browser_context_ = guest_web_contents->GetBrowserContext();
  embedder_extension_id_ = embedder_extension_id;

  WebContentsObserver::Observe(guest_web_contents);
  guest_web_contents->SetDelegate(this);
  webcontents_guestview_map.Get().insert(
      std::make_pair(guest_web_contents, this));
  GuestViewManager::FromBrowserContext(browser_context_)->
      AddGuest(guest_instance_id_, guest_web_contents);
}

// static
GuestViewBase* GuestViewBase::Create(
    int guest_instance_id,
    WebContents* guest_web_contents,
    const std::string& embedder_extension_id,
    const std::string& view_type) {
  if (view_type == "webview") {
    return new WebViewGuest(guest_instance_id,
                            guest_web_contents,
                            embedder_extension_id);
  }
  NOTREACHED();
  return NULL;
}

// static
GuestViewBase* GuestViewBase::FromWebContents(WebContents* web_contents) {
  WebContentsGuestViewMap* guest_map = webcontents_guestview_map.Pointer();
  WebContentsGuestViewMap::iterator it = guest_map->find(web_contents);
  return it == guest_map->end() ? NULL : it->second;
}

// static
GuestViewBase* GuestViewBase::From(int embedder_process_id,
                                   int guest_instance_id) {
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(embedder_process_id);
  if (!host)
    return NULL;

  content::WebContents* guest_web_contents =
      GuestViewManager::FromBrowserContext(host->GetBrowserContext())->
          GetGuestByInstanceIDSafely(guest_instance_id, embedder_process_id);
  if (!guest_web_contents)
    return NULL;

  return GuestViewBase::FromWebContents(guest_web_contents);
}

// static
bool GuestViewBase::IsGuest(WebContents* web_contents) {
  return !!GuestViewBase::FromWebContents(web_contents);
}

// static
void GuestViewBase::GetDefaultContentSettingRules(
    RendererContentSettingRules* rules,
    bool incognito) {
  rules->image_rules.push_back(
      ContentSettingPatternSource(ContentSettingsPattern::Wildcard(),
                                  ContentSettingsPattern::Wildcard(),
                                  CONTENT_SETTING_ALLOW,
                                  std::string(),
                                  incognito));

  rules->script_rules.push_back(
      ContentSettingPatternSource(ContentSettingsPattern::Wildcard(),
                                  ContentSettingsPattern::Wildcard(),
                                  CONTENT_SETTING_ALLOW,
                                  std::string(),
                                  incognito));
}

base::WeakPtr<GuestViewBase> GuestViewBase::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool GuestViewBase::IsDragAndDropEnabled() const {
  return false;
}

void GuestViewBase::Destroy() {
  WillDestroy();
  if (!destruction_callback_.is_null())
    destruction_callback_.Run();
  delete guest_web_contents();
}

void GuestViewBase::DidAttach() {
  // Give the derived class an opportunity to perform some actions.
  DidAttachToEmbedder();

  SendQueuedEvents();
}

void GuestViewBase::SetOpener(GuestViewBase* guest) {
  if (guest && guest->IsViewType(GetViewType())) {
    opener_ = guest->AsWeakPtr();
    return;
  }
  opener_ = base::WeakPtr<GuestViewBase>();
}

void GuestViewBase::RegisterDestructionCallback(
    const DestructionCallback& callback) {
  destruction_callback_ = callback;
}

void GuestViewBase::WillAttach(content::WebContents* embedder_web_contents,
                               const base::DictionaryValue& extra_params) {
  embedder_web_contents_ = embedder_web_contents;
  embedder_web_contents_observer_.reset(
      new EmbedderWebContentsObserver(this));
  embedder_render_process_id_ =
      embedder_web_contents->GetRenderProcessHost()->GetID();
  extra_params.GetInteger(guestview::kParameterInstanceId, &view_instance_id_);
  extra_params_.reset(extra_params.DeepCopy());

  WillAttachToEmbedder();
}

void GuestViewBase::DidStopLoading(content::RenderViewHost* render_view_host) {
  if (!IsDragAndDropEnabled()) {
    const char script[] = "window.addEventListener('dragstart', function() { "
                          "  window.event.preventDefault(); "
                          "});";
    render_view_host->GetMainFrame()->ExecuteJavaScript(
        base::ASCIIToUTF16(script));
  }
  DidStopLoading();
}

void GuestViewBase::WebContentsDestroyed() {
  GuestDestroyed();
  delete this;
}

bool GuestViewBase::ShouldFocusPageAfterCrash() {
  // Focus is managed elsewhere.
  return false;
}

bool GuestViewBase::PreHandleGestureEvent(content::WebContents* source,
                                         const blink::WebGestureEvent& event) {
  return event.type == blink::WebGestureEvent::GesturePinchBegin ||
      event.type == blink::WebGestureEvent::GesturePinchUpdate ||
      event.type == blink::WebGestureEvent::GesturePinchEnd;
}

GuestViewBase::~GuestViewBase() {
  std::pair<int, int> key(embedder_render_process_id_, guest_instance_id_);

  webcontents_guestview_map.Get().erase(guest_web_contents());

  GuestViewManager::FromBrowserContext(browser_context_)->
      RemoveGuest(guest_instance_id_);

  pending_events_.clear();
}

void GuestViewBase::DispatchEvent(Event* event) {
  scoped_ptr<Event> event_ptr(event);
  if (!in_extension()) {
    NOTREACHED();
    return;
  }

  if (!attached()) {
    pending_events_.push_back(linked_ptr<Event>(event_ptr.release()));
    return;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context_);

  extensions::EventFilteringInfo info;
  info.SetInstanceID(view_instance_id_);
  scoped_ptr<base::ListValue> args(new base::ListValue());
  args->Append(event->GetArguments().release());

  extensions::EventRouter::DispatchEvent(
      embedder_web_contents_,
      profile,
      embedder_extension_id_,
      event->name(),
      args.Pass(),
      extensions::EventRouter::USER_GESTURE_UNKNOWN,
      info);
}

void GuestViewBase::SendQueuedEvents() {
  if (!attached())
    return;
  while (!pending_events_.empty()) {
    linked_ptr<Event> event_ptr = pending_events_.front();
    pending_events_.pop_front();
    DispatchEvent(event_ptr.release());
  }
}
