package ghostdriver;

import org.junit.Test;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.NoSuchWindowException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.remote.SessionNotFoundException;

import java.net.MalformedURLException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

public class SessionBasicTest extends BaseTest {

    @Test(expected = SessionNotFoundException.class)
    public void quitShouldTerminatePhantomJSProcess() throws MalformedURLException {
        // Get Driver Instance
        WebDriver d = getDriver();
        d.navigate().to("about:blank");

        // Quit the driver, that will cause the process to close
        d.quit();

        // Throws "SessionNotFoundException", because no process is actually left to respond
        d.getWindowHandle();
    }

    @Test(expected = NoSuchWindowException.class)
    public void closeShouldNotTerminatePhantomJSProcess() throws MalformedURLException {
        // By default, 1 window is created when Driver is launched
        WebDriver d = getDriver();
        assertEquals(1, d.getWindowHandles().size());

        // Check the number of windows
        d.navigate().to("about:blank");
        assertEquals(1, d.getWindowHandles().size());

        // Create a new window
        ((JavascriptExecutor) d).executeScript("window.open('http://www.google.com','google');");
        assertEquals(2, d.getWindowHandles().size());

        // Close 1 window and check that 1 is left
        d.close();
        assertEquals(1, d.getWindowHandles().size());

        // Switch to that window
        d.switchTo().window("google");
        assertNotNull(d.getWindowHandle());

        // Close the remaining window and check now there are no windows available
        d.close();
        assertEquals(0, d.getWindowHandles().size());

        // This should throw a "NoSuchWindowException": the Driver is still running, but no Session/Window are left
        d.getWindowHandle();
    }

}
