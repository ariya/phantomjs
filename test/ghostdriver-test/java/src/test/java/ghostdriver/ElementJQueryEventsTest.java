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
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.WebDriver;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Arrays;

import static org.junit.Assert.assertTrue;

@RunWith(value = Parameterized.class)
public class ElementJQueryEventsTest extends BaseTestWithServer {

    @Parameters(name = "jQuery Version: {0}")
    public static Iterable<Object[]> data() {
        return Arrays.asList(new Object[][]{
                {"2.0.3"}, {"2.0.2"}, {"2.0.1"}, {"2.0.0"},
                {"1.10.2"}, {"1.10.1"}, {"1.10.0"},
                {"1.9.1"}, {"1.9.0"},
                {"1.8.3"}, {"1.8.2"}, {"1.8.1"}, {"1.8.0"},
                {"1.7.2"}, //{"1.7.1"}, {"1.7.0"},
                {"1.6.4"}, //{"1.6.3"}, {"1.6.2"}, {"1.6.1"}, {"1.6.0"},
                {"1.5.2"}, //{"1.5.1"}, {"1.5.0"},
                {"1.4.4"}, //{"1.4.3"}, {"1.4.2"}, {"1.4.1"}, {"1.4.0"},
                {"1.3.2"}, //{"1.3.1"}, {"1.3.0"},
                {"1.2.6"}, //{"1.2.3"}
        });
    }

    private String mJqueryVersion;

    public ElementJQueryEventsTest(String jQueryVersion) {
        mJqueryVersion = jQueryVersion;
    }

    @Test
    public void shouldBeAbleToClickAndEventsBubbleUpUsingJquery() {
        final String buttonId = "clickme";

        server.setHttpHandler("GET", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                res.getOutputStream().println(
                        "<html>\n" +
                                "<head>\n" +
                                "<script src=\"//ajax.googleapis.com/ajax/libs/jquery/" + mJqueryVersion + "/jquery.min.js\"></script>\n" +
                                "<script type=\"text/javascript\">\n" +
                                "   var clicked = false;" +
                                "   $(document).ready(function() {" +
                                "       $('#" + buttonId + "').bind('click', function(e) {" +
                                "           clicked = true;" +
                                "       });" +
                                "   });\n" +
                                "</script>\n" +
                                "</head>\n" +
                                "<body>\n" +
                                "    <a href='#' id='" + buttonId + "'>click me</a>\n" +
                                "</body>\n" +
                                "</html>");
            }
        });

        WebDriver d = getDriver();
        d.get(server.getBaseUrl());

        // Click on the link inside the page
        d.findElement(By.id(buttonId)).click();

        // Check element was clicked as expected
        assertTrue((Boolean)((JavascriptExecutor)d).executeScript("return clicked;"));
    }
}
