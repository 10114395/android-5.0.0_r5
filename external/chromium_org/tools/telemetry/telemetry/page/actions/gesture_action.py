# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page.actions import page_action
from telemetry import decorators
from telemetry.web_perf import timeline_interaction_record as tir_module

class GestureAction(page_action.PageAction):
  def __init__(self, attributes=None):
    super(GestureAction, self).__init__(attributes)
    if not hasattr(self, 'automatically_record_interaction'):
      self.automatically_record_interaction = True

  def RunAction(self, tab):
    interaction_name = 'Gesture_%s' % self.__class__.__name__
    if self.automatically_record_interaction:
      tab.ExecuteJavaScript('console.time("%s");' %
          tir_module.TimelineInteractionRecord.GetJavaScriptMarker(
              interaction_name, [tir_module.IS_SMOOTH]))
    self.RunGesture(tab)
    if self.automatically_record_interaction:
      tab.ExecuteJavaScript('console.timeEnd("%s");' %
          tir_module.TimelineInteractionRecord.GetJavaScriptMarker(
              interaction_name, [tir_module.IS_SMOOTH]))

  def RunGesture(self, tab):
    raise NotImplementedError()

  @staticmethod
  def GetGestureSourceTypeFromOptions(tab):
    gesture_source_type = tab.browser.synthetic_gesture_source_type
    return 'chrome.gpuBenchmarking.' + gesture_source_type.upper() + '_INPUT'

  @staticmethod
  @decorators.Cache
  def IsGestureSourceTypeSupported(tab, gesture_source_type):
    # TODO(dominikg): remove once support for
    #                 'chrome.gpuBenchmarking.gestureSourceTypeSupported' has
    #                 been rolled into reference build.
    if tab.EvaluateJavaScript("""
        typeof chrome.gpuBenchmarking.gestureSourceTypeSupported ===
            'undefined'"""):
      return (tab.browser.platform.GetOSName() != 'mac' or
              gesture_source_type.lower() != 'touch')

    return tab.EvaluateJavaScript("""
        chrome.gpuBenchmarking.gestureSourceTypeSupported(
            chrome.gpuBenchmarking.%s_INPUT)"""
        % (gesture_source_type.upper()))
