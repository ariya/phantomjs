/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012-2014, Ivan De Marino <http://ivandemarino.me>
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
import org.openqa.selenium.WebDriver;

public class DriverBasicTest extends BaseTest {
    @Test
    public void useDriverButDontQuit() {
        WebDriver d = getDriver();
        disableAutoQuitDriver();

        d.get("http://www.google.com/");
        d.quit();
    }

//    @Test
//    public void shouldSurviveExecutingManyTimesTheSameCommand() {
//        WebDriver d = getDriver();
//        d.get("http://www.google.com");
//        for (int j = 0; j < 100; j++) {
//            try {
//                d.findElement(By.linkText(org.apache.commons.lang3.RandomStringUtils.randomAlphabetic(4))).isDisplayed();
//            } catch (NoSuchElementException nsee) {
//                // swallow exceptions: we don't care about the result
//            }
//        }
//    }
//
//    @Test
//    public void shouldSurviveExecutingManyTimesTheSameTest() throws Exception {
//        for (int i = 0; i < 100; ++i) {
//            prepareDriver();
//            shouldSurviveExecutingManyTimesTheSameCommand();
//            quitDriver();
//        }
//    }
}
