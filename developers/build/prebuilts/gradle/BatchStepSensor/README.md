Android BatchStepSensor Sample
==============================

This sample demonstrates the use of the two step sensors (step detector and counter) and
sensor batching.
 It shows how to register a SensorEventListener with and without
batching and shows how these events are received.
The Step Detector sensor fires an
event when a step is detected, while the step counter returns the total number of
steps since a listener was first registered for this sensor.
Both sensors only count steps while a listener is registered. This sample only covers the
basic case, where a listener is only registered while the app is running. Likewise,
batched sensors can be used in the background (when the CPU is suspended), which
requires manually flushing the sensor event queue before it overflows, which is not
covered in this sample.

Pre-requisites
--------------

- Android SDK v20
- Android Build Tools v20
- Android Support Repository

Getting Started
---------------

This sample uses the Gradle build system. To build this project, use the
"gradlew build" command or use "Import Project" in Android Studio.

Support
-------

- Google+ Community: https://plus.google.com/communities/105153134372062985968
- Stack Overflow: http://stackoverflow.com/questions/tagged/android

If you've found an error in this sample, please file an issue:
https://github.com/googlesamples/android-BatchStepSensor

Patches are encouraged, and may be submitted by forking this project and
submitting a pull request through GitHub. Please see CONTRIBUTING.md for more details.

License
-------

Copyright 2014 The Android Open Source Project, Inc.

Licensed to the Apache Software Foundation (ASF) under one or more contributor
license agreements.  See the NOTICE file distributed with this work for
additional information regarding copyright ownership.  The ASF licenses this
file to you under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy of
the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations under
the License.
