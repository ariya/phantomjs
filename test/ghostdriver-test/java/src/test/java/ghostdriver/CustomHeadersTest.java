/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2016, Jason Gowan
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

import org.junit.BeforeClass;
import org.junit.Test;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriverService;
import org.openqa.selenium.By;

import ghostdriver.server.HttpRequestCallback;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertEquals;

public class CustomHeadersTest extends BaseTestWithServer {

    private static final String CUSTOM_HEADER_NAME = "My-Custom-Header";
    private static final String CUSTOM_HEADER = "my value";

    @BeforeClass
    public static void setCustomHeaders() {
        sCaps.setCapability(
                "phantomjs.page.customHeaders.Accept-Encoding",
                "gzip, deflate"
                );
        sCaps.setCapability(
                "phantomjs.page.customHeaders."+CUSTOM_HEADER_NAME,
                CUSTOM_HEADER
        );
    }

    // regression test for detro/ghostdriver#489
    @Test
    public void testAcceptEncodingHeader() {
        WebDriver d = getDriver();
        d.get("https://cn.bing.com");
        assertFalse(d.getTitle().isEmpty());
    }

    @Test
    public void testCustomHeaders() {
        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println(
                        "<html>\n" +
                                "<head>\n" +
                                "</head>\n" +
                                "<body>\n" +
                                "<div name=\"" + CUSTOM_HEADER_NAME + "\" value=\"" + req.getHeader(CUSTOM_HEADER_NAME) + "\"></div>\n" +
                                "</body>\n" +
                                "</html>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());
        String actualCustomHeader = d.findElement(By.name(CUSTOM_HEADER_NAME)).getAttribute("value");
        assertEquals(CUSTOM_HEADER, actualCustomHeader);
    }
}
