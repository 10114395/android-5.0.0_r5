# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=W0401,W0614
from telemetry.page.actions.all_page_actions import *
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class BasicPlayPage(page_module.Page):

  def __init__(self, url, page_set):
    super(BasicPlayPage, self).__init__(url=url, page_set=page_set)
    self.add_browser_metrics = True

  def PlayAction(self, action_runner):
    action_runner.RunAction(PlayAction(
      {
        'wait_for_playing': True,
        'wait_for_ended': True
      }))

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)

  def SeekBeforeAndAfterPlayhead(self, action_runner):
    action_runner.RunAction(PlayAction(
      {
        'wait_for_playing': True,
        'wait_for_ended': False
      }))
    # Wait for 1 second so that we know the play-head is at ~1s.
    action_runner.Wait(1)
    # Seek to before the play-head location.
    action_runner.RunAction(SeekAction(
      {
        'seek_time': '0.5',
        'wait_for_seeked': True,
        'seek_label': 'seek_warm'
      }))
    # Seek to after the play-head location.
    action_runner.RunAction(SeekAction(
      {
        'seek_time': 15,
        'wait_for_seeked': True,
        'seek_label': 'seek_cold'
      }))

class SeekBeforeAndAfterPlayheadPage(BasicPlayPage):

  def __init__(self, url, page_set):
    super(SeekBeforeAndAfterPlayheadPage, self).__init__(url=url,
                                                         page_set=page_set)
    self.add_browser_metrics = False

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class MediaCnsCasesPageSet(page_set_module.PageSet):

  """ Media benchmark on network constrained conditions. """

  def __init__(self):
    super(MediaCnsCasesPageSet, self).__init__()

    urls_list = [
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_webm&src=tulip2.webm&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_webm&src=tulip2.webm&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_webm&src=tulip2.webm&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_ogv&src=tulip2.ogv&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_ogv&src=tulip2.ogv&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_ogv&src=tulip2.ogv&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_mp4&src=tulip2.mp4&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_mp4&src=tulip2.mp4&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_mp4&src=tulip2.mp4&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_wav&src=tulip2.wav&type=audio&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_wav&src=tulip2.wav&type=audio&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_wav&src=tulip2.wav&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_ogg&src=tulip2.ogg&type=audio&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_ogg&src=tulip2.ogg&type=audio&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_ogg&src=tulip2.ogg&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_mp3&src=tulip2.mp3&type=audio&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_mp3&src=tulip2.mp3&type=audio&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_mp3&src=tulip2.mp3&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=no_constraints_m4a&src=tulip2.m4a&type=audio&net=none',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=cable_m4a&src=tulip2.m4a&type=audio&net=cable',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_m4a&src=tulip2.m4a&type=audio&net=wifi'
    ]

    for url in urls_list:
      self.AddPage(BasicPlayPage(url, self))

    urls_list2 = [
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_mp3&src=tulip2.mp3&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_m4a&src=tulip2.m4a&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_ogg&src=tulip2.ogg&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_wav&src=tulip2.wav&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_mp4&src=tulip2.mp4&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_ogv&src=tulip2.ogv&type=audio&net=wifi',
      # pylint: disable=C0301
      'file://tough_video_cases/video.html?id=wifi_webm&src=tulip2.webm&type=audio&net=wifi'
    ]

    for url in urls_list2:
      self.AddPage(SeekBeforeAndAfterPlayheadPage(url, self))
