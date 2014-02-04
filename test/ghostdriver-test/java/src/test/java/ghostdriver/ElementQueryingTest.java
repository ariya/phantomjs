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
import org.openqa.selenium.*;
import org.openqa.selenium.internal.Locatable;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class ElementQueryingTest extends BaseTestWithServer {
    @Test
    public void checkAttributesOnGoogleSearchBox() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        WebElement el = d.findElement(By.cssSelector("input[name*='q']"));

        assertTrue(el.getAttribute("name").toLowerCase().contains("q"));
        assertTrue(el.getAttribute("type").toLowerCase().contains("text"));
        assertTrue(el.getAttribute("style").length() > 0);
        assertTrue(el.getAttribute("type").length() > 0);
    }

    @Test
    public void checkLocationAndSizeOfBingSearchBox() {
        WebDriver d = getDriver();
        d.manage().timeouts().pageLoadTimeout(20, TimeUnit.SECONDS);

        d.get("http://www.bing.com");
        WebElement searchBox = d.findElement(By.cssSelector("input[name*='q']"));

        assertTrue(searchBox.getCssValue("color").contains("rgb(0, 0, 0)") || searchBox.getCssValue("color").contains("rgba(0, 0, 0, 1)"));
        assertEquals("", searchBox.getAttribute("value"));
        assertEquals("input", searchBox.getTagName());
        assertEquals(true, searchBox.isEnabled());
        assertEquals(true, searchBox.isDisplayed());
        assertTrue(searchBox.getLocation().getX() >= 200);
        assertTrue(searchBox.getLocation().getY() >= 100);
        assertTrue(searchBox.getSize().getWidth() >= 350);
        assertTrue(searchBox.getSize().getHeight() >= 20);
    }

    @Test
    public void scrollElementIntoView() {
        WebDriver d = getDriver();

        d.get("https://developer.mozilla.org/en/CSS/Attribute_selectors");
        WebElement aboutGoogleLink = d.findElement(By.partialLinkText("About MDN"));
        Point locationBeforeScroll = aboutGoogleLink.getLocation();
        Point locationAfterScroll = ((Locatable) aboutGoogleLink).getCoordinates().inViewPort();

        assertTrue(locationBeforeScroll.x >= locationAfterScroll.x);
        assertTrue(locationBeforeScroll.y >= locationAfterScroll.y);
    }

    @Test
    public void getTextFromDifferentLocationsOfDOMTree() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<html><body>" +
                        "<div class=\"item\">\n" +
                        "    <span class=\"item-title\">\n" +
                        "        <a href=\"#\">\n" +
                        "             <h1>The Title of The Item</h1>\n" +
                        "        </a>\n" +
                        "     </span>\n" +
                        "     <div>\n" +
                        "         (Loads of other stuff)\n" +
                        "     </div>\n" +
                        "</div>" +
                        "</body></html>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        assertEquals("The Title of The Item\n(Loads of other stuff)", d.findElement(By.className("item")).getText());
        assertEquals("The Title of The Item", d.findElement(By.className("item")).findElement(By.tagName("h1")).getText());
        assertEquals("The Title of The Item", d.findElement(By.className("item")).findElement(By.tagName("a")).getText());
        assertEquals("The Title of The Item", d.findElement(By.className("item")).findElement(By.className("item-title")).getText());
    }

    @Test(expected = InvalidElementStateException.class)
    public void throwExceptionWhenInteractingWithInvisibleElement() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<!DOCTYPE html>" +
                        "<html>" +
                        "    <head>\n" +
                        "        <title>test</title>\n" +
                        "    </head>\n" +
                        "    <body>\n" +
                        "        <input type=\"text\" id=\"visible\">\n" +
                        "        <input type=\"text\" id=\"invisible\" style=\"display: none;\">\n" +
                        "    </body>" +
                        "</html>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        WebElement visibleInput = d.findElement(By.id("visible"));
        WebElement invisibleInput = d.findElement(By.id("invisible"));

        String textToType = "text to type";
        visibleInput.sendKeys(textToType);
        assertEquals(textToType, visibleInput.getAttribute("value"));

        invisibleInput.sendKeys(textToType);
    }
}
