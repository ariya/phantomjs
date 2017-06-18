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
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.NoSuchWindowException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.NoSuchSessionException;

import java.net.MalformedURLException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

public class SessionBasicTest extends BaseTest {

    @Test(expected = NoSuchSessionException.class)
    public void quitShouldTerminatePhantomJSProcess() throws MalformedURLException {
        // Get Driver Instance
        WebDriver d = getDriver();
        d.navigate().to("about:blank");

        // Quit the driver, that will cause the process to close
        d.quit();

        // Throws "SessionNotFoundException", because no process is actually left to respond
        d.getWindowHandle();
    }

    @Test(expected = NoSuchWindowException.class)
    public void closeShouldNotTerminatePhantomJSProcess() throws MalformedURLException {
        // By default, 1 window is created when Driver is launched
        WebDriver d = getDriver();
        assertEquals(1, d.getWindowHandles().size());

        // Check the number of windows
        d.navigate().to("about:blank");
        assertEquals(1, d.getWindowHandles().size());

        // Create a new window
        ((JavascriptExecutor) d).executeScript("window.open('http://www.google.com','google');");
        assertEquals(2, d.getWindowHandles().size());

        // Close 1 window and check that 1 is left
        d.close();
        assertEquals(1, d.getWindowHandles().size());

        // Switch to that window
        d.switchTo().window("google");
        assertNotNull(d.getWindowHandle());

        // Close the remaining window and check now there are no windows available
        d.close();
        assertEquals(0, d.getWindowHandles().size());

        // This should throw a "NoSuchWindowException": the Driver is still running, but no Session/Window are left
        d.getWindowHandle();
    }

}
