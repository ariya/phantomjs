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

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.chrome.ChromeDriver;
import org.openqa.selenium.firefox.FirefoxDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriver;
import org.openqa.selenium.phantomjs.PhantomJSDriverService;
import org.openqa.selenium.remote.DesiredCapabilities;
import org.openqa.selenium.remote.RemoteWebDriver;

import java.io.FileReader;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Properties;

/**
 * Tests base class.
 * Takes care of initialising the Remote WebDriver
 */
public abstract class BaseTest {
    private WebDriver mDriver                      = null;
    private boolean mAutoQuitDriver                = true;

    private static final String CONFIG_FILE        = "../config.ini";
    private static final String DRIVER_FIREFOX     = "firefox";
    private static final String DRIVER_CHROME      = "chrome";
    private static final String DRIVER_PHANTOMJS   = "phantomjs";

    protected static Properties sConfig;
    protected static DesiredCapabilities sCaps;

    private static boolean isUrl(String urlString) {
        try {
            new URL(urlString);
            return true;
        } catch (MalformedURLException mue) {
            return false;
        }
    }

    @BeforeClass
    public static void configure() throws IOException {
        // Read config file
        sConfig = new Properties();
        sConfig.load(new FileReader(CONFIG_FILE));

        // Prepare capabilities
        sCaps = new DesiredCapabilities();
        sCaps.setJavascriptEnabled(true);
        sCaps.setCapability("takesScreenshot", false);

        String driver = sConfig.getProperty("driver", DRIVER_PHANTOMJS);

        // Fetch PhantomJS-specific configuration parameters
        if (driver.equals(DRIVER_PHANTOMJS)) {
            // "phantomjs_exec_path"
            if (sConfig.getProperty("phantomjs_exec_path") != null) {
                sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_EXECUTABLE_PATH_PROPERTY, sConfig.getProperty("phantomjs_exec_path"));
            } else {
                throw new IOException(String.format("Property '%s' not set!", PhantomJSDriverService.PHANTOMJS_EXECUTABLE_PATH_PROPERTY));
            }
            // "phantomjs_driver_path"
            if (sConfig.getProperty("phantomjs_driver_path") != null) {
                System.out.println("Test will use an external GhostDriver");
                sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_GHOSTDRIVER_PATH_PROPERTY, sConfig.getProperty("phantomjs_driver_path"));
            } else {
                System.out.println("Test will use PhantomJS internal GhostDriver");
            }
        }

        // Disable "web-security", enable all possible "ssl-protocols" and "ignore-ssl-errors" for PhantomJSDriver
//        sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_CLI_ARGS, new String[] {
//            "--web-security=false",
//            "--ssl-protocol=any",
//            "--ignore-ssl-errors=true"
//        });
        ArrayList<String> cliArgsCap = new ArrayList<String>();
        cliArgsCap.add("--web-security=false");
        cliArgsCap.add("--ssl-protocol=any");
        cliArgsCap.add("--ignore-ssl-errors=true");
        sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_CLI_ARGS, cliArgsCap);

        // Control LogLevel for GhostDriver, via CLI arguments
        sCaps.setCapability(PhantomJSDriverService.PHANTOMJS_GHOSTDRIVER_CLI_ARGS, new String[] {
            "--logLevel=" + (sConfig.getProperty("phantomjs_driver_loglevel") != null ? sConfig.getProperty("phantomjs_driver_loglevel") : "INFO")
        });
    }

    @Before
    public void prepareDriver() throws Exception
    {
        // Which driver to use? (default "phantomjs")
        String driver = sConfig.getProperty("driver", DRIVER_PHANTOMJS);

        // Start appropriate Driver
        if (isUrl(driver)) {
            sCaps.setBrowserName("phantomjs");
            mDriver = new RemoteWebDriver(new URL(driver), sCaps);
        } else if (driver.equals(DRIVER_FIREFOX)) {
            mDriver = new FirefoxDriver(sCaps);
        } else if (driver.equals(DRIVER_CHROME)) {
            mDriver = new ChromeDriver(sCaps);
        } else if (driver.equals(DRIVER_PHANTOMJS)) {
            mDriver = new PhantomJSDriver(sCaps);
        }
    }

    protected WebDriver getDriver() {
        return mDriver;
    }

    protected void disableAutoQuitDriver() {
        mAutoQuitDriver = false;
    }

    protected void enableAutoQuitDriver() {
        mAutoQuitDriver = true;
    }

    protected boolean isAutoQuitDriverEnabled() {
        return mAutoQuitDriver;
    }

    @After
    public void quitDriver() {
        if (mAutoQuitDriver && mDriver != null) {
            mDriver.quit();
            mDriver = null;
        }
    }
}
