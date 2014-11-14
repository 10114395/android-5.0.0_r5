# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=W0401,W0614
from telemetry.page.actions.all_page_actions import *
from telemetry.page import page as page_module
from telemetry.page import page_set as page_set_module


class ToughVideoCasesPage(page_module.Page):

  def __init__(self, url, page_set):
    super(ToughVideoCasesPage, self).__init__(url=url, page_set=page_set)

  def LoopMixedAudio(self, action_runner):
    action_runner.RunAction(PlayAction(
      {
        'wait_for_playing': True,
        'wait_for_ended': False,
        'selector': '#background_audio'
      }))
    action_runner.RunAction(LoopAction(
      {
        'loop_count': 50,
        'selector': '#mixed_audio'
      }))

  def LoopSingleAudio(self, action_runner):
    action_runner.RunAction(LoopAction(
      {
        'loop_count': 50,
        'selector': '#single_audio'
      }))

  def PlayAction(self, action_runner):
    action_runner.RunAction(PlayAction(
      {
        'wait_for_playing': True,
        'wait_for_ended': True
      }))

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
        'seek_time': '9',
        'wait_for_seeked': True,
        'seek_label': 'seek_cold'
      }))


class Page1(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page1, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd.wav&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page2(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page2, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd.ogg&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page3(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page3, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd1080.ogv',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page4(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page4, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd1080.webm',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page5(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page5, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd2160.ogv',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_4k = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page6(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page6, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd2160.webm',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_4k = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page7(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page7, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.ogg&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page8(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page8, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.wav&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page9(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page9, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.ogv',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page10(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page10, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.webm',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page11(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page11, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd1080.mp4',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page12(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page12, self).__init__(
      url='file://tough_video_cases/video.html?src=crowd2160.mp4',
      page_set=page_set)

    self.add_browser_metrics = True
    self.is_4k = True
    self.is_50fps = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page13(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page13, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.mp3&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page14(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page14, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.mp4',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page15(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page15, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.m4a&type=audio',
      page_set=page_set)

    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page16(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page16, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.webm',
      page_set=page_set)

    self.is_4k = True
    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page17(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page17, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.mp4',
      page_set=page_set)

    self.is_4k = True
    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page18(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page18, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.ogv',
      page_set=page_set)

    self.is_4k = True
    self.add_browser_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.PlayAction(action_runner)


class Page19(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page19, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.ogg&type=audio',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page20(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page20, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.wav&type=audio',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page21(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page21, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.ogv',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page22(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page22, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.webm',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page23(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page23, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.mp3&type=audio',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page24(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page24, self).__init__(
      url='file://tough_video_cases/video.html?src=tulip2.mp4',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page25(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page25, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.webm',
      page_set=page_set)

    self.skip_basic_metrics = True
    self.is_4k = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page26(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page26, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.mp4',
      page_set=page_set)

    self.skip_basic_metrics = True
    self.is_4k = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page27(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page27, self).__init__(
      url='file://tough_video_cases/video.html?src=garden2_10s.ogv',
      page_set=page_set)

    self.skip_basic_metrics = True
    self.is_4k = True

  def RunMediaMetrics(self, action_runner):
    self.SeekBeforeAndAfterPlayhead(action_runner)


class Page28(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page28, self).__init__(
      url='file://tough_video_cases/audio_playback.html?id=single_audio',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.LoopSingleAudio(action_runner)


class Page29(ToughVideoCasesPage):

  def __init__(self, page_set):
    super(Page29, self).__init__(
      url='file://tough_video_cases/audio_playback.html?id=mixed_audio',
      page_set=page_set)

    self.skip_basic_metrics = True

  def RunMediaMetrics(self, action_runner):
    self.LoopMixedAudio(action_runner)


class ToughVideoCasesPageSet(page_set_module.PageSet):

  """
  Description: Video Stack Perf benchmark
  """
  def __init__(self):
    super(ToughVideoCasesPageSet, self).__init__(
            bucket=page_set_module.INTERNAL_BUCKET)

    self.AddPage(Page1(self))
    self.AddPage(Page2(self))
    self.AddPage(Page3(self))
    self.AddPage(Page4(self))
    self.AddPage(Page5(self))
    self.AddPage(Page6(self))
    self.AddPage(Page7(self))
    self.AddPage(Page8(self))
    self.AddPage(Page9(self))
    self.AddPage(Page10(self))
    self.AddPage(Page11(self))
    self.AddPage(Page12(self))
    self.AddPage(Page13(self))
    self.AddPage(Page14(self))
    self.AddPage(Page15(self))
    self.AddPage(Page16(self))
    self.AddPage(Page17(self))
    self.AddPage(Page18(self))
    self.AddPage(Page19(self))
    self.AddPage(Page20(self))
    self.AddPage(Page21(self))
    self.AddPage(Page22(self))
    self.AddPage(Page23(self))
    self.AddPage(Page24(self))
    self.AddPage(Page25(self))
    self.AddPage(Page26(self))
    self.AddPage(Page27(self))
    self.AddPage(Page28(self))
    self.AddPage(Page29(self))
