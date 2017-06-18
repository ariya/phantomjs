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

import org.junit.Before;
import org.junit.Test;
import org.openqa.selenium.Cookie;
import org.openqa.selenium.WebDriver;

import java.util.Set;

import static org.junit.Assert.assertFalse;

public class IsolatedSessionTest extends BaseTest {
    // New Session Cookies will be stored in here
    private String url = "http://httpbin.org/cookies/set";
    private Set<Cookie> firstSessionCookies;
    private Set<Cookie> secondSessionCookies;

    @Before
    public void createSession() throws Exception {
        disableAutoQuitDriver();

        // Create first Driver, and grab it's cookies
        WebDriver d = getDriver();
        d.get(url + "?session1=value1");
        // Grab set of session cookies
        firstSessionCookies = d.manage().getCookies();
        // Manually quit the current Driver and create a new one
        d.quit();

        // Create second Driver, and grab it's cookies
        prepareDriver();
        d = getDriver();
        d.get(url + "?session2=value2");
        // Grab set of session cookies
        secondSessionCookies = d.manage().getCookies();
        // Manually quit the current Driver and create a new one
        d.quit();
    }

    @Test
    public void shouldCreateASeparateSessionWithEveryNewDriverInstance() {
        // No cookie of the new Session can be found in the cookies of the old Session
        for (Cookie c : firstSessionCookies) {
            assertFalse(secondSessionCookies.contains(c));
        }
        // No cookie of the old Session can be found in the cookies of the new Session
        for (Cookie c : secondSessionCookies) {
            assertFalse(firstSessionCookies.contains(c));
        }
    }
}
