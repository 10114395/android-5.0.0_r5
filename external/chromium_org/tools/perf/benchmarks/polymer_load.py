# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import page_sets
from measurements import polymer_load
from telemetry import test


class PolymerLoadPica(test.Test):
  """Measures time to polymer-ready for PICA
  """
  test = polymer_load.PolymerLoadMeasurement
  page_set = page_sets.PicaPageSet
