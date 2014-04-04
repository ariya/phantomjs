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

import ghostdriver.server.EmptyPageHttpRequestCallback;
import ghostdriver.server.HttpRequestCallback;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.openqa.selenium.Cookie;
import org.openqa.selenium.InvalidCookieDomainException;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Date;

import static org.junit.Assert.*;

public class CookieTest extends BaseTestWithServer {
    private WebDriver driver;

    private final static HttpRequestCallback COOKIE_SETTING_CALLBACK = new EmptyPageHttpRequestCallback() {
        @Override
        public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
            super.call(req, res);
            javax.servlet.http.Cookie cookie = new javax.servlet.http.Cookie("test", "test");
            cookie.setDomain(".localhost");
            cookie.setMaxAge(360);

            res.addCookie(cookie);

            cookie = new javax.servlet.http.Cookie("test2", "test2");
            cookie.setDomain(".localhost");
            res.addCookie(cookie);
        }
    };

    private final static HttpRequestCallback EMPTY_CALLBACK = new EmptyPageHttpRequestCallback();

    @Before
    public void setup() {
        driver = getDriver();
    }

    @After
    public void cleanUp() {
        driver.manage().deleteAllCookies();
    }

    private void goToPage(String path) {
        driver.get(server.getBaseUrl() + path);
    }

    private void goToPage() {
        goToPage("");
    }

    private Cookie[] getCookies() {
        return driver.manage().getCookies().toArray(new Cookie[]{});
    }

    @Test
    public void gettingAllCookies() {
        server.setHttpHandler("GET", COOKIE_SETTING_CALLBACK);
        goToPage();
        Cookie[] cookies = getCookies();

        assertEquals(2, cookies.length);
        assertEquals("test", cookies[0].getName());
        assertEquals("test", cookies[0].getValue());
        assertEquals(".localhost", cookies[0].getDomain());
        assertEquals("/", cookies[0].getPath());
        assertTrue(cookies[0].getExpiry() != null);
        assertEquals(false, cookies[0].isSecure());
        assertEquals("test2", cookies[1].getName());
        assertEquals("test2", cookies[1].getValue());
        assertEquals(".localhost", cookies[1].getDomain());
        assertEquals("/", cookies[1].getPath());
        assertEquals(false, cookies[1].isSecure());
        assertTrue(cookies[1].getExpiry() == null);
    }

    @Test
    public void gettingAllCookiesOnANonCookieSettingPage() {
        server.setHttpHandler("GET", EMPTY_CALLBACK);
        goToPage();
        assertEquals(0, getCookies().length);
    }

    @Test
    public void deletingAllCookies() {
        server.setHttpHandler("GET", COOKIE_SETTING_CALLBACK);
        goToPage();
        driver.manage().deleteAllCookies();
        assertEquals(0, getCookies().length);
    }

    @Test
    public void deletingOneCookie() {
        server.setHttpHandler("GET", COOKIE_SETTING_CALLBACK);
        goToPage();

        driver.manage().deleteCookieNamed("test");

        Cookie[] cookies = getCookies();

        assertEquals(1, cookies.length);
        assertEquals("test2", cookies[0].getName());
    }

    @Test
    public void addingACookie() {
        server.setHttpHandler("GET", EMPTY_CALLBACK);
        goToPage();

        driver.manage().addCookie(new Cookie("newCookie", "newValue"));

        Cookie[] cookies = getCookies();
        assertEquals(1, cookies.length);
        assertEquals("newCookie", cookies[0].getName());
        assertEquals("newValue", cookies[0].getValue());
        assertEquals("localhost", cookies[0].getDomain());
        assertEquals("/", cookies[0].getPath());
        assertEquals(false, cookies[0].isSecure());
    }

    @Test
    public void modifyingACookie() {
        server.setHttpHandler("GET", COOKIE_SETTING_CALLBACK);
        goToPage();

        driver.manage().addCookie(new Cookie("test", "newValue", "localhost", "/", null, false));

        Cookie[] cookies = getCookies();
        assertEquals(2, cookies.length);
        assertEquals("test", cookies[0].getName());
        assertEquals("newValue", cookies[0].getValue());
        assertEquals(".localhost", cookies[0].getDomain());
        assertEquals("/", cookies[0].getPath());
        assertEquals(false, cookies[0].isSecure());

        assertEquals("test2", cookies[1].getName());
        assertEquals("test2", cookies[1].getValue());
        assertEquals(".localhost", cookies[1].getDomain());
        assertEquals("/", cookies[1].getPath());
        assertEquals(false, cookies[1].isSecure());
    }

    @Test
    public void shouldRetainCookieInfo() {
        server.setHttpHandler("GET", EMPTY_CALLBACK);
        goToPage();

        // Added cookie (in a sub-path - allowed)
        Cookie addedCookie =
                new Cookie.Builder("fish", "cod")
                        .expiresOn(new Date(System.currentTimeMillis() + 100 * 1000)) //< now + 100sec
                        .path("/404")
                        .domain("localhost")
                        .build();
        driver.manage().addCookie(addedCookie);

        // Search cookie on the root-path and fail to find it
        Cookie retrieved = driver.manage().getCookieNamed("fish");
        assertNull(retrieved);

        // Go to the "/404" sub-path (to find the cookie)
        goToPage("404");
        retrieved = driver.manage().getCookieNamed("fish");
        assertNotNull(retrieved);
        // Check that it all matches
        assertEquals(addedCookie.getName(), retrieved.getName());
        assertEquals(addedCookie.getValue(), retrieved.getValue());
        assertEquals(addedCookie.getExpiry(), retrieved.getExpiry());
        assertEquals(addedCookie.isSecure(), retrieved.isSecure());
        assertEquals(addedCookie.getPath(), retrieved.getPath());
        assertTrue(retrieved.getDomain().contains(addedCookie.getDomain()));
    }

    @Test(expected = InvalidCookieDomainException.class)
    public void shouldNotAllowToCreateCookieOnDifferentDomain() {
        goToPage();

        // Added cookie (in a sub-path)
        Cookie addedCookie = new Cookie.Builder("fish", "cod")
                .expiresOn(new Date(System.currentTimeMillis() + 100 * 1000)) //< now + 100sec
                .path("/404")
                .domain("github.com")
                .build();
        driver.manage().addCookie(addedCookie);
    }

    @Test
    public void shouldAllowToDeleteCookiesEvenIfNotSet() {
        WebDriver d = getDriver();
        d.get("https://github.com/");

        // Clear all cookies
        assertTrue(d.manage().getCookies().size() > 0);
        d.manage().deleteAllCookies();
        assertEquals(d.manage().getCookies().size(), 0);

        // All cookies deleted, call deleteAllCookies again. Should be a no-op.
        d.manage().deleteAllCookies();
        d.manage().deleteCookieNamed("non_existing_cookie");
        assertEquals(d.manage().getCookies().size(), 0);
    }

    @Test
    public void shouldAllowToSetCookieThatIsAlreadyExpired() {
        WebDriver d = getDriver();
        d.get("https://github.com/");

        // Clear all cookies
        assertTrue(d.manage().getCookies().size() > 0);
        d.manage().deleteAllCookies();
        assertEquals(d.manage().getCookies().size(), 0);

        // Added cookie that expires in the past
        Cookie addedCookie = new Cookie.Builder("expired", "yes")
                .expiresOn(new Date(System.currentTimeMillis() - 1000)) //< now - 1 second
                .build();
        d.manage().addCookie(addedCookie);

        Cookie cookie = d.manage().getCookieNamed("expired");
        assertNull(cookie);
    }

    @Test(expected = Exception.class)
    public void shouldThrowExceptionIfAddingCookieBeforeLoadingAnyUrl() {
        // NOTE: At the time of writing, this test doesn't pass with FirefoxDriver.
        // ChromeDriver is fine instead.
        String xval = "123456789101112"; //< detro: I buy you a beer if you guess what am I quoting here
        WebDriver d = getDriver();

        // Set cookie, without opening any page: should throw an exception
        d.manage().addCookie(new Cookie("x", xval));
    }

    @Test
    public void shouldBeAbleToCreateCookieViaJavascriptOnGoogle() {
        String ckey = "cookiekey";
        String cval = "cookieval";

        WebDriver d = getDriver();
        d.get("http://www.google.com");
        JavascriptExecutor js = (JavascriptExecutor) d;

        // Of course, no cookie yet(!)
        Cookie c = d.manage().getCookieNamed(ckey);
        assertNull(c);

        // Attempt to create cookie on multiple Google domains
        js.executeScript("javascript:(" +
                "function() {" +
                "   cook = document.cookie;" +
                "   begin = cook.indexOf('"+ckey+"=');" +
                "   var val;" +
                "   if (begin !== -1) {" +
                "       var end = cook.indexOf(\";\",begin);" +
                "       if (end === -1)" +
                "           end=cook.length;" +
                "       val=cook.substring(begin+11,end);" +
                "   }" +
                "   val = ['"+cval+"'];" +
                "   if (val) {" +
                "       var d=Array('com','co.jp','ca','fr','de','co.uk','it','es','com.br');" +
                "       for (var i = 0; i < d.length; i++) {" +
                "           document.cookie = '"+ckey+"='+val+';path=/;domain=.google.'+d[i]+'; ';" +
                "       }" +
                "   }" +
                "})();");
        c = d.manage().getCookieNamed(ckey);
        assertNotNull(c);
        assertEquals(cval, c.getValue());

        // Set cookie as empty
        js.executeScript("javascript:(" +
                "function() {" +
                "   var d = Array('com','co.jp','ca','fr','de','co.uk','it','cn','es','com.br');" +
                "   for(var i = 0; i < d.length; i++) {" +
                "       document.cookie='"+ckey+"=;path=/;domain=.google.'+d[i]+'; ';" +
                "   }" +
                "})();");
        c = d.manage().getCookieNamed(ckey);
        assertNotNull(c);
        assertEquals("", c.getValue());
    }
}
