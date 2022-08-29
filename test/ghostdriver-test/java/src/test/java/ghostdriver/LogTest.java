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
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.logging.LogEntries;
import org.openqa.selenium.logging.LogEntry;
import org.openqa.selenium.logging.LogType;
import org.openqa.selenium.logging.LoggingPreferences;
import org.openqa.selenium.remote.CapabilityType;
import java.util.logging.Level;
import org.junit.BeforeClass;

import java.util.Set;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class LogTest extends BaseTestWithServer {

    @BeforeClass
    public static void setCustomHeaders() {
        LoggingPreferences logPrefs = new LoggingPreferences();
        logPrefs.enable(LogType.BROWSER, Level.ALL);
        logPrefs.enable("har", Level.ALL);
        sCaps.setCapability(CapabilityType.LOGGING_PREFS, logPrefs);
    }

    @Test
    public void shouldReturnListOfAvailableLogs() {
        WebDriver d = getDriver();
        Set<String> logTypes = d.manage().logs().getAvailableLogTypes();

        if (d.getClass().getSimpleName().equals("PhantomJSDriver")) {
            // GhostDriver only has 3 log types...
            assertEquals(3, logTypes.size());
            // ... and "har" is one of them
            assertTrue(logTypes.contains("har"));
        } else {
            assertTrue(logTypes.size() >= 2);
        }
        assertTrue(logTypes.contains("client"));
        assertTrue(logTypes.contains("browser"));

    }

    @Test
    public void shouldReturnLogTypeBrowser() {
        WebDriver d = getDriver();
        d.get(server.getBaseUrl() + "/common/errors.html");
        // Throw 3 errors that are logged in the Browser's console
        WebElement throwErrorButton = d.findElement(By.cssSelector("input[type='button']"));
        throwErrorButton.click();
        throwErrorButton.click();
        throwErrorButton.click();

        // Retrieve and count the errors
        LogEntries logEntries = d.manage().logs().get("browser");
        assertEquals(3, logEntries.getAll().size());

        for (LogEntry logEntry : logEntries) {
            System.out.println(logEntry);
        }

        // Clears logs
        logEntries = d.manage().logs().get("browser");
        assertEquals(0, logEntries.getAll().size());
    }

    @Test
    public void shouldReturnLogTypeHar() {
        WebDriver d = getDriver();
        d.get(server.getBaseUrl() + "/common/iframes.html");

        LogEntries logEntries = d.manage().logs().get("har");
        for (LogEntry logEntry : logEntries) {
            System.out.println(logEntry);
        }

        String firstRequestMessage = logEntries.getAll().get(0).getMessage();
        String secondRequestMessage = d.manage().logs().get("har").getAll().get(0).getMessage();
        assertTrue(secondRequestMessage.length() < firstRequestMessage.length());
    }
}
