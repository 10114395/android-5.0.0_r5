// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_blocking_page.h"

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/ssl/ssl_error_info.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/cert_store.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/ssl_status.h"
#include "grit/app_locale_settings.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "net/base/hash_value.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
#include "chrome/browser/captive_portal/captive_portal_service.h"
#include "chrome/browser/captive_portal/captive_portal_service_factory.h"
#endif

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

using base::ASCIIToUTF16;
using base::TimeTicks;
using content::InterstitialPage;
using content::NavigationController;
using content::NavigationEntry;

namespace {

// Events for UMA. Do not reorder or change!
enum SSLBlockingPageEvent {
  SHOW_ALL,
  SHOW_OVERRIDABLE,
  PROCEED_OVERRIDABLE,
  PROCEED_NAME,
  PROCEED_DATE,
  PROCEED_AUTHORITY,
  DONT_PROCEED_OVERRIDABLE,
  DONT_PROCEED_NAME,
  DONT_PROCEED_DATE,
  DONT_PROCEED_AUTHORITY,
  MORE,
  SHOW_UNDERSTAND,  // Used by the summer 2013 Finch trial. Deprecated.
  SHOW_INTERNAL_HOSTNAME,
  PROCEED_INTERNAL_HOSTNAME,
  SHOW_NEW_SITE,
  PROCEED_NEW_SITE,
  PROCEED_MANUAL_NONOVERRIDABLE,
  CAPTIVE_PORTAL_DETECTION_ENABLED,
  CAPTIVE_PORTAL_DETECTION_ENABLED_OVERRIDABLE,
  CAPTIVE_PORTAL_PROBE_COMPLETED,
  CAPTIVE_PORTAL_PROBE_COMPLETED_OVERRIDABLE,
  CAPTIVE_PORTAL_NO_RESPONSE,
  CAPTIVE_PORTAL_NO_RESPONSE_OVERRIDABLE,
  CAPTIVE_PORTAL_DETECTED,
  CAPTIVE_PORTAL_DETECTED_OVERRIDABLE,
  UNUSED_BLOCKING_PAGE_EVENT,
};

void RecordSSLBlockingPageEventStats(SSLBlockingPageEvent event) {
  UMA_HISTOGRAM_ENUMERATION("interstitial.ssl",
                            event,
                            UNUSED_BLOCKING_PAGE_EVENT);
}

void RecordSSLBlockingPageDetailedStats(
    bool proceed,
    int cert_error,
    bool overridable,
    bool internal,
    int num_visits,
    bool captive_portal_detection_enabled,
    bool captive_portal_probe_completed,
    bool captive_portal_no_response,
    bool captive_portal_detected) {
  UMA_HISTOGRAM_ENUMERATION("interstitial.ssl_error_type",
      SSLErrorInfo::NetErrorToErrorType(cert_error), SSLErrorInfo::END_OF_ENUM);
#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
  if (captive_portal_detection_enabled)
    RecordSSLBlockingPageEventStats(
        overridable ?
        CAPTIVE_PORTAL_DETECTION_ENABLED_OVERRIDABLE :
        CAPTIVE_PORTAL_DETECTION_ENABLED);
  if (captive_portal_probe_completed)
    RecordSSLBlockingPageEventStats(
        overridable ?
        CAPTIVE_PORTAL_PROBE_COMPLETED_OVERRIDABLE :
        CAPTIVE_PORTAL_PROBE_COMPLETED);
  // Log only one of portal detected and no response results.
  if (captive_portal_detected)
    RecordSSLBlockingPageEventStats(
        overridable ?
        CAPTIVE_PORTAL_DETECTED_OVERRIDABLE :
        CAPTIVE_PORTAL_DETECTED);
  else if (captive_portal_no_response)
    RecordSSLBlockingPageEventStats(
        overridable ?
        CAPTIVE_PORTAL_NO_RESPONSE_OVERRIDABLE :
        CAPTIVE_PORTAL_NO_RESPONSE);
#endif
  if (!overridable) {
    if (proceed) {
      RecordSSLBlockingPageEventStats(PROCEED_MANUAL_NONOVERRIDABLE);
    }
    // Overridable is false if the user didn't have any option except to turn
    // back. If that's the case, don't record some of the metrics.
    return;
  }
  if (num_visits == 0)
    RecordSSLBlockingPageEventStats(SHOW_NEW_SITE);
  if (proceed) {
    RecordSSLBlockingPageEventStats(PROCEED_OVERRIDABLE);
    if (internal)
      RecordSSLBlockingPageEventStats(PROCEED_INTERNAL_HOSTNAME);
    if (num_visits == 0)
      RecordSSLBlockingPageEventStats(PROCEED_NEW_SITE);
  } else if (!proceed) {
    RecordSSLBlockingPageEventStats(DONT_PROCEED_OVERRIDABLE);
  }
  SSLErrorInfo::ErrorType type = SSLErrorInfo::NetErrorToErrorType(cert_error);
  switch (type) {
    case SSLErrorInfo::CERT_COMMON_NAME_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_NAME);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_NAME);
      break;
    }
    case SSLErrorInfo::CERT_DATE_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_DATE);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_DATE);
      break;
    }
    case SSLErrorInfo::CERT_AUTHORITY_INVALID: {
      if (proceed)
        RecordSSLBlockingPageEventStats(PROCEED_AUTHORITY);
      else
        RecordSSLBlockingPageEventStats(DONT_PROCEED_AUTHORITY);
      break;
    }
    default: {
      break;
    }
  }
}

}  // namespace

// Note that we always create a navigation entry with SSL errors.
// No error happening loading a sub-resource triggers an interstitial so far.
SSLBlockingPage::SSLBlockingPage(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool overridable,
    bool strict_enforcement,
    const base::Callback<void(bool)>& callback)
    : callback_(callback),
      web_contents_(web_contents),
      cert_error_(cert_error),
      ssl_info_(ssl_info),
      request_url_(request_url),
      overridable_(overridable),
      strict_enforcement_(strict_enforcement),
      internal_(false),
      num_visits_(-1),
      captive_portal_detection_enabled_(false),
      captive_portal_probe_completed_(false),
      captive_portal_no_response_(false),
      captive_portal_detected_(false) {
  Profile* profile = Profile::FromBrowserContext(
      web_contents->GetBrowserContext());
  // For UMA stats.
  if (net::IsHostnameNonUnique(request_url_.HostNoBrackets()))
    internal_ = true;
  RecordSSLBlockingPageEventStats(SHOW_ALL);
  if (overridable_ && !strict_enforcement_) {
    RecordSSLBlockingPageEventStats(SHOW_OVERRIDABLE);
    if (internal_)
      RecordSSLBlockingPageEventStats(SHOW_INTERNAL_HOSTNAME);
    HistoryService* history_service = HistoryServiceFactory::GetForProfile(
        profile, Profile::EXPLICIT_ACCESS);
    if (history_service) {
      history_service->GetVisibleVisitCountToHost(
          request_url_,
          &request_consumer_,
          base::Bind(&SSLBlockingPage::OnGotHistoryCount,
                    base::Unretained(this)));
    }
  }

#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
  CaptivePortalService* captive_portal_service =
      CaptivePortalServiceFactory::GetForProfile(profile);
  captive_portal_detection_enabled_ = captive_portal_service ->enabled();
  captive_portal_service ->DetectCaptivePortal();
  registrar_.Add(this,
                 chrome::NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT,
                 content::Source<Profile>(profile));
#endif

  interstitial_page_ = InterstitialPage::Create(
      web_contents_, true, request_url, this);
  interstitial_page_->Show();
}

SSLBlockingPage::~SSLBlockingPage() {
  if (!callback_.is_null()) {
    RecordSSLBlockingPageDetailedStats(false,
                                       cert_error_,
                                       overridable_ && !strict_enforcement_,
                                       internal_,
                                       num_visits_,
                                       captive_portal_detection_enabled_,
                                       captive_portal_probe_completed_,
                                       captive_portal_no_response_,
                                       captive_portal_detected_);
    // The page is closed without the user having chosen what to do, default to
    // deny.
    NotifyDenyCertificate();
  }
}

std::string SSLBlockingPage::GetHTMLContents() {
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSSLInterstitialVersionV1) ||
      base::FieldTrialList::FindFullName("SSLInterstitialVersion") == "V1") {
    return GetHTMLContentsV1();
  }
  return GetHTMLContentsV2();
}

std::string SSLBlockingPage::GetHTMLContentsV1() {
  base::DictionaryValue strings;
  int resource_id;
  if (overridable_ && !strict_enforcement_) {
    // Let's build the overridable error page.
    SSLErrorInfo error_info =
        SSLErrorInfo::CreateError(
            SSLErrorInfo::NetErrorToErrorType(cert_error_),
            ssl_info_.cert.get(),
            request_url_);

    resource_id = IDR_SSL_ROAD_BLOCK_HTML;
    strings.SetString("headLine", error_info.title());
    strings.SetString("description", error_info.details());
    strings.SetString("moreInfoTitle",
        l10n_util::GetStringUTF16(IDS_CERT_ERROR_EXTRA_INFO_TITLE));
    SetExtraInfo(&strings, error_info.extra_information());

    strings.SetString(
        "exit", l10n_util::GetStringUTF16(IDS_SSL_OVERRIDABLE_PAGE_EXIT));
    strings.SetString(
        "title", l10n_util::GetStringUTF16(IDS_SSL_OVERRIDABLE_PAGE_TITLE));
    strings.SetString(
        "proceed", l10n_util::GetStringUTF16(IDS_SSL_OVERRIDABLE_PAGE_PROCEED));
    strings.SetString(
        "reasonForNotProceeding", l10n_util::GetStringUTF16(
            IDS_SSL_OVERRIDABLE_PAGE_SHOULD_NOT_PROCEED));
    strings.SetString("errorType", "overridable");
    strings.SetString("textdirection", base::i18n::IsRTL() ? "rtl" : "ltr");
  } else {
    // Let's build the blocking error page.
    resource_id = IDR_SSL_BLOCKING_HTML;

    // Strings that are not dependent on the URL.
    strings.SetString(
        "title", l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_TITLE));
    strings.SetString(
        "reloadMsg", l10n_util::GetStringUTF16(IDS_ERRORPAGES_BUTTON_RELOAD));
    strings.SetString(
        "more", l10n_util::GetStringUTF16(IDS_ERRORPAGES_BUTTON_MORE));
    strings.SetString(
        "less", l10n_util::GetStringUTF16(IDS_ERRORPAGES_BUTTON_LESS));
    strings.SetString(
        "moreTitle",
        l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_MORE_TITLE));
    strings.SetString(
        "techTitle",
        l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_TECH_TITLE));

    // Strings that are dependent on the URL.
    base::string16 url(ASCIIToUTF16(request_url_.host()));
    bool rtl = base::i18n::IsRTL();
    strings.SetString("textDirection", rtl ? "rtl" : "ltr");
    if (rtl)
      base::i18n::WrapStringWithLTRFormatting(&url);
    strings.SetString(
        "headline", l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_HEADLINE,
                                               url.c_str()));
    strings.SetString(
        "message", l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_BODY_TEXT,
                                              url.c_str()));
    strings.SetString(
        "moreMessage",
        l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_MORE_TEXT,
                                   url.c_str()));
    strings.SetString("reloadUrl", request_url_.spec());

    // Strings that are dependent on the error type.
    SSLErrorInfo::ErrorType type =
        SSLErrorInfo::NetErrorToErrorType(cert_error_);
    base::string16 errorType;
    if (type == SSLErrorInfo::CERT_REVOKED) {
      errorType = base::string16(ASCIIToUTF16("Key revocation"));
      strings.SetString(
          "failure",
          l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_REVOKED));
    } else if (type == SSLErrorInfo::CERT_INVALID) {
      errorType = base::string16(ASCIIToUTF16("Malformed certificate"));
      strings.SetString(
          "failure",
          l10n_util::GetStringUTF16(IDS_SSL_BLOCKING_PAGE_FORMATTED));
    } else if (type == SSLErrorInfo::CERT_PINNED_KEY_MISSING) {
      errorType = base::string16(ASCIIToUTF16("Certificate pinning failure"));
      strings.SetString(
          "failure",
          l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_PINNING,
                                     url.c_str()));
    } else if (type == SSLErrorInfo::CERT_WEAK_KEY_DH) {
      errorType = base::string16(ASCIIToUTF16("Weak DH public key"));
      strings.SetString(
          "failure",
          l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_WEAK_DH,
                                     url.c_str()));
    } else {
      // HSTS failure.
      errorType = base::string16(ASCIIToUTF16("HSTS failure"));
      strings.SetString(
          "failure",
          l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_HSTS, url.c_str()));
    }
    if (rtl)
      base::i18n::WrapStringWithLTRFormatting(&errorType);
    strings.SetString(
        "errorType", l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_ERROR,
                                                errorType.c_str()));

    // Strings that display the invalid cert.
    base::string16 subject(
        ASCIIToUTF16(ssl_info_.cert->subject().GetDisplayName()));
    base::string16 issuer(
        ASCIIToUTF16(ssl_info_.cert->issuer().GetDisplayName()));
    std::string hashes;
    for (std::vector<net::HashValue>::const_iterator it =
            ssl_info_.public_key_hashes.begin();
         it != ssl_info_.public_key_hashes.end();
         ++it) {
      base::StringAppendF(&hashes, "%s ", it->ToString().c_str());
    }
    base::string16 fingerprint(ASCIIToUTF16(hashes));
    if (rtl) {
      // These are always going to be LTR.
      base::i18n::WrapStringWithLTRFormatting(&subject);
      base::i18n::WrapStringWithLTRFormatting(&issuer);
      base::i18n::WrapStringWithLTRFormatting(&fingerprint);
    }
    strings.SetString(
        "subject", l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_SUBJECT,
                                              subject.c_str()));
    strings.SetString(
        "issuer", l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_ISSUER,
                                             issuer.c_str()));
    strings.SetString(
        "fingerprint",
        l10n_util::GetStringFUTF16(IDS_SSL_BLOCKING_PAGE_HASHES,
                                   fingerprint.c_str()));
  }

  base::StringPiece html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          resource_id));
  return webui::GetI18nTemplateHtml(html, &strings);
}

std::string SSLBlockingPage::GetHTMLContentsV2() {
  base::DictionaryValue load_time_data;
  base::string16 url(ASCIIToUTF16(request_url_.host()));
  if (base::i18n::IsRTL())
    base::i18n::WrapStringWithLTRFormatting(&url);
  webui::SetFontAndTextDirection(&load_time_data);

  // Shared values for both the overridable and non-overridable versions.
  load_time_data.SetBoolean("ssl", true);
  load_time_data.SetBoolean(
      "overridable", overridable_ && !strict_enforcement_);
  load_time_data.SetString(
      "tabTitle", l10n_util::GetStringUTF16(IDS_SSL_V2_TITLE));
  load_time_data.SetString(
      "heading", l10n_util::GetStringUTF16(IDS_SSL_V2_HEADING));
  load_time_data.SetString(
      "primaryParagraph",
      l10n_util::GetStringFUTF16(IDS_SSL_V2_PRIMARY_PARAGRAPH, url));
  load_time_data.SetString(
     "openDetails",
     l10n_util::GetStringUTF16(IDS_SSL_V2_OPEN_DETAILS_BUTTON));
  load_time_data.SetString(
     "closeDetails",
     l10n_util::GetStringUTF16(IDS_SSL_V2_CLOSE_DETAILS_BUTTON));

  if (overridable_ && !strict_enforcement_) {  // Overridable.
    SSLErrorInfo error_info =
        SSLErrorInfo::CreateError(
            SSLErrorInfo::NetErrorToErrorType(cert_error_),
            ssl_info_.cert.get(),
            request_url_);
    load_time_data.SetString(
        "explanationParagraph", error_info.details());
    load_time_data.SetString(
        "primaryButtonText",
        l10n_util::GetStringUTF16(IDS_SSL_OVERRIDABLE_SAFETY_BUTTON));
    load_time_data.SetString(
        "finalParagraph",
        l10n_util::GetStringFUTF16(IDS_SSL_OVERRIDABLE_PROCEED_PARAGRAPH, url));
  } else {  // Non-overridable.
    load_time_data.SetBoolean("overridable", false);
    load_time_data.SetString(
        "explanationParagraph",
        l10n_util::GetStringFUTF16(IDS_SSL_NONOVERRIDABLE_MORE, url));
    load_time_data.SetString(
        "primaryButtonText",
        l10n_util::GetStringUTF16(IDS_SSL_NONOVERRIDABLE_RELOAD_BUTTON));
    // Customize the help link depending on the specific error type.
    // Only mark as HSTS if none of the more specific error types apply, and use
    // INVALID as a fallback if no other string is appropriate.
    SSLErrorInfo::ErrorType type =
        SSLErrorInfo::NetErrorToErrorType(cert_error_);
    load_time_data.SetInteger("errorType", type);
    int help_string = IDS_SSL_NONOVERRIDABLE_INVALID;
    switch (type) {
      case SSLErrorInfo::CERT_REVOKED:
        help_string = IDS_SSL_NONOVERRIDABLE_REVOKED;
        break;
      case SSLErrorInfo::CERT_PINNED_KEY_MISSING:
        help_string = IDS_SSL_NONOVERRIDABLE_PINNED;
        break;
      case SSLErrorInfo::CERT_INVALID:
        help_string = IDS_SSL_NONOVERRIDABLE_INVALID;
        break;
      default:
        if (strict_enforcement_)
          help_string = IDS_SSL_NONOVERRIDABLE_HSTS;
    }
    load_time_data.SetString(
        "finalParagraph", l10n_util::GetStringFUTF16(help_string, url));
    load_time_data.SetString("errorCode", net::ErrorToString(cert_error_));
  }

  base::StringPiece html(
     ResourceBundle::GetSharedInstance().GetRawDataResource(
         IRD_SSL_INTERSTITIAL_V2_HTML));
  webui::UseVersion2 version;
  return webui::GetI18nTemplateHtml(html, &load_time_data);
}

void SSLBlockingPage::OverrideEntry(NavigationEntry* entry) {
  int cert_id = content::CertStore::GetInstance()->StoreCert(
      ssl_info_.cert.get(), web_contents_->GetRenderProcessHost()->GetID());
  DCHECK(cert_id);

  entry->GetSSL().security_style =
      content::SECURITY_STYLE_AUTHENTICATION_BROKEN;
  entry->GetSSL().cert_id = cert_id;
  entry->GetSSL().cert_status = ssl_info_.cert_status;
  entry->GetSSL().security_bits = ssl_info_.security_bits;
}

// This handles the commands sent from the interstitial JavaScript. They are
// defined in chrome/browser/resources/ssl/ssl_errors_common.js.
// DO NOT reorder or change this logic without also changing the JavaScript!
void SSLBlockingPage::CommandReceived(const std::string& command) {
  int cmd = 0;
  bool retval = base::StringToInt(command, &cmd);
  DCHECK(retval);
  switch (cmd) {
    case CMD_DONT_PROCEED: {
      interstitial_page_->DontProceed();
      break;
    }
    case CMD_PROCEED: {
      interstitial_page_->Proceed();
      break;
    }
    case CMD_MORE: {
      RecordSSLBlockingPageEventStats(MORE);
      break;
    }
    case CMD_RELOAD: {
      // The interstitial can't refresh itself.
      web_contents_->GetController().Reload(true);
      break;
    }
    case CMD_HELP: {
      // The interstitial can't open a popup or navigate itself.
      // TODO(felt): We're going to need a new help page.
      content::NavigationController::LoadURLParams help_page_params(GURL(
          "https://support.google.com/chrome/answer/4454607"));
      web_contents_->GetController().LoadURLWithParams(help_page_params);
      break;
    }
    default: {
      NOTREACHED();
    }
  }
}

void SSLBlockingPage::OverrideRendererPrefs(
      content::RendererPreferences* prefs) {
  Profile* profile = Profile::FromBrowserContext(
      web_contents_->GetBrowserContext());
  renderer_preferences_util::UpdateFromSystemSettings(prefs, profile);
}

void SSLBlockingPage::OnProceed() {
  RecordSSLBlockingPageDetailedStats(true,
                                     cert_error_,
                                     overridable_ && !strict_enforcement_,
                                     internal_,
                                     num_visits_,
                                     captive_portal_detection_enabled_,
                                     captive_portal_probe_completed_,
                                     captive_portal_no_response_,
                                     captive_portal_detected_);
  // Accepting the certificate resumes the loading of the page.
  NotifyAllowCertificate();
}

void SSLBlockingPage::OnDontProceed() {
  RecordSSLBlockingPageDetailedStats(false,
                                     cert_error_,
                                     overridable_ && !strict_enforcement_,
                                     internal_,
                                     num_visits_,
                                     captive_portal_detection_enabled_,
                                     captive_portal_probe_completed_,
                                     captive_portal_no_response_,
                                     captive_portal_detected_);
  NotifyDenyCertificate();
}

void SSLBlockingPage::NotifyDenyCertificate() {
  // It's possible that callback_ may not exist if the user clicks "Proceed"
  // followed by pressing the back button before the interstitial is hidden.
  // In that case the certificate will still be treated as allowed.
  if (callback_.is_null())
    return;

  callback_.Run(false);
  callback_.Reset();
}

void SSLBlockingPage::NotifyAllowCertificate() {
  DCHECK(!callback_.is_null());

  callback_.Run(true);
  callback_.Reset();
}

// static
void SSLBlockingPage::SetExtraInfo(
    base::DictionaryValue* strings,
    const std::vector<base::string16>& extra_info) {
  DCHECK_LT(extra_info.size(), 5U);  // We allow 5 paragraphs max.
  const char* keys[5] = {
      "moreInfo1", "moreInfo2", "moreInfo3", "moreInfo4", "moreInfo5"
  };
  int i;
  for (i = 0; i < static_cast<int>(extra_info.size()); i++) {
    strings->SetString(keys[i], extra_info[i]);
  }
  for (; i < 5; i++) {
    strings->SetString(keys[i], std::string());
  }
}

void SSLBlockingPage::OnGotHistoryCount(HistoryService::Handle handle,
                                        bool success,
                                        int num_visits,
                                        base::Time first_visit) {
  num_visits_ = num_visits;
}

void SSLBlockingPage::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
  // When detection is disabled, captive portal service always sends
  // RESULT_INTERNET_CONNECTED. Ignore any probe results in that case.
  if (!captive_portal_detection_enabled_)
    return;
  if (type == chrome::NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT) {
    captive_portal_probe_completed_ = true;
    CaptivePortalService::Results* results =
        content::Details<CaptivePortalService::Results>(
            details).ptr();
    // If a captive portal was detected at any point when the interstitial was
    // displayed, assume that the interstitial was caused by a captive portal.
    // Example scenario:
    // 1- Interstitial displayed and captive portal detected, setting the flag.
    // 2- Captive portal detection automatically opens portal login page.
    // 3- User logs in on the portal login page.
    // A notification will be received here for RESULT_INTERNET_CONNECTED. Make
    // sure we don't clear the captive portal flag, since the interstitial was
    // potentially caused by the captive portal.
    captive_portal_detected_ = captive_portal_detected_ ||
        (results->result == captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL);
    // Also keep track of non-HTTP portals and error cases.
    captive_portal_no_response_ = captive_portal_no_response_ ||
        (results->result == captive_portal::RESULT_NO_RESPONSE);
  }
#endif
}
