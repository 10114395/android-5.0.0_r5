#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Implements a unittest TestRunner with GTest output.

This output is ported from gtest.cc's PrettyUnitTestResultPrinter, but
designed to be a drop-in replacement for unittest's TextTestRunner.
"""

import sys
import time
import unittest

from telemetry.unittest import gtest_unittest_results


class GTestTestSuite(unittest.TestSuite):
  def __call__(self, *args, **kwargs):
    result = args[0]
    timestamp = time.time()
    unit = 'test' if len(self._tests) == 1 else 'tests'
    if not any(isinstance(x, unittest.TestSuite) for x in self._tests):
      print '[----------] %d %s' % (len(self._tests), unit)
    for test in self._tests:
      if result.shouldStop:
        break
      test(result)
    endts = time.time()
    ms = (endts - timestamp) * 1000
    if not any(isinstance(x, unittest.TestSuite) for x in self._tests):
      print '[----------] %d %s (%d ms total)' % (len(self._tests), unit, ms)
      print
    return result


class GTestTestRunner(object):
  def __init__(self, print_result_after_run=True):
    self.print_result_after_run = print_result_after_run
    self.result = gtest_unittest_results.GTestUnittestResults(sys.stdout)

  def run(self, test):
    "Run the given test case or test suite."
    test(self.result)
    if self.print_result_after_run:
      self.result.PrintSummary()
    return self.result
