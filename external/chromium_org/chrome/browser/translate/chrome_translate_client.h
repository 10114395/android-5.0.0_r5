// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRANSLATE_CHROME_TRANSLATE_CLIENT_H_
#define CHROME_BROWSER_TRANSLATE_CHROME_TRANSLATE_CLIENT_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"
#include "components/translate/content/browser/content_translate_driver.h"
#include "components/translate/core/browser/translate_client.h"
#include "components/translate/core/browser/translate_step.h"
#include "components/translate/core/common/translate_errors.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if defined(CLD2_DYNAMIC_MODE)
#include "base/basictypes.h"
#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"
#include "base/task_runner.h"
#endif

namespace base {
class File;
}  // namespace base

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace test {
class ScopedCLDDynamicDataHarness;
}  // namespace test

struct LanguageDetectionDetails;
class LanguageState;
class PrefService;
class TranslateAcceptLanguages;
class TranslatePrefs;
class TranslateManager;

class ChromeTranslateClient
    : public TranslateClient,
      public content::WebContentsObserver,
      public content::WebContentsUserData<ChromeTranslateClient> {
 public:
  virtual ~ChromeTranslateClient();

  // Gets the LanguageState associated with the page.
  LanguageState& GetLanguageState();

  // Returns the ContentTranslateDriver instance associated with this
  // WebContents.
  ContentTranslateDriver& translate_driver() { return translate_driver_; }

  // Helper method to return a new TranslatePrefs instance.
  static scoped_ptr<TranslatePrefs> CreateTranslatePrefs(PrefService* prefs);

  // Helper method to return the TranslateAcceptLanguages instance associated
  // with |browser_context|.
  static TranslateAcceptLanguages* GetTranslateAcceptLanguages(
      content::BrowserContext* browser_context);

  // Helper method to return the TranslateManager instance associated with
  // |web_contents|, or NULL if there is no such associated instance.
  static TranslateManager* GetManagerFromWebContents(
      content::WebContents* web_contents);

  // Gets |source| and |target| language for translation.
  static void GetTranslateLanguages(content::WebContents* web_contents,
                                    std::string* source,
                                    std::string* target);

  // Gets the associated TranslateManager.
  TranslateManager* GetTranslateManager();

  // Gets the associated WebContents. Returns NULL if the WebContents is being
  // destroyed.
  content::WebContents* GetWebContents();

  // Number of attempts before waiting for a page to be fully reloaded.
  void set_translate_max_reload_attempts(int attempts) {
    max_reload_check_attempts_ = attempts;
  }

  // TranslateClient implementation.
  virtual TranslateDriver* GetTranslateDriver() OVERRIDE;
  virtual PrefService* GetPrefs() OVERRIDE;
  virtual scoped_ptr<TranslatePrefs> GetTranslatePrefs() OVERRIDE;
  virtual TranslateAcceptLanguages* GetTranslateAcceptLanguages() OVERRIDE;
  virtual int GetInfobarIconID() const OVERRIDE;
  virtual scoped_ptr<infobars::InfoBar> CreateInfoBar(
      scoped_ptr<TranslateInfoBarDelegate> delegate) const OVERRIDE;
  virtual void ShowTranslateUI(translate::TranslateStep step,
                               const std::string source_language,
                               const std::string target_language,
                               TranslateErrors::Type error_type,
                               bool triggered_from_menu) OVERRIDE;
  virtual bool IsTranslatableURL(const GURL& url) OVERRIDE;
  virtual void ShowReportLanguageDetectionErrorUI(
      const GURL& report_url) OVERRIDE;

 private:
  explicit ChromeTranslateClient(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ChromeTranslateClient>;
  friend class test::ScopedCLDDynamicDataHarness;  // For cleaning static state.

  // content::WebContentsObserver implementation.
  virtual void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void DidNavigateAnyFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) OVERRIDE;
  virtual void WebContentsDestroyed() OVERRIDE;

  // Initiates translation once the page is finished loading.
  void InitiateTranslation(const std::string& page_lang, int attempt);
  void OnLanguageDetermined(const LanguageDetectionDetails& details,
                            bool page_needs_translation);
  void OnPageTranslated(int32 page_id,
                        const std::string& original_lang,
                        const std::string& translated_lang,
                        TranslateErrors::Type error_type);

#if defined(CLD2_DYNAMIC_MODE)
  // Called when we receive ChromeViewHostMsg_NeedCLDData from a renderer.
  // If we have already cached the data, responds immediately; else, enqueues
  // a HandleCLDDataRequest on the blocking pool to cache the data.
  // Acquires and releases s_file_lock_ in a non-blocking manner; queries
  // handled while the file is being cached will gracefully and immediately
  // fail.
  // It is up to the originator of the message to poll again later if required;
  // no "negative response" will be generated.
  void OnCLDDataRequested();

  // Invoked on the blocking pool in order to cache the data. When successful,
  // immediately responds to the request that initiated OnCLDDataRequested.
  // Holds s_file_lock_ while the file is being cached.
  static void HandleCLDDataRequest();

  // If the CLD data is ready, send it to the renderer. Briefly checks the lock.
  void MaybeSendCLDDataAvailable();

  // Sends the renderer a response containing the data file handle. No locking.
  void SendCLDDataAvailable(const base::File* handle,
                            const uint64 data_offset,
                            const uint64 data_length);

  // The data file,  cached as long as the process stays alive.
  // We also track the offset at which the data starts, and its length.
  static base::File* s_cached_file_;    // guarded by file_lock_
  static uint64 s_cached_data_offset_;  // guarded by file_lock_
  static uint64 s_cached_data_length_;  // guarded by file_lock_

  // Guards s_cached_file_
  static base::LazyInstance<base::Lock> s_file_lock_;

#endif

  // Shows the translate bubble.
  void ShowBubble(translate::TranslateStep step,
                  TranslateErrors::Type error_type);

  // Max number of attempts before checking if a page has been reloaded.
  int max_reload_check_attempts_;

  ContentTranslateDriver translate_driver_;
  scoped_ptr<TranslateManager> translate_manager_;

  // Necessary for binding the callback to HandleCLDDataRequest on the blocking
  // pool and for delaying translation initialization until the page has
  // finished loading on a reload.
  base::WeakPtrFactory<ChromeTranslateClient> weak_pointer_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeTranslateClient);
};

#endif  // CHROME_BROWSER_TRANSLATE_CHROME_TRANSLATE_CLIENT_H_
