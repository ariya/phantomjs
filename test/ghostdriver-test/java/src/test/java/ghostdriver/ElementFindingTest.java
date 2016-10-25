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

import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class ElementFindingTest extends BaseTestWithServer {
    @Test
    public void findChildElement() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<div id=\"y-masthead\">" +
                        "<input type=\"text\" name=\"t\" />" +
                        "<input type=\"hidden\" name=\"h\" value=\"v\" />" +
                        "</div>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        WebElement parent = d.findElement(By.id("y-masthead"));

        assertNotNull(parent.findElement(By.name("t")));
    }

    @Test
    public void findChildElements() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<div id=\"y-masthead\">" +
                        "<input type=\"text\" name=\"t\" />" +
                        "<input type=\"hidden\" name=\"h\" value=\"v\" />" +
                        "</div>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        WebElement parent = d.findElement(By.id("y-masthead"));

        List<WebElement> children = parent.findElements(By.tagName("input"));
        assertEquals(2, children.size());
    }

    @Test
    public void findMultipleElements() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<div id=\"y-masthead\">" +
                        "<input type=\"text\" name=\"t\" />" +
                        "<input type=\"hidden\" name=\"h\" value=\"v\" />" +
                        "<input type=\"button\" name=\"b\" value=\"button\" />" +
                        "</div>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        assertEquals(3, d.findElements(By.tagName("input")).size());
    }

    @Test
    public void findNoElementsMeetingCriteria() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        List<WebElement> els = d.findElements(By.name("noElementWithThisName"));

        assertEquals(0, els.size());
    }

    @Test
    public void findNoChildElementsMeetingCriteria() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        WebElement parent = d.findElement(By.name("q"));

        List<WebElement> children = parent.findElements(By.tagName("input"));

        assertEquals(0, children.size());
    }

    @Test
    public void findActiveElement() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        WebElement inputField = d.findElement(By.cssSelector("input[name='q']"));
        WebElement active = d.switchTo().activeElement();

        assertEquals(inputField.getTagName(), active.getTagName());
        assertEquals(inputField.getLocation(), active.getLocation());
        assertEquals(inputField.hashCode(), active.hashCode());
        assertEquals(inputField.getText(), active.getText());
        assertTrue(inputField.equals(active));
    }

    @Test(expected = NoSuchElementException.class)
    public void failToFindNonExistentElement() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        WebElement inputField = d.findElement(By.cssSelector("input[name='idontexist']"));
    }

    @Test(expected = InvalidSelectorException.class)
    public void failFindElementForInvalidXPathLocator() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        WebElement inputField = d.findElement(By.xpath("this][isnot][valid"));
    }

    @Test(expected = InvalidSelectorException.class)
    public void failFindElementsForInvalidXPathLocator() {
        WebDriver d = getDriver();

        d.get("http://www.google.com");
        List<WebElement> inputField = d.findElements(By.xpath("this][isnot][valid"));
    }

    @Test
    public void findElementWithImplicitWait() {
        WebDriver d = getDriver();

        d.get("about:blank");
        String injectLink = "document.body.innerHTML = \"<a onclick=\\\"setTimeout(function(){" +
                    "var e=document.createElement('span');" +
                    "e.innerText='test';" +
                    "e.id='testing'+document.body.childNodes.length;" +
                    "document.body.appendChild(e);" +
                    "}, 750)\\\"" +
                " id='add'>add</a>\"";
        ((JavascriptExecutor)d).executeScript(injectLink);
        WebElement add = d.findElement(By.id("add"));

        // DO NOT WAIT when looking for an element
        d.manage().timeouts().implicitlyWait(0, TimeUnit.SECONDS);

        // Add element
        add.click();
        // Check element is not there yet
        try {
            d.findElement(By.id("testing1"));
            throw new RuntimeException("expected NoSuchElementException");
        } catch (NoSuchElementException nse) { /* nothing to do */ }

        // DO WAIT 1 SECOND before giving up while looking for an element
        d.manage().timeouts().implicitlyWait(1, TimeUnit.SECONDS);
        // Add element
        add.click();
        // Check element is there
        assertNotNull(d.findElement(By.id("testing2")));

        // DO WAIT 0.5 SECONDS before giving up while looking for an element
        d.manage().timeouts().implicitlyWait(500, TimeUnit.MILLISECONDS);
        // Add element
        add.click();

        // Check element is not there yet
        try {
            d.findElement(By.id("testing3"));
            throw new RuntimeException("expected NoSuchElementException");
        } catch (NoSuchElementException nse) { /* nothing to do */ }
    }

    @Test
    public void findElementsWithImplicitWait() {
        WebDriver d = getDriver();

        d.get("about:blank");
        String injectLink = "document.body.innerHTML = \"<a onclick=\\\"setTimeout(function(){" +
                    "   var e=document.createElement('span');" +
                    "   e.innerText='test';" +
                    "   e.id='testing'+document.body.childNodes.length;" +
                    "   document.body.appendChild(e);" +
                    "}, 750);\\\" " +
                " id='add'>add</a>\"";
        ((JavascriptExecutor)d).executeScript(injectLink);
        WebElement add = d.findElement(By.id("add"));

        // DO NOT WAIT while looking for an element
        d.manage().timeouts().implicitlyWait(0, TimeUnit.SECONDS);
        // Add element
        add.click();
        // Check element is not there yet
        assertEquals(0, d.findElements(By.id("testing1")).size());

        // DO WAIT 3 SEC when looking for an element
        d.manage().timeouts().implicitlyWait(3, TimeUnit.SECONDS);
        // Add element
        add.click();
        // Check element is there
        assert(d.findElements(By.tagName("span")).size() >= 1 && d.findElements(By.tagName("span")).size() <= 2);
    }

    @Test
    public void findElementViaXpathLocator() {
        // Define HTTP response for test
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                ServletOutputStream out = res.getOutputStream();
                out.println("<html><body>" +
                        "<button class='login main btn'>Login Button</button>" +
                        "</body></html>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        WebElement loginButton = d.findElement(By.xpath("//button[contains(@class, 'login')]"));
        assertNotNull(loginButton);
        assertTrue(loginButton.getText().toLowerCase().contains("login"));
        assertEquals("button", loginButton.getTagName().toLowerCase());
    }
}
