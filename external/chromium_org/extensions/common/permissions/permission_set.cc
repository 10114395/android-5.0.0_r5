// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/permissions/permission_set.h"

#include <algorithm>
#include <iterator>
#include <string>

#include "base/strings/stringprintf.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/url_pattern.h"
#include "extensions/common/url_pattern_set.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace extensions {

namespace {

void AddPatternsAndRemovePaths(const URLPatternSet& set, URLPatternSet* out) {
  DCHECK(out);
  for (URLPatternSet::const_iterator i = set.begin(); i != set.end(); ++i) {
    URLPattern p = *i;
    p.SetPath("/*");
    out->AddPattern(p);
  }
}

}  // namespace

//
// PermissionSet
//

PermissionSet::PermissionSet() : should_warn_all_hosts_(UNINITIALIZED) {}

PermissionSet::PermissionSet(
    const APIPermissionSet& apis,
    const ManifestPermissionSet& manifest_permissions,
    const URLPatternSet& explicit_hosts,
    const URLPatternSet& scriptable_hosts)
    : apis_(apis),
      manifest_permissions_(manifest_permissions),
      scriptable_hosts_(scriptable_hosts),
      should_warn_all_hosts_(UNINITIALIZED) {
  AddPatternsAndRemovePaths(explicit_hosts, &explicit_hosts_);
  InitImplicitPermissions();
  InitEffectiveHosts();
}

// static
PermissionSet* PermissionSet::CreateDifference(
    const PermissionSet* set1,
    const PermissionSet* set2) {
  scoped_refptr<PermissionSet> empty = new PermissionSet();
  const PermissionSet* set1_safe = (set1 == NULL) ? empty.get() : set1;
  const PermissionSet* set2_safe = (set2 == NULL) ? empty.get() : set2;

  APIPermissionSet apis;
  APIPermissionSet::Difference(set1_safe->apis(), set2_safe->apis(), &apis);

  ManifestPermissionSet manifest_permissions;
  ManifestPermissionSet::Difference(set1_safe->manifest_permissions(),
                                    set2_safe->manifest_permissions(),
                                    &manifest_permissions);

  URLPatternSet explicit_hosts;
  URLPatternSet::CreateDifference(set1_safe->explicit_hosts(),
                                  set2_safe->explicit_hosts(),
                                  &explicit_hosts);

  URLPatternSet scriptable_hosts;
  URLPatternSet::CreateDifference(set1_safe->scriptable_hosts(),
                                  set2_safe->scriptable_hosts(),
                                  &scriptable_hosts);

  return new PermissionSet(apis, manifest_permissions,
                           explicit_hosts, scriptable_hosts);
}

// static
PermissionSet* PermissionSet::CreateIntersection(
    const PermissionSet* set1,
    const PermissionSet* set2) {
  scoped_refptr<PermissionSet> empty = new PermissionSet();
  const PermissionSet* set1_safe = (set1 == NULL) ? empty.get() : set1;
  const PermissionSet* set2_safe = (set2 == NULL) ? empty.get() : set2;

  APIPermissionSet apis;
  APIPermissionSet::Intersection(set1_safe->apis(), set2_safe->apis(), &apis);

  ManifestPermissionSet manifest_permissions;
  ManifestPermissionSet::Intersection(set1_safe->manifest_permissions(),
                                      set2_safe->manifest_permissions(),
                                      &manifest_permissions);

  URLPatternSet explicit_hosts;
  URLPatternSet::CreateIntersection(set1_safe->explicit_hosts(),
                                    set2_safe->explicit_hosts(),
                                    &explicit_hosts);

  URLPatternSet scriptable_hosts;
  URLPatternSet::CreateIntersection(set1_safe->scriptable_hosts(),
                                    set2_safe->scriptable_hosts(),
                                    &scriptable_hosts);

  return new PermissionSet(apis, manifest_permissions,
                           explicit_hosts, scriptable_hosts);
}

// static
PermissionSet* PermissionSet::CreateUnion(
    const PermissionSet* set1,
    const PermissionSet* set2) {
  scoped_refptr<PermissionSet> empty = new PermissionSet();
  const PermissionSet* set1_safe = (set1 == NULL) ? empty.get() : set1;
  const PermissionSet* set2_safe = (set2 == NULL) ? empty.get() : set2;

  APIPermissionSet apis;
  APIPermissionSet::Union(set1_safe->apis(), set2_safe->apis(), &apis);

  ManifestPermissionSet manifest_permissions;
  ManifestPermissionSet::Union(set1_safe->manifest_permissions(),
                               set2_safe->manifest_permissions(),
                               &manifest_permissions);

  URLPatternSet explicit_hosts;
  URLPatternSet::CreateUnion(set1_safe->explicit_hosts(),
                             set2_safe->explicit_hosts(),
                             &explicit_hosts);

  URLPatternSet scriptable_hosts;
  URLPatternSet::CreateUnion(set1_safe->scriptable_hosts(),
                             set2_safe->scriptable_hosts(),
                             &scriptable_hosts);

  return new PermissionSet(apis, manifest_permissions,
                           explicit_hosts, scriptable_hosts);
}

bool PermissionSet::operator==(
    const PermissionSet& rhs) const {
  return apis_ == rhs.apis_ &&
      manifest_permissions_ == rhs.manifest_permissions_ &&
      scriptable_hosts_ == rhs.scriptable_hosts_ &&
      explicit_hosts_ == rhs.explicit_hosts_;
}

bool PermissionSet::Contains(const PermissionSet& set) const {
  return apis_.Contains(set.apis()) &&
         manifest_permissions_.Contains(set.manifest_permissions()) &&
         explicit_hosts().Contains(set.explicit_hosts()) &&
         scriptable_hosts().Contains(set.scriptable_hosts());
}

std::set<std::string> PermissionSet::GetAPIsAsStrings() const {
  std::set<std::string> apis_str;
  for (APIPermissionSet::const_iterator i = apis_.begin();
       i != apis_.end(); ++i) {
    apis_str.insert(i->name());
  }
  return apis_str;
}

bool PermissionSet::IsEmpty() const {
  // Not default if any host permissions are present.
  if (!(explicit_hosts().is_empty() && scriptable_hosts().is_empty()))
    return false;

  // Or if it has no api permissions.
  return apis().empty() && manifest_permissions().empty();
}

bool PermissionSet::HasAPIPermission(
    APIPermission::ID id) const {
  return apis().find(id) != apis().end();
}

bool PermissionSet::HasAPIPermission(const std::string& permission_name) const {
  const APIPermissionInfo* permission =
      PermissionsInfo::GetInstance()->GetByName(permission_name);
  // Ensure our PermissionsProvider is aware of this permission.
  CHECK(permission) << permission_name;
  return (permission && apis_.count(permission->id()));
}

bool PermissionSet::CheckAPIPermission(APIPermission::ID permission) const {
  return CheckAPIPermissionWithParam(permission, NULL);
}

bool PermissionSet::CheckAPIPermissionWithParam(
    APIPermission::ID permission,
    const APIPermission::CheckParam* param) const {
  APIPermissionSet::const_iterator iter = apis().find(permission);
  if (iter == apis().end())
    return false;
  return iter->Check(param);
}

bool PermissionSet::HasExplicitAccessToOrigin(
    const GURL& origin) const {
  return explicit_hosts().MatchesURL(origin);
}

bool PermissionSet::HasScriptableAccessToURL(
    const GURL& origin) const {
  // We only need to check our host list to verify access. The host list should
  // already reflect any special rules (such as chrome://favicon, all hosts
  // access, etc.).
  return scriptable_hosts().MatchesURL(origin);
}

bool PermissionSet::HasEffectiveAccessToAllHosts() const {
  // There are two ways this set can have effective access to all hosts:
  //  1) it has an <all_urls> URL pattern.
  //  2) it has a named permission with implied full URL access.
  for (URLPatternSet::const_iterator host = effective_hosts().begin();
       host != effective_hosts().end(); ++host) {
    if (host->match_all_urls() ||
        (host->match_subdomains() && host->host().empty()))
      return true;
  }

  for (APIPermissionSet::const_iterator i = apis().begin();
       i != apis().end(); ++i) {
    if (i->info()->implies_full_url_access())
      return true;
  }
  return false;
}

bool PermissionSet::ShouldWarnAllHosts() const {
  if (should_warn_all_hosts_ == UNINITIALIZED)
    InitShouldWarnAllHosts();
  return should_warn_all_hosts_ == WARN_ALL_HOSTS;
}

bool PermissionSet::HasEffectiveAccessToURL(const GURL& url) const {
  return effective_hosts().MatchesURL(url);
}

bool PermissionSet::HasEffectiveFullAccess() const {
  for (APIPermissionSet::const_iterator i = apis().begin();
       i != apis().end(); ++i) {
    if (i->info()->implies_full_access())
      return true;
  }
  return false;
}

PermissionSet::~PermissionSet() {}

void PermissionSet::InitImplicitPermissions() {
  // The downloads permission implies the internal version as well.
  if (apis_.find(APIPermission::kDownloads) != apis_.end())
    apis_.insert(APIPermission::kDownloadsInternal);

  // The fileBrowserHandler permission implies the internal version as well.
  if (apis_.find(APIPermission::kFileBrowserHandler) != apis_.end())
    apis_.insert(APIPermission::kFileBrowserHandlerInternal);
}

void PermissionSet::InitEffectiveHosts() {
  effective_hosts_.ClearPatterns();

  URLPatternSet::CreateUnion(
      explicit_hosts(), scriptable_hosts(), &effective_hosts_);
}

void PermissionSet::InitShouldWarnAllHosts() const {
  if (HasEffectiveAccessToAllHosts()) {
    should_warn_all_hosts_ = WARN_ALL_HOSTS;
    return;
  }

  for (URLPatternSet::const_iterator iter = effective_hosts_.begin();
       iter != effective_hosts_.end();
       ++iter) {
    // If this doesn't even match subdomains, it can't possibly imply all hosts.
    if (!iter->match_subdomains())
      continue;

    // If iter->host() is a recognized TLD, this will be 0. We don't include
    // private TLDs, so that, e.g., *.appspot.com does not imply all hosts.
    size_t registry_length =
        net::registry_controlled_domains::GetRegistryLength(
            iter->host(),
            net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
            net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
    // If there was more than just a TLD in the host (e.g., *.foobar.com), it
    // doesn't imply all hosts.
    if (registry_length > 0)
      continue;

    // At this point the host could either be just a TLD ("com") or some unknown
    // TLD-like string ("notatld"). To disambiguate between them construct a
    // fake URL, and check the registry. This returns 0 if the TLD is
    // unrecognized, or the length of the recognized TLD.
    registry_length = net::registry_controlled_domains::GetRegistryLength(
        base::StringPrintf("foo.%s", iter->host().c_str()),
        net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
        net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
    // If we recognized this TLD, then this is a pattern like *.com, and it
    // should imply all hosts.
    if (registry_length > 0) {
      should_warn_all_hosts_ = WARN_ALL_HOSTS;
      return;
    }
  }

  should_warn_all_hosts_ = DONT_WARN_ALL_HOSTS;
}

}  // namespace extensions
