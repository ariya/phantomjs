package ghostdriver;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.openqa.selenium.Cookie;
import org.openqa.selenium.WebDriver;

import java.util.Set;

import static org.junit.Assert.assertFalse;

public class IsolatedSessionTest extends BaseTest {
    // New Session Cookies will be stored in here
    private String url = "http://www.google.com";
    private Set<Cookie> sessionCookies;

    @Before
    public void createSession() throws Exception {
        disableAutoQuitDriver();

        WebDriver d = getDriver();
        d.get(url);

        // Grab set of session cookies
        sessionCookies = d.manage().getCookies();

        // Manually quit the current Driver and create a new one
        d.quit();
        prepareDriver();
    }

    @Test
    public void shouldCreateASeparateSessionWithEveryNewDriverInstance() {
        WebDriver d = getDriver();
        d.get(url);

        // Grab NEW set of session cookies
        Set<Cookie> newSessionCookies = d.manage().getCookies();

        // No cookie of the new Session can be found in the cookies of the old Session
        for (Cookie c : sessionCookies) {
            assertFalse(newSessionCookies.contains(c));
        }
        // No cookie of the old Session can be found in the cookies of the new Session
        for (Cookie c : newSessionCookies) {
            assertFalse(sessionCookies.contains(c));
        }
    }

    @After
    public void quitDriver() {
        getDriver().quit();
    }
}
