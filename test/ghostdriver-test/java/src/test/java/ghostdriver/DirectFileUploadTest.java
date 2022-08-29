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
import org.apache.commons.fileupload.FileItem;
import org.apache.commons.fileupload.FileUploadException;
import org.apache.commons.fileupload.disk.DiskFileItemFactory;
import org.apache.commons.fileupload.servlet.ServletFileUpload;
import org.apache.commons.io.IOUtils;
import org.junit.Test;
import org.junit.Ignore;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.support.ui.ExpectedConditions;
import org.openqa.selenium.support.ui.WebDriverWait;
import org.openqa.selenium.phantomjs.PhantomJSDriver;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.*;
import java.util.List;

@Ignore
public class DirectFileUploadTest extends BaseTestWithServer {
    private static final String LOREM_IPSUM_TEXT = "lorem ipsum dolor sit amet";
    private static final String FILE_HTML = "<div>" + LOREM_IPSUM_TEXT + "</div>";

    @Test
    public void checkFileUploadCompletes() throws IOException {
        WebDriver d = getDriver();
        if (!(d instanceof PhantomJSDriver)) {
            // Skip this test if not using PhantomJS.
            // The command under test is only available when using PhantomJS
            return;
        }
        PhantomJSDriver phantom = (PhantomJSDriver)d;

        String buttonId = "upload";

        // Create the test file for uploading
        File testFile = File.createTempFile("webdriver", "tmp");
        testFile.deleteOnExit();
        BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(
            new FileOutputStream(testFile.getAbsolutePath()), "utf-8"));
        writer.write(FILE_HTML);
        writer.close();

        server.setHttpHandler("POST", new HttpRequestCallback() {
            @Override
            public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
                if (ServletFileUpload.isMultipartContent(req) && req.getPathInfo().endsWith("/upload")) {
                    // Create a factory for disk-based file items
                    DiskFileItemFactory factory = new DiskFileItemFactory(1024, new File(System.getProperty("java.io.tmpdir")));

                    // Create a new file upload handler
                    ServletFileUpload upload = new ServletFileUpload(factory);

                    // Parse the request
                    List<FileItem> items;
                    try {
                        items = upload.parseRequest(req);
                    } catch (FileUploadException fue) {
                        throw new IOException(fue);
                    }

                    res.setHeader("Content-Type", "text/html; charset=UTF-8");
                    InputStream is = items.get(0).getInputStream();
                    OutputStream os = res.getOutputStream();
                    IOUtils.copy(is, os);

                    os.write("<script>window.top.window.onUploadDone();</script>".getBytes());

                    IOUtils.closeQuietly(is);
                    IOUtils.closeQuietly(os);
                    return;
                }

                res.sendError(400);
            }
        });

        // Upload the temp file
        phantom.get(server.getBaseUrl() + "/common/upload.html");

        phantom.executePhantomJS("var page = this; page.uploadFile('input#"+ buttonId +"', '"+ testFile.getAbsolutePath() +"');");

        phantom.findElement(By.id("go")).submit();

        // Uploading files across a network may take a while, even if they're really small.
        // Wait for the loading label to disappear.
        WebDriverWait wait = new WebDriverWait(phantom, 10);
        wait.until(ExpectedConditions.invisibilityOfElementLocated(By.id("upload_label")));

        phantom.switchTo().frame("upload_target");

        wait = new WebDriverWait(phantom, 5);
        wait.until(ExpectedConditions.textToBePresentInElementLocated(By.xpath("//body"), LOREM_IPSUM_TEXT));

        // Navigate after file upload to verify callbacks are properly released.
        phantom.get("http://www.google.com/");
    }
}
