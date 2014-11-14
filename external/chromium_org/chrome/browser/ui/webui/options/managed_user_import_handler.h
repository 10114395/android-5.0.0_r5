// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_IMPORT_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_IMPORT_HANDLER_H_

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/supervised_user/supervised_user_sync_service_observer.h"
#include "chrome/browser/ui/webui/options/options_ui.h"
#include "components/signin/core/browser/signin_error_controller.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace options {

// Handler for the 'import existing managed user' dialog.
class ManagedUserImportHandler : public OptionsPageUIHandler,
                                 public SupervisedUserSyncServiceObserver,
                                 public SigninErrorController::Observer {
 public:
  typedef base::CallbackList<void(const std::string&, const std::string&)>
      CallbackList;

  ManagedUserImportHandler();
  virtual ~ManagedUserImportHandler();

  // OptionsPageUIHandler implementation.
  virtual void GetLocalizedValues(
      base::DictionaryValue* localized_strings) OVERRIDE;
  virtual void InitializeHandler() OVERRIDE;

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages() OVERRIDE;

  // SupervisedUserSyncServiceObserver implementation.
  virtual void OnSupervisedUserAcknowledged(
      const std::string& supervised_user_id) OVERRIDE {}
  virtual void OnSupervisedUsersSyncingStopped() OVERRIDE {}
  virtual void OnSupervisedUsersChanged() OVERRIDE;

  // SigninErrorController::Observer implementation.
  virtual void OnErrorChanged() OVERRIDE;

 private:
  // Clears the cached list of managed users and fetches the new list of managed
  // users.
  void FetchManagedUsers();

  // Callback for the "requestManagedUserImportUpdate" message.
  // Checks the sign-in status of the custodian and accordingly
  // sends an update to the WebUI. The update can be to show/hide
  // an error bubble and update/clear the managed user list.
  void RequestManagedUserImportUpdate(const base::ListValue* args);

  // Sends an array of managed users to WebUI. Each entry is of the form:
  //   managedProfile = {
  //     id: "Managed User ID",
  //     name: "Managed User Name",
  //     iconURL: "chrome://path/to/icon/image",
  //     onCurrentDevice: true or false,
  //     needAvatar: true or false
  //   }
  // The array holds all existing managed users attached to the
  // custodian's profile which initiated the request.
  void SendExistingManagedUsers(const base::DictionaryValue* dict);

  // Sends messages to the JS side to clear managed users and show an error
  // bubble.
  void ClearManagedUsersAndShowError();

  bool IsAccountConnected() const;
  bool HasAuthError() const;

  // Called when a managed user shared setting is changed. If the avatar was
  // changed, FetchManagedUsers() is called.
  void OnSharedSettingChanged(const std::string& managed_user_id,
                              const std::string& key);

  scoped_ptr<CallbackList::Subscription> subscription_;

  ScopedObserver<SigninErrorController, ManagedUserImportHandler> observer_;

  base::WeakPtrFactory<ManagedUserImportHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ManagedUserImportHandler);
};

}  // namespace options

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS_MANAGED_USER_IMPORT_HANDLER_H_
