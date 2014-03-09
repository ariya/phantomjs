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

import ghostdriver.server.HttpRequestCallback;
import org.junit.Test;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.interactions.Actions;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;

public class MouseCommandsTest extends BaseTestWithServer {
    @Test
    public void move() {
        WebDriver d = getDriver();
        Actions actionBuilder = new Actions(d);

        d.get("http://www.duckduckgo.com");

        // Move mouse by x,y
        actionBuilder.moveByOffset(100, 100).build().perform();
        // Move mouse on a given element
        actionBuilder.moveToElement(d.findElement(By.id("logo_homepage"))).build().perform();
        // Move mouse on a given element, by x,y relative coordinates
        actionBuilder.moveToElement(d.findElement(By.id("logo_homepage")), 50, 50).build().perform();
    }

    @Test
    public void clickAndRightClick() {
        WebDriver d = getDriver();
        Actions actionBuilder = new Actions(d);

        d.get("http://www.duckduckgo.com");

        // Left click
        actionBuilder.click().build().perform();
        // Right click
        actionBuilder.contextClick(null).build().perform();
        // Right click on the logo (it will cause a "/moveto" before clicking
        actionBuilder.contextClick(d.findElement(By.id("logo_homepage"))).build().perform();
    }

    @Test
    public void doubleClick() {
        WebDriver d = getDriver();
        Actions actionBuilder = new Actions(d);

        d.get("http://www.duckduckgo.com");

        // Double click
        actionBuilder.doubleClick().build().perform();
        // Double click on the logo
        actionBuilder.doubleClick(d.findElement(By.id("logo_homepage"))).build().perform();
    }

    @Test
    public void clickAndHold() {
        WebDriver d = getDriver();
        Actions actionBuilder = new Actions(d);

        d.get("http://www.duckduckgo.com");

        // Hold, then release
        actionBuilder.clickAndHold().build().perform();
        actionBuilder.release();
        // Hold on the logo, then release
        actionBuilder.clickAndHold(d.findElement(By.id("logo_homepage"))).build().perform();
        actionBuilder.release();
    }

    @Test
    public void handleClickWhenOnClickInlineCodeFails() {
        // Define HTTP response for test
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<html>" +
                        "<head>" +
                        "<script>\n" +
                        "function functionThatHasErrors() {\n" +
                        "    a.callSomeMethodThatDoesntExist();\n" +
                        "}\n" +
                        "function validFunction() {\n" +
                        "    window.location = 'http://google.com';\n" +
                        "}\n" +
                        "</script>\n" +
                        "</head>" +
                        "<body>" +
                        "\n" +
                        "<a href=\"javascript:;\" onclick=\"validFunction();functionThatHasErrors();\">Click me</a>" +
                        "</body>" +
                        "</html>");
            }
        });

        // Navigate to local server
        WebDriver d = getDriver();
        d.navigate().to(server.getBaseUrl());

        WebElement el = d.findElement(By.linkText("Click me"));
        el.click();
    }
}
