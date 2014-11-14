// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {
static const char kDomain[] = "a.com";
static const char kSitesDir[] = "automation/sites";
static const char kGotTree[] = "got_tree";
}  // anonymous namespace

class AutomationApiTest : public ExtensionApiTest {
 protected:
  GURL GetURLForPath(const std::string& host, const std::string& path) {
    std::string port = base::IntToString(embedded_test_server()->port());
    GURL::Replacements replacements;
    replacements.SetHostStr(host);
    replacements.SetPortStr(port);
    GURL url =
        embedded_test_server()->GetURL(path).ReplaceComponents(replacements);
    return url;
  }

  void StartEmbeddedTestServer() {
    base::FilePath test_data;
    ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &test_data));
    embedded_test_server()->ServeFilesFromDirectory(
        test_data.AppendASCII("extensions/api_test")
        .AppendASCII(kSitesDir));
    ASSERT_TRUE(ExtensionApiTest::StartEmbeddedTestServer());
    host_resolver()->AddRule("*", embedded_test_server()->base_url().host());
  }

  void LoadPage() {
    StartEmbeddedTestServer();
    const GURL url = GetURLForPath(kDomain, "/index.html");
    ui_test_utils::NavigateToURL(browser(), url);
  }

 public:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
  }
};

IN_PROC_BROWSER_TEST_F(AutomationApiTest, TestRendererAccessibilityEnabled) {
  LoadPage();

  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  content::WebContents* const tab =
      browser()->tab_strip_model()->GetWebContentsAt(0);
  content::RenderWidgetHost* rwh =
      tab->GetRenderWidgetHostView()->GetRenderWidgetHost();
  ASSERT_NE((content::RenderWidgetHost*)NULL, rwh)
      << "Couldn't get RenderWidgetHost";
  ASSERT_FALSE(rwh->IsFullAccessibilityModeForTesting());
  ASSERT_FALSE(rwh->IsTreeOnlyAccessibilityModeForTesting());

  base::FilePath extension_path =
      test_data_dir_.AppendASCII("automation/tests/basic");
  ExtensionTestMessageListener got_tree(kGotTree, false /* no reply */);
  LoadExtension(extension_path);
  ASSERT_TRUE(got_tree.WaitUntilSatisfied());

  rwh = tab->GetRenderWidgetHostView()->GetRenderWidgetHost();
  ASSERT_NE((content::RenderWidgetHost*)NULL, rwh)
      << "Couldn't get RenderWidgetHost";
  ASSERT_FALSE(rwh->IsFullAccessibilityModeForTesting());
  ASSERT_TRUE(rwh->IsTreeOnlyAccessibilityModeForTesting());
}

#if defined(ADDRESS_SANITIZER)
#define Maybe_SanityCheck DISABLED_SanityCheck
#else
#define Maybe_SanityCheck SanityCheck
#endif
IN_PROC_BROWSER_TEST_F(AutomationApiTest, Maybe_SanityCheck) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs", "sanity_check.html"))
      << message_;
}

// Test is failing on ASAN bots, crbug.com/379927
IN_PROC_BROWSER_TEST_F(AutomationApiTest, DISABLED_GetTreeByTabId) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs", "tab_id.html"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AutomationApiTest, Events) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs", "events.html"))
      << message_;
}

#if defined(OS_LINUX) && defined(ADDRESS_SANITIZER)
// Timing out on linux ASan bot: http://crbug.com/385701
#define MAYBE_Actions DISABLED_Actions
#else
#define MAYBE_Actions Actions
#endif

IN_PROC_BROWSER_TEST_F(AutomationApiTest, MAYBE_Actions) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs", "actions.html"))
      << message_;
}

#if defined(ADDRESS_SANITIZER)
#define Maybe_Location DISABLED_Location
#else
#define Maybe_Location Location
#endif
IN_PROC_BROWSER_TEST_F(AutomationApiTest, Maybe_Location) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs", "location.html"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AutomationApiTest, TabsAutomationBooleanPermissions) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest(
          "automation/tests/tabs_automation_boolean", "permissions.html"))
      << message_;
}

// See crbug.com/384673
#if defined(ADDRESS_SANITIZER) || defined(OS_CHROMEOS)
#define Maybe_TabsAutomationBooleanActions DISABLED_TabsAutomationBooleanActions
#else
#define Maybe_TabsAutomationBooleanActions TabsAutomationBooleanActions
#endif
IN_PROC_BROWSER_TEST_F(AutomationApiTest, Maybe_TabsAutomationBooleanActions) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest(
          "automation/tests/tabs_automation_boolean", "actions.html"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AutomationApiTest, TabsAutomationHostsPermissions) {
  StartEmbeddedTestServer();
  ASSERT_TRUE(RunExtensionSubtest(
          "automation/tests/tabs_automation_hosts", "permissions.html"))
      << message_;
}

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(AutomationApiTest, Desktop) {
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/desktop", "desktop.html"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(AutomationApiTest, DesktopNotRequested) {
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/tabs",
                                  "desktop_not_requested.html")) << message_;
}

IN_PROC_BROWSER_TEST_F(AutomationApiTest, DesktopActions) {
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/desktop", "actions.html"))
      << message_;
}
#else
IN_PROC_BROWSER_TEST_F(AutomationApiTest, DesktopNotSupported) {
  ASSERT_TRUE(RunExtensionSubtest("automation/tests/desktop",
                                  "desktop_not_supported.html")) << message_;
}
#endif

}  // namespace extensions
