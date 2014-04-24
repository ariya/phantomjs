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

import org.junit.BeforeClass;
import org.junit.Test;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriverService;

import static org.junit.Assert.assertTrue;

public class NavigationTest extends BaseTest {
    @BeforeClass
    public static void setUserAgentForPhantomJSDriver() {
        // Setting a generic Chrome UA to bypass some UA spoofing
        sCaps.setCapability(
                PhantomJSDriverService.PHANTOMJS_PAGE_SETTINGS_PREFIX + "userAgent",
                "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.97 Safari/537.11"
        );
    }

    @Test
    public void navigateAroundMDN() {
        WebDriver d = getDriver();

        d.get("https://developer.mozilla.org/en-US/");
        assertTrue(d.getTitle().toLowerCase().contains("Mozilla".toLowerCase()));
        d.navigate().to("https://developer.mozilla.org/en/HTML/HTML5");
        assertTrue(d.getTitle().toLowerCase().contains("HTML5".toLowerCase()));
        d.navigate().refresh();
        assertTrue(d.getTitle().toLowerCase().contains("HTML5".toLowerCase()));
        d.navigate().back();
        assertTrue(d.getTitle().toLowerCase().contains("Mozilla".toLowerCase()));
        d.navigate().forward();
        assertTrue(d.getTitle().toLowerCase().contains("HTML5".toLowerCase()));
    }

    @Test
    public void navigateBackWithNoHistory() throws Exception {
        // Fresh Driver (every test gets one)
        WebDriver d = getDriver();

        // Navigate back and forward: should be a no-op, given we haven't loaded anything yet
        d.navigate().back();
        d.navigate().forward();

        // Make sure explicit navigation still works.
        d.get("http://google.com");
    }

    @Test
    public void navigateToGoogleAdwords() {
        WebDriver d = getDriver();
        d.get("http://adwords.google.com");
        assertTrue(d.getCurrentUrl().contains("google.com"));
    }

    @Test
    public void navigateToNameJet() {
        // NOTE: This passes only when the User Agent is NOT PhantomJS {@see setUserAgentForPhantomJSDriver}
        // method above.
        WebDriver d = getDriver();
        d.navigate().to("http://www.namejet.com/");
    }
}
