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

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Set;

import static org.junit.Assert.*;

public class WindowHandlesTest extends BaseTestWithServer {
    @Test
    public void enumerateWindowHandles() {
        WebDriver d = getDriver();

        // Didn't open any page yet: no Window Handles yet
        Set<String> whandles = d.getWindowHandles();
        assertEquals(whandles.size(), 1);

        // Open Google and count the Window Handles: there should be at least 1
        d.get("http://www.google.com");
        whandles = d.getWindowHandles();
        assertEquals(whandles.size(), 1);
    }

    @Test
    public void enumerateWindowHandle() {
        WebDriver d = getDriver();

        // Didn't open any page yet: no Window Handles yet
        String whandle = d.getWindowHandle();
        assertFalse(whandle.isEmpty());
    }

    @Test
    public void openPopupAndGetCurrentUrl() throws InterruptedException {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println("<html>" +
                        "<head>" +
                        "<script language=\"javascript\" type=\"text/javascript\">\n" +
                        "function openProjectPopup(url) {\n" +
                        "    var popWidth  = 1024;\n" +
                        "    var leftX  = (screen.width) ? (screen.width-popWidth)/2 : 0;\n" +
                        "    var height = screen.availHeight;\n" +
                        "    var win = window.open(url, \"projectPopup\", \"left=\"+leftX+\",top=0,width=\"+popWidth+\",height=\"+height+\",location=yes,menubar=no,resizable=yes,status=no,scrollbars=yes\");\n" +
                        "    win.location.href='http://www.jnto.go.jp/'; //put a link to a slow loading webpage here\n" +
                        "    win.focus();\n" +
                        "}\n" +
                        "</script>\n" +
                        "</head>\n" +
                        "   <body>\n" +
                        "       <a href=\"popup.htm\" onclick=\"return openProjectPopup('popup.htm')\">Link to popup</a>" +
                        "   </body>\n" +
                        "</html>");
            }
        });

        // Load page
        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        // Click on link that will cause popup to be created
        d.findElement(By.xpath("//a")).click();
        // Switch to new popup
        String popupHandle = (String)d.getWindowHandles().toArray()[1];
        d.switchTo().window(popupHandle);

        assertTrue(d.getTitle().contains("Japan"));
    }
}
