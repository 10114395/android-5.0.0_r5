// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/sync_core_proxy_impl.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop_proxy.h"
#include "sync/engine/non_blocking_sync_common.h"
#include "sync/internal_api/sync_core.h"

namespace syncer {

SyncCoreProxyImpl::SyncCoreProxyImpl(
    scoped_refptr<base::SequencedTaskRunner> sync_task_runner,
    base::WeakPtr<SyncCore> sync_core)
    : sync_task_runner_(sync_task_runner),
      sync_core_(sync_core) {}

SyncCoreProxyImpl::~SyncCoreProxyImpl() {}

void SyncCoreProxyImpl::ConnectTypeToCore(
    ModelType type,
    const DataTypeState& data_type_state,
    base::WeakPtr<NonBlockingTypeProcessor> type_processor) {
  VLOG(1) << "ConnectTypeToCore: " << ModelTypeToString(type);
  sync_task_runner_->PostTask(FROM_HERE,
                              base::Bind(&SyncCore::ConnectSyncTypeToCore,
                                         sync_core_,
                                         type,
                                         data_type_state,
                                         base::MessageLoopProxy::current(),
                                         type_processor));
}

void SyncCoreProxyImpl::Disconnect(ModelType type) {
  sync_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SyncCore::Disconnect, sync_core_, type));
}

scoped_ptr<SyncCoreProxy> SyncCoreProxyImpl::Clone() const {
  return scoped_ptr<SyncCoreProxy>(
      new SyncCoreProxyImpl(sync_task_runner_, sync_core_));
}

}  // namespace syncer
