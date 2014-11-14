// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/gcd_private/gcd_private_api.h"

#include "base/lazy_instance.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/local_discovery/cloud_device_list.h"
#include "chrome/browser/local_discovery/cloud_print_printer_list.h"
#include "chrome/browser/local_discovery/gcd_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_manager_base.h"

namespace extensions {

using extensions::api::gcd_private::GCDDevice;

namespace {

const int kNumRequestsNeeded = 2;

const char kIDPrefixCloudPrinter[] = "cloudprint:";
const char kIDPrefixGcd[] = "gcd:";

GcdPrivateAPI::GCDApiFlowFactoryForTests* g_gcd_api_flow_factory = NULL;

base::LazyInstance<BrowserContextKeyedAPIFactory<GcdPrivateAPI> > g_factory =
    LAZY_INSTANCE_INITIALIZER;

scoped_ptr<local_discovery::GCDApiFlow> MakeGCDApiFlow(Profile* profile) {
  if (g_gcd_api_flow_factory) {
    return g_gcd_api_flow_factory->CreateGCDApiFlow();
  }

  ProfileOAuth2TokenService* token_service =
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile);
  if (!token_service)
    return scoped_ptr<local_discovery::GCDApiFlow>();
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetInstance()->GetForProfile(profile);
  if (!signin_manager)
    return scoped_ptr<local_discovery::GCDApiFlow>();
  return local_discovery::GCDApiFlow::Create(
      profile->GetRequestContext(),
      token_service,
      signin_manager->GetAuthenticatedAccountId());
}

}  // namespace

GcdPrivateAPI::GcdPrivateAPI(content::BrowserContext* context)
    : browser_context_(context) {
}

GcdPrivateAPI::~GcdPrivateAPI() {
}

// static
BrowserContextKeyedAPIFactory<GcdPrivateAPI>*
GcdPrivateAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
void GcdPrivateAPI::SetGCDApiFlowFactoryForTests(
    GCDApiFlowFactoryForTests* factory) {
  g_gcd_api_flow_factory = factory;
}

GcdPrivateGetCloudDeviceListFunction::GcdPrivateGetCloudDeviceListFunction() {
}
GcdPrivateGetCloudDeviceListFunction::~GcdPrivateGetCloudDeviceListFunction() {
}

bool GcdPrivateGetCloudDeviceListFunction::RunAsync() {
  requests_succeeded_ = 0;
  requests_failed_ = 0;

  printer_list_ = MakeGCDApiFlow(GetProfile());
  device_list_ = MakeGCDApiFlow(GetProfile());

  if (!printer_list_ || !device_list_)
    return false;

  // Balanced in CheckListingDone()
  AddRef();

  printer_list_->Start(make_scoped_ptr<local_discovery::GCDApiFlow::Request>(
      new local_discovery::CloudPrintPrinterList(this)));
  device_list_->Start(make_scoped_ptr<local_discovery::GCDApiFlow::Request>(
      new local_discovery::CloudDeviceList(this)));

  return true;
}

void GcdPrivateGetCloudDeviceListFunction::OnDeviceListReady(
    const DeviceList& devices) {
  requests_succeeded_++;

  devices_.insert(devices_.end(), devices.begin(), devices.end());

  CheckListingDone();
}

void GcdPrivateGetCloudDeviceListFunction::OnDeviceListUnavailable() {
  requests_failed_++;

  CheckListingDone();
}

void GcdPrivateGetCloudDeviceListFunction::CheckListingDone() {
  if (requests_failed_ + requests_succeeded_ != kNumRequestsNeeded)
    return;

  if (requests_succeeded_ == 0) {
    SendResponse(false);
    return;
  }

  std::vector<linked_ptr<GCDDevice> > devices;

  for (DeviceList::iterator i = devices_.begin(); i != devices_.end(); i++) {
    linked_ptr<GCDDevice> device(new GCDDevice);
    device->setup_type = extensions::api::gcd_private::SETUP_TYPE_CLOUD;
    if (i->type == local_discovery::kGCDTypePrinter) {
      device->id_string = kIDPrefixCloudPrinter + i->id;
    } else {
      device->id_string = kIDPrefixGcd + i->id;
    }

    device->cloud_id.reset(new std::string(i->id));
    device->device_type = i->type;
    device->device_name = i->display_name;
    device->device_description = i->description;

    devices.push_back(device);
  }

  results_ = extensions::api::gcd_private::GetCloudDeviceList::Results::Create(
      devices);

  SendResponse(true);
  Release();
}

}  // namespace extensions
