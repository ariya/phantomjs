/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2017, Jason Gowan
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
import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriverService;
import org.openqa.selenium.support.ui.WebDriverWait;
import org.openqa.selenium.support.ui.ExpectedConditions;

public class UnhandledAlertDismissTest extends BaseTestWithServer {


    @Override
    public void prepareDriver() throws Exception {
        sCaps.setCapability("unhandledPromptBehavior", "dismiss");

        super.prepareDriver();
    }

    @Test
    public void canHandleAlert() {
        // Get Driver Instance
        WebDriver d = getDriver();

        d.get(server.getBaseUrl() + "/common/alerts.html");
        d.findElement(By.id("alert2")).click();

        new WebDriverWait(d, 5).until(ExpectedConditions.presenceOfElementLocated(By.id("cheese-child")));
    }

    @Test
    public void canHandleConfirm() {
        // Get Driver Instance
        WebDriver d = getDriver();

        d.get(server.getBaseUrl() + "/common/alerts.html");
        WebElement elem = d.findElement(By.id("confirm2"));
        elem.click();

        new WebDriverWait(d, 5).until(ExpectedConditions.attributeToBe(elem, "value", "false"));
    }

    @Test
    public void canHandlePrompt() {
        // Get Driver Instance
        WebDriver d = getDriver();

        d.get(server.getBaseUrl() + "/common/alerts.html");
        WebElement elem = d.findElement(By.id("prompt2"));
        elem.click();

        new WebDriverWait(d, 5).until(ExpectedConditions.attributeToBe(elem, "value", "default value"));
    }
}
