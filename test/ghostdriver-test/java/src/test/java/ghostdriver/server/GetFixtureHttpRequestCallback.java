package ghostdriver.server;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;

public class GetFixtureHttpRequestCallback implements HttpRequestCallback {

    private static final String FIXTURE_PATH = "fixtures";

    @Override
    public void call(HttpServletRequest req, HttpServletResponse res) throws IOException {
        String path = req.getPathInfo();

        if (null != path) {
            try {
                // Construct path to the file
                Path filePath = FileSystems.getDefault().getPath(FIXTURE_PATH, path);
                // Set Content Type
                res.setContentType(filePathToMimeType(filePath.toString()));
                // Read and write to response
                Files.copy(filePath, res.getOutputStream());

                return;
            } catch (RuntimeException re) {
                // Not Found. Handled below.
            } catch (NoSuchFileException nsfe) {
                // Not Found. Handled below.
            } catch (IOException ioe) {
                // Not Found. Handled below.
            }
        }

        System.out.println("Fixture NOT FOUND");
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
