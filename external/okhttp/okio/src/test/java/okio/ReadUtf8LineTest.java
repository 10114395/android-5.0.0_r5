/*
 * Copyright (C) 2014 Square, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package okio;

import java.io.EOFException;
import java.io.IOException;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public abstract class ReadUtf8LineTest {
  protected abstract BufferedSource newSource(String s);

  @Test public void readLines() throws IOException {
    BufferedSource source = newSource("abc\ndef\n");
    assertEquals("abc", source.readUtf8LineStrict());
    assertEquals("def", source.readUtf8LineStrict());
    try {
      source.readUtf8LineStrict();
      fail();
    } catch (EOFException expected) {
    }
  }

  @Test public void emptyLines() throws IOException {
    BufferedSource source = newSource("\n\n\n");
    assertEquals("", source.readUtf8LineStrict());
    assertEquals("", source.readUtf8LineStrict());
    assertEquals("", source.readUtf8LineStrict());
    assertTrue(source.exhausted());
  }

  @Test public void crDroppedPrecedingLf() throws IOException {
    BufferedSource source = newSource("abc\r\ndef\r\nghi\rjkl\r\n");
    assertEquals("abc", source.readUtf8LineStrict());
    assertEquals("def", source.readUtf8LineStrict());
    assertEquals("ghi\rjkl", source.readUtf8LineStrict());
  }

  @Test public void bufferedReaderCompatible() throws IOException {
    BufferedSource source = newSource("abc\ndef");
    assertEquals("abc", source.readUtf8Line());
    assertEquals("def", source.readUtf8Line());
    assertEquals(null, source.readUtf8Line());
  }

  @Test public void bufferedReaderCompatibleWithTrailingNewline() throws IOException {
    BufferedSource source = newSource("abc\ndef\n");
    assertEquals("abc", source.readUtf8Line());
    assertEquals("def", source.readUtf8Line());
    assertEquals(null, source.readUtf8Line());
  }
}
