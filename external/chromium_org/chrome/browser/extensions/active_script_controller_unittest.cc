// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/values.h"
#include "chrome/browser/extensions/active_script_controller.h"
#include "chrome/browser/extensions/active_tab_permission_granter.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/id_util.h"
#include "extensions/common/manifest.h"
#include "extensions/common/value_builder.h"

namespace extensions {

namespace {

const char kAllHostsPermission[] = "*://*/*";

}  // namespace

// Unittests for the ActiveScriptController mostly test the internal logic
// of the controller itself (when to allow/deny extension script injection).
// Testing real injection is allowed/denied as expected (i.e., that the
// ActiveScriptController correctly interfaces in the system) is done in the
// ActiveScriptControllerBrowserTests.
class ActiveScriptControllerUnitTest : public ChromeRenderViewHostTestHarness {
 protected:
  ActiveScriptControllerUnitTest();
  virtual ~ActiveScriptControllerUnitTest();

  // Creates an extension with all hosts permission and adds it to the registry.
  const Extension* AddExtension();

  // Returns the current page id.
  int GetPageId();

  // Returns a closure to use as a script execution for a given extension.
  base::Closure GetExecutionCallbackForExtension(
      const std::string& extension_id);

  // Returns the number of times a given extension has had a script execute.
  size_t GetExecutionCountForExtension(const std::string& extension_id) const;

  ActiveScriptController* controller() { return active_script_controller_; }

 private:
  // Increment the number of executions for the given |extension_id|.
  void IncrementExecutionCount(const std::string& extension_id);

  virtual void SetUp() OVERRIDE;

  // Since ActiveScriptController's behavior is behind a flag, override the
  // feature switch.
  FeatureSwitch::ScopedOverride feature_override_;

  // The associated ActiveScriptController.
  ActiveScriptController* active_script_controller_;

  // The map of observed executions, keyed by extension id.
  std::map<std::string, int> extension_executions_;
};

ActiveScriptControllerUnitTest::ActiveScriptControllerUnitTest()
    : feature_override_(FeatureSwitch::scripts_require_action(),
                        FeatureSwitch::OVERRIDE_ENABLED),
      active_script_controller_(NULL) {
}

ActiveScriptControllerUnitTest::~ActiveScriptControllerUnitTest() {
}

const Extension* ActiveScriptControllerUnitTest::AddExtension() {
  const std::string kId = id_util::GenerateId("all_hosts_extension");
  scoped_refptr<const Extension> extension =
      ExtensionBuilder()
          .SetManifest(
              DictionaryBuilder()
                  .Set("name", "all_hosts_extension")
                  .Set("description", "an extension")
                  .Set("manifest_version", 2)
                  .Set("version", "1.0.0")
                  .Set("permissions",
                       ListBuilder().Append(kAllHostsPermission)))
          .SetLocation(Manifest::INTERNAL)
          .SetID(kId)
          .Build();

  ExtensionRegistry::Get(profile())->AddEnabled(extension);
  return extension;
}

int ActiveScriptControllerUnitTest::GetPageId() {
  content::NavigationEntry* navigation_entry =
      web_contents()->GetController().GetVisibleEntry();
  DCHECK(navigation_entry);  // This should never be NULL.
  return navigation_entry->GetPageID();
}

base::Closure ActiveScriptControllerUnitTest::GetExecutionCallbackForExtension(
    const std::string& extension_id) {
  // We use base unretained here, but if this ever gets executed outside of
  // this test's lifetime, we have a major problem anyway.
  return base::Bind(&ActiveScriptControllerUnitTest::IncrementExecutionCount,
                    base::Unretained(this),
                    extension_id);
}

size_t ActiveScriptControllerUnitTest::GetExecutionCountForExtension(
    const std::string& extension_id) const {
  std::map<std::string, int>::const_iterator iter =
      extension_executions_.find(extension_id);
  if (iter != extension_executions_.end())
    return iter->second;
  return 0u;
}

void ActiveScriptControllerUnitTest::IncrementExecutionCount(
    const std::string& extension_id) {
  ++extension_executions_[extension_id];
}

void ActiveScriptControllerUnitTest::SetUp() {
  ChromeRenderViewHostTestHarness::SetUp();

  TabHelper::CreateForWebContents(web_contents());
  TabHelper* tab_helper = TabHelper::FromWebContents(web_contents());
  // None of these should ever be NULL.
  DCHECK(tab_helper);
  DCHECK(tab_helper->location_bar_controller());
  active_script_controller_ =
      tab_helper->location_bar_controller()->active_script_controller();
  DCHECK(active_script_controller_);
}

// Test that extensions with all_hosts require permission to execute, and, once
// that permission is granted, do execute.
TEST_F(ActiveScriptControllerUnitTest, RequestPermissionAndExecute) {
  const Extension* extension = AddExtension();
  ASSERT_TRUE(extension);

  NavigateAndCommit(GURL("https://www.google.com"));

  // Ensure that there aren't any executions pending.
  ASSERT_EQ(0u, GetExecutionCountForExtension(extension->id()));
  ASSERT_FALSE(controller()->GetActionForExtension(extension));

  // Since the extension requests all_hosts, we should require user consent.
  EXPECT_TRUE(
      controller()->RequiresUserConsentForScriptInjection(extension));

  // Request an injection. There should be an action visible, but no executions.
  controller()->RequestScriptInjection(
      extension,
      GetPageId(),
      GetExecutionCallbackForExtension(extension->id()));
  EXPECT_TRUE(controller()->GetActionForExtension(extension));
  EXPECT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  // Click to accept the extension executing.
  controller()->OnClicked(extension);

  // The extension should execute, and the action should go away.
  EXPECT_EQ(1u, GetExecutionCountForExtension(extension->id()));
  EXPECT_FALSE(controller()->GetActionForExtension(extension));

  // Since we already executed on the given page, we shouldn't need permission
  // for a second time.
  EXPECT_FALSE(
      controller()->RequiresUserConsentForScriptInjection(extension));

  // Reloading should clear those permissions, and we should again require user
  // consent.
  Reload();
  EXPECT_TRUE(
      controller()->RequiresUserConsentForScriptInjection(extension));

  // Grant access.
  controller()->RequestScriptInjection(
      extension,
      GetPageId(),
      GetExecutionCallbackForExtension(extension->id()));
  controller()->OnClicked(extension);
  EXPECT_EQ(2u, GetExecutionCountForExtension(extension->id()));
  EXPECT_FALSE(controller()->GetActionForExtension(extension));

  // Navigating to another site should also clear the permissions.
  NavigateAndCommit(GURL("https://www.foo.com"));
  EXPECT_TRUE(
      controller()->RequiresUserConsentForScriptInjection(extension));
}

// Test that injections that are not executed by the time the user navigates are
// ignored and never execute.
TEST_F(ActiveScriptControllerUnitTest, PendingInjectionsRemovedAtNavigation) {
  const Extension* extension = AddExtension();
  ASSERT_TRUE(extension);

  NavigateAndCommit(GURL("https://www.google.com"));

  ASSERT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  // Request an injection. There should be an action visible, but no executions.
  controller()->RequestScriptInjection(
      extension,
      GetPageId(),
      GetExecutionCallbackForExtension(extension->id()));
  EXPECT_TRUE(controller()->GetActionForExtension(extension));
  EXPECT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  // Reload. This should remove the pending injection, and we should not
  // execute anything.
  Reload();
  EXPECT_FALSE(controller()->GetActionForExtension(extension));
  EXPECT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  // Request and accept a new injection.
  controller()->RequestScriptInjection(
      extension,
      GetPageId(),
      GetExecutionCallbackForExtension(extension->id()));
  controller()->OnClicked(extension);

  // The extension should only have executed once, even though a grand total
  // of two executions were requested.
  EXPECT_EQ(1u, GetExecutionCountForExtension(extension->id()));
  EXPECT_FALSE(controller()->GetActionForExtension(extension));
}

// Test that queueing multiple pending injections, and then accepting, triggers
// them all.
TEST_F(ActiveScriptControllerUnitTest, MultiplePendingInjection) {
  const Extension* extension = AddExtension();
  ASSERT_TRUE(extension);
  NavigateAndCommit(GURL("https://www.google.com"));

  ASSERT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  const size_t kNumInjections = 3u;
  // Queue multiple pending injections.
  for (size_t i = 0u; i < kNumInjections; ++i) {
    controller()->RequestScriptInjection(
        extension,
        GetPageId(),
        GetExecutionCallbackForExtension(extension->id()));
  }
  EXPECT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  controller()->OnClicked(extension);

  // All pending injections should have executed.
  EXPECT_EQ(kNumInjections, GetExecutionCountForExtension(extension->id()));
  EXPECT_FALSE(controller()->GetActionForExtension(extension));
}

TEST_F(ActiveScriptControllerUnitTest, ActiveScriptsUseActiveTabPermissions) {
  const Extension* extension = AddExtension();
  NavigateAndCommit(GURL("https://www.google.com"));

  ActiveTabPermissionGranter* active_tab_permission_granter =
      TabHelper::FromWebContents(web_contents())
          ->active_tab_permission_granter();
  ASSERT_TRUE(active_tab_permission_granter);
  // Grant the extension active tab permissions. This normally happens, e.g.,
  // if the user clicks on a browser action.
  active_tab_permission_granter->GrantIfRequested(extension);

  // Since we have active tab permissions, we shouldn't need user consent
  // anymore.
  EXPECT_FALSE(controller()->RequiresUserConsentForScriptInjection(extension));

  // Also test that granting active tab runs any pending tasks.
  Reload();
  // Navigating should mean we need permission again.
  EXPECT_TRUE(controller()->RequiresUserConsentForScriptInjection(extension));

  controller()->RequestScriptInjection(
      extension,
      GetPageId(),
      GetExecutionCallbackForExtension(extension->id()));
  EXPECT_TRUE(controller()->GetActionForExtension(extension));
  EXPECT_EQ(0u, GetExecutionCountForExtension(extension->id()));

  // Grant active tab.
  active_tab_permission_granter->GrantIfRequested(extension);

  // The pending injections should have run since active tab permission was
  // granted.
  EXPECT_EQ(1u, GetExecutionCountForExtension(extension->id()));
  EXPECT_FALSE(controller()->GetActionForExtension(extension));
}

TEST_F(ActiveScriptControllerUnitTest, ActiveScriptsCanHaveAllUrlsPref) {
  const Extension* extension = AddExtension();
  ASSERT_TRUE(extension);

  NavigateAndCommit(GURL("https://www.google.com"));
  EXPECT_TRUE(controller()->RequiresUserConsentForScriptInjection(extension));

  // Enable the extension on all urls.
  util::SetAllowedScriptingOnAllUrls(extension->id(), profile(), true);

  EXPECT_FALSE(controller()->RequiresUserConsentForScriptInjection(extension));
  // This should carry across navigations, and websites.
  NavigateAndCommit(GURL("http://www.foo.com"));
  EXPECT_FALSE(controller()->RequiresUserConsentForScriptInjection(extension));

  // Turning off the preference should have instant effect.
  util::SetAllowedScriptingOnAllUrls(extension->id(), profile(), false);
  EXPECT_TRUE(controller()->RequiresUserConsentForScriptInjection(extension));

  // And should also persist across navigations and websites.
  NavigateAndCommit(GURL("http://www.bar.com"));
  EXPECT_TRUE(controller()->RequiresUserConsentForScriptInjection(extension));
}

}  // namespace extensions
