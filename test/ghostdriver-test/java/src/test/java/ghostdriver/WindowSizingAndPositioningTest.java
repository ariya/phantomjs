/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2014, Ivan De Marino <http://ivandemarino.me>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package ghostdriver;

import org.junit.Test;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.Point;
import org.openqa.selenium.WebDriver;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class WindowSizingAndPositioningTest extends BaseTest {
    @Test
    public void manipulateWindowSize() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        assertTrue(d.manage().window().getSize().width > 100);
        assertTrue(d.manage().window().getSize().height > 100);

        d.manage().window().setSize(new Dimension(1024, 768));
        assertEquals(d.manage().window().getSize().width, 1024);
        assertEquals(d.manage().window().getSize().height, 768);
    }

    @Test
    public void manipulateWindowPosition() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        assertTrue(d.manage().window().getPosition().x >= 0);
        assertTrue(d.manage().window().getPosition().y >= 0);

        d.manage().window().setPosition(new Point(0, 0));
        assertTrue(d.manage().window().getPosition().x == 0);
        assertTrue(d.manage().window().getPosition().y == 0);
    }

    @Test
    public void manipulateWindowMaximize() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");

        Dimension sizeBefore = d.manage().window().getSize();
        d.manage().window().maximize();
        Dimension sizeAfter = d.manage().window().getSize();

        assertTrue(sizeBefore.width <= sizeAfter.width);
        assertTrue(sizeBefore.height <= sizeAfter.height);
    }
}
