# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

from telemetry.page.actions.gesture_action import GestureAction
from telemetry.page.actions import page_action

class PinchAction(GestureAction):
  def __init__(self, attributes=None):
    super(PinchAction, self).__init__(attributes)

  def WillRunAction(self, tab):
    for js_file in ['gesture_common.js', 'pinch.js']:
      with open(os.path.join(os.path.dirname(__file__), js_file)) as f:
        js = f.read()
        tab.ExecuteJavaScript(js)

    # Fail if browser doesn't support synthetic pinch gestures.
    if not tab.EvaluateJavaScript('window.__PinchAction_SupportedByBrowser()'):
      raise page_action.PageActionNotSupported(
          'Synthetic pinch not supported for this browser')

    # TODO(dominikg): Remove once JS interface changes have rolled into stable.
    if not tab.EvaluateJavaScript('chrome.gpuBenchmarking.newPinchInterface'):
      raise page_action.PageActionNotSupported(
          'This version of the browser doesn\'t support the new JS interface '
          'for pinch gestures.')

    if (GestureAction.GetGestureSourceTypeFromOptions(tab) ==
        'chrome.gpuBenchmarking.MOUSE_INPUT'):
      raise page_action.PageActionNotSupported(
          'Pinch page action does not support mouse input')

    if not GestureAction.IsGestureSourceTypeSupported(tab, 'touch'):
      raise page_action.PageActionNotSupported(
          'Touch input not supported for this browser')

    done_callback = 'function() { window.__pinchActionDone = true; }'
    tab.ExecuteJavaScript("""
        window.__pinchActionDone = false;
        window.__pinchAction = new __PinchAction(%s);"""
        % done_callback)

  @staticmethod
  def _GetDefaultScaleFactorForPage(tab):
    current_scale_factor = tab.EvaluateJavaScript(
        'window.outerWidth / window.innerWidth')
    return 3.0 / current_scale_factor

  def RunGesture(self, tab):
    left_anchor_percentage = getattr(self, 'left_anchor_percentage', 0.5)
    top_anchor_percentage = getattr(self, 'top_anchor_percentage', 0.5)
    scale_factor = getattr(self, 'scale_factor',
                           PinchAction._GetDefaultScaleFactorForPage(tab))
    speed = getattr(self, 'speed', 800)

    if hasattr(self, 'element_function'):
      tab.ExecuteJavaScript("""
          (%s)(function(element) { window.__pinchAction.start(
             { element: element,
               left_anchor_percentage: %s,
               top_anchor_percentage: %s,
               scale_factor: %s,
               speed: %s })
             });""" % (self.element_function,
                       left_anchor_percentage,
                       top_anchor_percentage,
                       scale_factor,
                       speed))
    else:
      tab.ExecuteJavaScript("""
          window.__pinchAction.start(
          { element: document.body,
            left_anchor_percentage: %s,
            top_anchor_percentage: %s,
            scale_factor: %s,
            speed: %s });"""
        % (left_anchor_percentage,
           top_anchor_percentage,
           scale_factor,
           speed))

    tab.WaitForJavaScriptExpression('window.__pinchActionDone', 60)
