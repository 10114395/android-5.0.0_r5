// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_ENGINE_MODEL_TYPE_REGISTRY_H_
#define SYNC_ENGINE_MODEL_TYPE_REGISTRY_H_

#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "sync/base/sync_export.h"
#include "sync/internal_api/public/base/model_type.h"
#include "sync/internal_api/public/engine/model_safe_worker.h"
#include "sync/internal_api/public/sessions/type_debug_info_observer.h"

namespace syncer {

namespace syncable {
class Directory;
}  // namespace syncable

class CommitContributor;
class DirectoryCommitContributor;
class DirectoryUpdateHandler;
class DirectoryTypeDebugInfoEmitter;
class NonBlockingTypeProcessorCore;
class NonBlockingTypeProcessor;
class UpdateHandler;
struct DataTypeState;

typedef std::map<ModelType, UpdateHandler*> UpdateHandlerMap;
typedef std::map<ModelType, CommitContributor*> CommitContributorMap;
typedef std::map<ModelType, DirectoryTypeDebugInfoEmitter*>
    DirectoryTypeDebugInfoEmitterMap;

// Keeps track of the sets of active update handlers and commit contributors.
class SYNC_EXPORT_PRIVATE ModelTypeRegistry {
 public:
  // This alternative constructor does not support any directory types.
  // It is used only in tests.
  ModelTypeRegistry();

  // Constructs a ModelTypeRegistry that supports directory types.
  ModelTypeRegistry(
      const std::vector<scoped_refptr<ModelSafeWorker> >& workers,
      syncable::Directory* directory);
  ~ModelTypeRegistry();

  // Sets the set of enabled types.
  void SetEnabledDirectoryTypes(const ModelSafeRoutingInfo& routing_info);

  // Enables an off-thread type for syncing.  Connects the given processor
  // and its task_runner to the newly created processor core.
  //
  // Expects that the processor's ModelType is not currently enabled.
  void InitializeNonBlockingType(
      syncer::ModelType type,
      const DataTypeState& data_type_state,
      scoped_refptr<base::SequencedTaskRunner> type_task_runner,
      base::WeakPtr<NonBlockingTypeProcessor> processor);

  // Disables the syncing of an off-thread type.
  //
  // Expects that the type is currently enabled.
  // Deletes the processor core associated with the type.
  void RemoveNonBlockingType(syncer::ModelType type);

  // Gets the set of enabled types.
  ModelTypeSet GetEnabledTypes() const;

  // Simple getters.
  UpdateHandlerMap* update_handler_map();
  CommitContributorMap* commit_contributor_map();
  DirectoryTypeDebugInfoEmitterMap* directory_type_debug_info_emitter_map();

  void RegisterDirectoryTypeDebugInfoObserver(
      syncer::TypeDebugInfoObserver* observer);
  void UnregisterDirectoryTypeDebugInfoObserver(
      syncer::TypeDebugInfoObserver* observer);
  bool HasDirectoryTypeDebugInfoObserver(
      syncer::TypeDebugInfoObserver* observer);
  void RequestEmitDebugInfo();

 private:
  ModelTypeSet GetEnabledNonBlockingTypes() const;
  ModelTypeSet GetEnabledDirectoryTypes() const;

  // Sets of handlers and contributors.
  ScopedVector<DirectoryCommitContributor> directory_commit_contributors_;
  ScopedVector<DirectoryUpdateHandler> directory_update_handlers_;
  ScopedVector<DirectoryTypeDebugInfoEmitter>
      directory_type_debug_info_emitters_;

  ScopedVector<NonBlockingTypeProcessorCore> non_blocking_type_processor_cores_;

  // Maps of UpdateHandlers and CommitContributors.
  // They do not own any of the objects they point to.
  UpdateHandlerMap update_handler_map_;
  CommitContributorMap commit_contributor_map_;

  // Map of DebugInfoEmitters for directory types.
  // Non-blocking types handle debug info differently.
  // Does not own its contents.
  DirectoryTypeDebugInfoEmitterMap directory_type_debug_info_emitter_map_;

  // The known ModelSafeWorkers.
  std::map<ModelSafeGroup, scoped_refptr<ModelSafeWorker> > workers_map_;

  // The directory.  Not owned.
  syncable::Directory* directory_;

  // The set of enabled directory types.
  ModelTypeSet enabled_directory_types_;

  // The set of observers of per-type debug info.
  //
  // Each of the DirectoryTypeDebugInfoEmitters needs such a list.  There's
  // a lot of them, and their lifetimes are unpredictable, so it makes the
  // book-keeping easier if we just store the list here.  That way it's
  // guaranteed to live as long as this sync backend.
  ObserverList<TypeDebugInfoObserver> type_debug_info_observers_;

  DISALLOW_COPY_AND_ASSIGN(ModelTypeRegistry);
};

}  // namespace syncer

#endif // SYNC_ENGINE_MODEL_TYPE_REGISTRY_H_

