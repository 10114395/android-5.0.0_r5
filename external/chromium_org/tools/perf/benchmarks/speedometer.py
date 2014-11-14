# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Apple's Speedometer performance benchmark.

Speedometer measures simulated user interactions in web applications.

The current benchmark uses TodoMVC to simulate user actions for adding,
completing, and removing to-do items. Speedometer repeats the same actions using
DOM APIs - a core set of web platform APIs used extensively in web applications-
as well as six popular JavaScript frameworks: Ember.js, Backbone.js, jQuery,
AngularJS, React, and Flight. Many of these frameworks are used on the most
popular websites in the world, such as Facebook and Twitter. The performance of
these types of operations depends on the speed of the DOM APIs, the JavaScript
engine, CSS style resolution, layout, and other technologies.
"""

import os

from telemetry import test
from telemetry.page import page_measurement
from telemetry.page import page_set


class SpeedometerMeasurement(page_measurement.PageMeasurement):

  def MeasurePage(self, _, tab, results):
    tab.WaitForDocumentReadyStateToBeComplete()
    tab.ExecuteJavaScript('benchmarkClient.iterationCount = 10; startTest();')
    tab.WaitForJavaScriptExpression(
        'benchmarkClient._finishedTestCount == benchmarkClient.testsCount', 600)
    results.Add(
        'Total', 'ms', tab.EvaluateJavaScript('benchmarkClient._timeValues'))


@test.Disabled('android')  # Times out
class Speedometer(test.Test):
  test = SpeedometerMeasurement

  def CreatePageSet(self, options):
    ps = page_set.PageSet(
        file_path=os.path.abspath(__file__),
        archive_data_file='../page_sets/data/speedometer.json',
        make_javascript_deterministic=False)
    ps.AddPageWithDefaultRunNavigate('http://browserbench.org/Speedometer/')
    return ps
