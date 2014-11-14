// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PAIRING_CONTROLLER_PAIRING_FLOW_H_
#define CHROMEOS_PAIRING_CONTROLLER_PAIRING_FLOW_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {
class UserContext;
}

namespace content {
class BrowserContext;
}

namespace chromeos {

class CHROMEOS_EXPORT ControllerPairingFlow {
 public:
  enum Stage {
    STAGE_NONE,
    STAGE_DEVICES_DISCOVERY,
    STAGE_DEVICE_NOT_FOUND,
    STAGE_ESTABLISHING_CONNECTION,
    STAGE_ESTABLISHING_CONNECTION_ERROR,
    STAGE_WAITING_FOR_CODE_CONFIRMATION,
    STAGE_HOST_UPDATE_IN_PROGRESS,
    STAGE_HOST_CONNECTION_LOST,
    STAGE_WAITING_FOR_CREDENTIALS,
    STAGE_HOST_ENROLLMENT_IN_PROGRESS,
    STAGE_HOST_ENROLLMENT_ERROR,
    STAGE_PAIRING_DONE,
    STAGE_FINISHED
  };

  class Observer {
   public:
    Observer();
    virtual ~Observer();

    // Called when flow has moved on from one stage to another.
    virtual void PairingStageChanged(Stage new_stage) = 0;

    // Called when new device was discovered or existing device was lost.
    // This notification is made only on |STAGE_SCANNING_FOR_DEVICES| stage.
    virtual void DiscoveredDevicesListChanged() = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Observer);
  };

  typedef std::vector<std::string> DeviceIdList;

  ControllerPairingFlow();
  virtual ~ControllerPairingFlow();

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Returns current stage of flow.
  virtual Stage GetCurrentStage() = 0;

  // Starts pairing flow. Can be called only on |STAGE_NONE| stage.
  virtual void StartFlow() = 0;

  // Returns list of discovered devices. Can be called only on
  // |STAGE_DEVICES_DISCOVERY| stage.
  virtual DeviceIdList GetDiscoveredDevices() = 0;

  // This method is called to start pairing with the device having |device_id|
  // ID. Can be called only on |STAGE_DEVICES_DISCOVERY| stage.
  virtual void ChooseDeviceForPairing(const std::string& device_id) = 0;

  // Rescan for devices to pair with. Can be called only on
  // |STAGE_DEVICE_NOT_FOUND| stage.
  virtual void RepeatDiscovery() = 0;

  // Returns pairing confirmation code.
  // Could be called only on |STATE_WAITING_FOR_CODE_CONFIRMATION| stage.
  virtual std::string GetConfirmationCode() = 0;

  // Called to confirm or deny confirmation code. Can be called only on
  // |STAGE_WAITING_FOR_CODE_CONFIRMATION| stage.
  virtual void SetConfirmationCodeIsCorrect(bool correct) = 0;

  // Called when user successfully authenticated on GAIA page. Can be called
  // only on |STAGE_WAITING_FOR_CREDENTIALS| stage.
  virtual void OnAuthenticationDone(
      const chromeos::UserContext& user_context,
      content::BrowserContext* browser_context) = 0;

  // Installs app and starts session.
  // Can be called only on |STAGE_PAIRING_DONE| stage.
  virtual void StartSession() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ControllerPairingFlow);
};

}  // namespace chromeos

#endif  // CHROMEOS_PAIRING_CONTROLLER_PAIRING_FLOW_H_
