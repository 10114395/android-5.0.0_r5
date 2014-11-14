#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from extensions_paths import CHROME_EXTENSIONS
from server_instance import ServerInstance
from test_file_system import TestFileSystem


_TEST_FILESYSTEM = {
  'api': {
    '_api_features.json': json.dumps({
      'audioCapture': {
        'channel': 'stable',
        'extension_types': ['platform_app']
      },
      'background': [
        {
          'channel': 'stable',
          'extension_types': ['extension']
        },
        {
          'channel': 'stable',
          'extension_types': ['platform_app'],
          'whitelist': ['im not here']
        }
      ],
      'inheritsFromDifferentDependencyName': {
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency']
      },
      'omnibox': {
        'dependencies': ['manifest:omnibox'],
        'contexts': ['blessed_extension']
      },
      'syncFileSystem': {
        'dependencies': ['permission:syncFileSystem'],
        'contexts': ['blessed_extension']
      },
      'tabs': {
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app'],
        'contexts': ['blessed_extension']
      },
      'test': {
        'channel': 'stable',
        'extension_types': 'all',
        'contexts': [
            'blessed_extension', 'unblessed_extension', 'content_script']
      },
      'overridesPlatformAndChannelFromDependency': {
        'channel': 'beta',
        'dependencies': [
          'permission:overridesPlatformAndChannelFromDependency'
        ],
        'extension_types': ['platform_app']
      },
      'windows': {
        'dependencies': ['api:tabs'],
        'contexts': ['blessed_extension']
      }
    }),
    '_manifest_features.json': json.dumps({
      'app.content_security_policy': {
        'channel': 'stable',
        'extension_types': ['platform_app'],
        'min_manifest_version': 2,
        'whitelist': ['this isnt happening']
      },
      'background': {
        'channel': 'stable',
        'extension_types': ['extension', 'legacy_packaged_app', 'hosted_app']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'channel': 'dev',
        'extension_types': ['extension']
      },
      'manifest_version': {
        'channel': 'stable',
        'extension_types': 'all'
      },
      'omnibox': {
        'channel': 'stable',
        'extension_types': ['extension'],
        'platforms': ['win']
      },
      'page_action': {
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'sockets': {
        'channel': 'dev',
        'extension_types': ['platform_app']
      }
    }),
    '_permission_features.json': json.dumps({
      'bluetooth': {
        'channel': 'dev',
        'extension_types': ['platform_app']
      },
      'overridesPlatformAndChannelFromDependency': {
        'channel': 'stable',
        'extension_types': ['extension']
      },
      'power': {
        'channel': 'stable',
        'extension_types': [
          'extension', 'legacy_packaged_app', 'platform_app'
        ]
      },
      'syncFileSystem': {
        'channel': 'beta',
        'extension_types': ['platform_app']
      },
      'tabs': {
        'channel': 'stable',
        'extension_types': ['extension']
      }
    })
  },
  'docs': {
    'templates': {
      'json': {
        'manifest.json': json.dumps({
          'background': {
            'documentation': 'background_pages.html'
          },
          'manifest_version': {
            'documentation': 'manifest/manifest_version.html',
            'example': 2,
            'level': 'required'
          },
          'page_action': {
            'documentation': 'pageAction.html',
            'example': {},
            'level': 'only_one'
          }
        }),
        'permissions.json': json.dumps({
          'fakeUnsupportedFeature': {},
          'syncFileSystem': {
            'partial': 'permissions/sync_file_system.html'
          },
          'tabs': {
            'partial': 'permissions/tabs.html'
          },
        })
      }
    }
  }
}


class FeaturesBundleTest(unittest.TestCase):
  def setUp(self):
    self._server = ServerInstance.ForTest(
        TestFileSystem(_TEST_FILESYSTEM, relative_to=CHROME_EXTENSIONS))

  def testManifestFeatures(self):
    expected_features = {
      'background': {
        'name': 'background',
        'channel': 'stable',
        'platforms': ['extensions'],
        'documentation': 'background_pages.html'
      },
      'inheritsPlatformAndChannelFromDependency': {
        'channel': 'dev',
        'name': 'inheritsPlatformAndChannelFromDependency',
        'platforms': ['extensions']
      },
      'manifest_version': {
        'name': 'manifest_version',
        'channel': 'stable',
        'platforms': ['apps', 'extensions'],
        'documentation': 'manifest/manifest_version.html',
        'level': 'required',
        'example': 2
      },
      'omnibox': {
        'name': 'omnibox',
        'channel': 'stable',
        'platforms': ['extensions']
      },
      'page_action': {
        'name': 'page_action',
        'channel': 'stable',
        'platforms': ['extensions'],
        'documentation': 'pageAction.html',
        'level': 'only_one',
        'example': {}
      },
      'sockets': {
        'name': 'sockets',
        'channel': 'dev',
        'platforms': ['apps']
      }
    }
    self.assertEqual(
        expected_features,
        self._server.features_bundle.GetManifestFeatures().Get())

  def testPermissionFeatures(self):
    expected_features = {
      'bluetooth': {
        'name': 'bluetooth',
        'channel': 'dev',
        'platforms': ['apps'],
      },
      'fakeUnsupportedFeature': {
        'name': 'fakeUnsupportedFeature',
        'platforms': []
      },
      'overridesPlatformAndChannelFromDependency': {
        'name': 'overridesPlatformAndChannelFromDependency',
        'channel': 'stable',
        'platforms': ['extensions']
      },
      'power': {
        'name': 'power',
        'channel': 'stable',
        'platforms': ['apps', 'extensions'],
      },
      'syncFileSystem': {
        'name': 'syncFileSystem',
        'channel': 'beta',
        'platforms': ['apps'],
        'partial': 'permissions/sync_file_system.html'
      },
      'tabs': {
        'name': 'tabs',
        'channel': 'stable',
        'platforms': ['extensions'],
        'partial': 'permissions/tabs.html'
      }
    }
    self.assertEqual(
        expected_features,
        self._server.features_bundle.GetPermissionFeatures().Get())

  def testAPIFeatures(self):
    expected_features = {
      'audioCapture': {
        'name': 'audioCapture',
        'channel': 'stable',
        'platforms': ['apps']
      },
      'background': {
        'name': 'background',
        'channel': 'stable',
        'platforms': ['extensions']
      },
      'inheritsFromDifferentDependencyName': {
        'channel': 'dev',
        'name': 'inheritsFromDifferentDependencyName',
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency'],
        'platforms': ['extensions']
      },
      'inheritsPlatformAndChannelFromDependency': {
        'channel': 'dev',
        'name': 'inheritsPlatformAndChannelFromDependency',
        'dependencies': ['manifest:inheritsPlatformAndChannelFromDependency'],
        'platforms': ['extensions']
      },
      'omnibox': {
        'channel': 'stable',
        'name': 'omnibox',
        'platforms': ['extensions'],
        'contexts': ['blessed_extension'],
        'dependencies': ['manifest:omnibox']
      },
      'overridesPlatformAndChannelFromDependency': {
        'channel': 'beta',
        'name': 'overridesPlatformAndChannelFromDependency',
        'dependencies': [
          'permission:overridesPlatformAndChannelFromDependency'
        ],
        'platforms': ['apps']
      },
      'syncFileSystem': {
        'channel': 'beta',
        'name': 'syncFileSystem',
        'platforms': ['apps'],
        'contexts': ['blessed_extension'],
        'dependencies': ['permission:syncFileSystem']
      },
      'tabs': {
        'channel': 'stable',
        'name': 'tabs',
        'channel': 'stable',
        'platforms': ['extensions'],
        'contexts': ['blessed_extension'],
      },
      'test': {
        'channel': 'stable',
        'name': 'test',
        'channel': 'stable',
        'platforms': ['apps', 'extensions'],
        'contexts': [
            'blessed_extension', 'unblessed_extension', 'content_script'],
      },
      'windows': {
        'channel': 'stable',
        'name': 'windows',
        'platforms': ['extensions'],
        'contexts': ['blessed_extension'],
        'dependencies': ['api:tabs']
      }
    }
    self.assertEqual(
        expected_features,
        self._server.features_bundle.GetAPIFeatures().Get())


if __name__ == '__main__':
  unittest.main()
