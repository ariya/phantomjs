/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2017, Jason Gowan
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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriverService;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriverException;
import org.junit.BeforeClass;

import ghostdriver.server.HttpRequestCallback;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;

public class AuthBasicTest extends BaseTestWithServer {

    // credentials for testing, no one would ever use these
    private final static String userName = "admin";
    private final static String password = "admin";

    @BeforeClass
    public static void setCustomHeaders() {
        sCaps.setCapability(
                "phantomjs.page.customHeaders.Accept-Encoding",
                "gzip, deflate"
                );
    }

    @Override
    public void prepareDriver() throws Exception {
        sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_PAGE_SETTINGS_PREFIX + "userName", userName);
        sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_PAGE_SETTINGS_PREFIX + "password", password);

        super.prepareDriver();
    }

    @Test
    public void simpleBasicAuthShouldWork() {
        // Get Driver Instance
        WebDriver driver = getDriver();

        // wrong password
        driver.get(String.format("http://httpbin.org/basic-auth/%s/Wrong%s", userName, password));
        assertTrue(!driver.getPageSource().contains("authenticated"));

        // we should be authorized
        driver.get(String.format("http://httpbin.org/basic-auth/%s/%s", userName, password));
        assertTrue(driver.getPageSource().contains("authenticated"));
    }

    // we should be able to interact with pages that have content security policies
    // @Ignore
    @Test
    public void canSendKeysAndClickOnPageWithCSP() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.addHeader("Content-Security-Policy", "default-src 'self'; script-src 'self';");
                res.getOutputStream().println(
                        "<html>\n" +
                                "<head>\n" +
                                "</head>\n" +
                                "<body>\n" +
                                "<input id='username' />\n" +
                                "</body>\n" +
                                "</html>");
            }
        });

        // Get Driver Instance
        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        WebElement element = d.findElement(By.id("username"));
        element.sendKeys("jesg");
        element.click();
        try {
            ((JavascriptExecutor) d).executeScript("1+1");
            fail("we should not be able to eval javascript on csp page");
        } catch (WebDriverException e) {}
    }

}
