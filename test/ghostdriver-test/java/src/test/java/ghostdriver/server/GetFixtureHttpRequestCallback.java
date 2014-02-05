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
