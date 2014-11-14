# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=W0401,W0614
from telemetry.page.actions.all_page_actions import *
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class MapsPage(page_module.Page):

  def __init__(self, page_set):
    super(MapsPage, self).__init__(
      url='http://localhost:10020/tracker.html',
      page_set=page_set,
      name='Maps.maps_001')
    self.archive_data_file = 'data/maps.json'

  def RunNavigateSteps(self, action_runner):
    action_runner.NavigateToPage(self)
    action_runner.Wait(3)

  def RunSmoothness(self, action_runner):
    action_runner.WaitForJavaScriptCondition('window.testDone')


class MapsPageSet(page_set_module.PageSet):

  """ Google Maps examples """

  def __init__(self):
    super(MapsPageSet, self).__init__(
        archive_data_file='data/maps.json',
        bucket=page_set_module.INTERNAL_BUCKET)

    self.AddPage(MapsPage(self))
