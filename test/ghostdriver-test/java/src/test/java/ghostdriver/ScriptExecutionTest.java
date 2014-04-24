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

import org.junit.Ignore;
import org.junit.Test;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class ScriptExecutionTest extends BaseTest {
    @Test
    public void findGoogleInputFieldInjectingJavascript() {
        WebDriver d = getDriver();
        d.get("http://www.google.com");
        WebElement e = (WebElement)((JavascriptExecutor) d).executeScript(
                "return document.querySelector(\"[name='\"+arguments[0]+\"']\");",
                "q");
        assertNotNull(e);
        assertEquals("input", e.getTagName().toLowerCase());
    }

    @Test
    public void setTimeoutAsynchronously() {
        WebDriver d = getDriver();
        d.get("http://www.google.com");
        String res = (String)((JavascriptExecutor) d).executeAsyncScript(
                "window.setTimeout(arguments[arguments.length - 1], arguments[0], 'done');",
                1000);
        assertEquals("done", res);
    }

    @Test
    public void shouldBeAbleToPassMultipleArgumentsToAsyncScripts() {
        WebDriver d = getDriver();
        d.manage().timeouts().setScriptTimeout(0, TimeUnit.MILLISECONDS);
        d.get("http://www.google.com/");
        Number result = (Number) ((JavascriptExecutor) d).executeAsyncScript(
                "arguments[arguments.length - 1](arguments[0] + arguments[1]);",
                1,
                2);
        assertEquals(3, result.intValue());

        // Verify that a future navigation does not cause the driver to have problems.
        d.get("http://www.google.com/");
    }

    @Test
    public void shouldBeAbleToExecuteMultipleAsyncScriptsSequentially() {
        WebDriver d = getDriver();
        d.manage().timeouts().setScriptTimeout(0, TimeUnit.MILLISECONDS);
        d.get("http://www.google.com/");
        Number numericResult = (Number) ((JavascriptExecutor) d).executeAsyncScript(
                "arguments[arguments.length - 1](123);");
        assertEquals(123, numericResult.intValue());
        String stringResult = (String) ((JavascriptExecutor) d).executeAsyncScript(
                "arguments[arguments.length - 1]('abc');");
        assertEquals("abc", stringResult);
    }

    @Ignore("Known issue #140 - see https://github.com/detro/ghostdriver/issues/140)")
    @Test
    public void shouldBeAbleToExecuteMultipleAsyncScriptsSequentiallyWithNavigation() {
        // NOTE: This test is supposed to fail!
        // It's a reminder that there is some internal issue in PhantomJS still to address.

        WebDriver d = getDriver();
        d.manage().timeouts().setScriptTimeout(0, TimeUnit.MILLISECONDS);

        d.get("http://www.google.com/");
        Number numericResult = (Number) ((JavascriptExecutor) d).executeAsyncScript(
                "arguments[arguments.length - 1](123);");
        assertEquals(123, numericResult.intValue());

        d.get("http://www.google.com/");
        String stringResult = (String) ((JavascriptExecutor) d).executeAsyncScript(
                "arguments[arguments.length - 1]('abc');");
        assertEquals("abc", stringResult);

        // Verify that a future navigation does not cause the driver to have problems.
        d.get("http://www.google.com/");
    }

    @Ignore("Known issue #140 - see https://github.com/detro/ghostdriver/issues/140)")
    @Test
    public void executeAsyncScriptMultipleTimesWithoutCrashing() {
        // NOTE: This test is supposed to fail!
        // It's a reminder that there is some internal issue in PhantomJS still to address.

        WebDriver d = getDriver();

        String hello = null;
        try {
            hello = URLEncoder.encode("<h1>hello</h1>", "UTF-8");
        } catch (UnsupportedEncodingException uee) {
            fail();
        }

        for (int i = 1; i < 5; ++i) {
            d.get("data:text/html;content-type=utf-8,"+hello);
            String h = (String)((JavascriptExecutor) d).executeAsyncScript("arguments[arguments.length - 1]('hello')");
            assertEquals("hello", h);
        }
    }
}
