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

import ghostdriver.server.HttpRequestCallback;
import org.junit.Test;
import org.openqa.selenium.*;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;
import java.io.*;
// import org.apache.commons.fileupload.FileItem;
// import org.apache.commons.fileupload.FileUploadException;
// import org.apache.commons.fileupload.disk.DiskFileItemFactory;
// import org.apache.commons.io.IOUtils;

import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class VisibilityTest extends BaseTestWithServer {

    @Test
    public void testShouldNotBeAbleToTypeToAnElementThatIsNotDisplayed() {

        WebDriver d = getDriver();
        d.get(server.getBaseUrl() + "/common/send_keys_visibility.html");

        WebElement elem = d.findElement(By.id("unclickable"));

        try {
            elem.sendKeys("this is not visible");
            fail("You should not be able to send keyboard input to an invisible element");
        } catch (InvalidElementStateException e) {
        }

        assertFalse(elem.getAttribute("value").equals("this is not visible"));
        assertTrue(d.findElement(By.id("log")).getText().trim().equals("Log:"));
    }

    @Test
    public void testShouldNotBeAbleToTypeToAFileInputElementThatIsNotDisplayed() throws IOException {

        // Create the test file for uploading
        File testFile = File.createTempFile("webdriver", "tmp");
        testFile.deleteOnExit();
        BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(
            new FileOutputStream(testFile.getAbsolutePath()), "utf-8"));
        writer.write("Hello");
        writer.close();

        WebDriver d = getDriver();
        d.get(server.getBaseUrl() + "/common/send_keys_visibility.html");

        WebElement elem = d.findElement(By.id("unclickable_file"));

        try {
            elem.sendKeys(testFile.getAbsolutePath());
            fail("You should not be able to send keyboard input to an invisible element");
        } catch (ElementNotVisibleException e) {
        }

        assertFalse(elem.getAttribute("value").equals(testFile.getAbsolutePath()));
        assertTrue(d.findElement(By.id("log")).getText().trim().equals("Log:"));
    }
}
