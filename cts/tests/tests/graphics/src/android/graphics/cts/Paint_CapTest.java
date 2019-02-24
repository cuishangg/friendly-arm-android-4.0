/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.graphics.cts;

import junit.framework.TestCase;
import dalvik.annotation.TestTargets;
import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetNew;
import dalvik.annotation.TestTargetClass;
import android.graphics.Paint;
import android.graphics.Paint.Cap;

@TestTargetClass(Paint.Cap.class)
public class Paint_CapTest extends TestCase {

    @TestTargetNew(
        level = TestLevel.COMPLETE,
        method = "valueOf",
        args = {java.lang.String.class}
    )
    public void testValueOf() {

        assertEquals(Cap.BUTT, Cap.valueOf("BUTT"));
        assertEquals(Cap.ROUND, Cap.valueOf("ROUND"));
        assertEquals(Cap.SQUARE, Cap.valueOf("SQUARE"));
    }

    @TestTargets({
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            method = "values",
            args = {}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            method = "setStrokeCap",
            args = {android.graphics.Paint.Cap.class}
        ),
        @TestTargetNew(
            level = TestLevel.COMPLETE,
            method = "getStrokeCap",
            args = {}
        )
    })
    public void testValues() {
        // set the actual value
        Cap[] actual = Cap.values();

        assertEquals(3, actual.length);
        assertEquals(Cap.BUTT, actual[0]);
        assertEquals(Cap.ROUND, actual[1]);
        assertEquals(Cap.SQUARE, actual[2]);

        // Here we use Cap as the param of setStrokeCap
        // and get the setting result by getStrokeCap
        Paint p = new Paint();
        p.setStrokeCap(actual[0]);
        assertEquals(Cap.BUTT, p.getStrokeCap());
        p.setStrokeCap(actual[1]);
        assertEquals(Cap.ROUND, p.getStrokeCap());
        p.setStrokeCap(actual[2]);
        assertEquals(Cap.SQUARE, p.getStrokeCap());
    }
}