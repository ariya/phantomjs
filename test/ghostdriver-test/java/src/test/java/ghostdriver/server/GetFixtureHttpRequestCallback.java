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

package ghostdriver.server;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.util.logging.Logger;

public class GetFixtureHttpRequestCallback implements HttpRequestCallback {

    private static final Logger LOG = Logger.getLogger(GetFixtureHttpRequestCallback.class.getName());

    private static final String FIXTURE_PATH = "../fixtures";

    @Override
    public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
        // Construct path to the file
        Path filePath = FileSystems.getDefault().getPath(FIXTURE_PATH, req.getPathInfo());

        // If the file exists
        if (filePath.toFile().exists()) {
            try {
                // Set Content Type
                res.setContentType(filePathToMimeType(filePath.toString()));
                // Read and write to response
                Files.copy(filePath, res.getOutputStream());

                return;
            } catch (NoSuchFileException nsfe) {
                LOG.warning(nsfe.getClass().getName());
            } catch (IOException ioe) {
                LOG.warning(ioe.getClass().getName());
            } catch (RuntimeException re) {
                LOG.warning(re.getClass().getName());
            }
        }

        LOG.warning("Fixture NOT FOUND: "+filePath);
        res.sendError(404); //< Not Found
    }

    private static String filePathToMimeType(String filePath) {
        if (filePath.endsWith(".js")) {
            return "application/javascript";
        }
        if (filePath.endsWith(".json")) {
            return "text/json";
        }
        if (filePath.endsWith(".png")) {
            return "image/png";
        }
        if (filePath.endsWith(".jpg")) {
            return "image/jpg";
        }
        if (filePath.endsWith(".gif")) {
            return "image/gif";
        }

        return "text/html";
    }
}
