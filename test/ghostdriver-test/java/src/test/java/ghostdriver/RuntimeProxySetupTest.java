/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012-2014, Ivan De Marino <http://ivandemarino.me>
Copyright (c) 2014, Artem Koshelev <artkoshelev@gmail.com>
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

import com.github.detro.browsermobproxyclient.BMPCLocalLauncher;
import com.github.detro.browsermobproxyclient.BMPCProxy;
import com.github.detro.browsermobproxyclient.manager.BMPCLocalManager;
import com.google.gson.JsonObject;
import org.junit.*;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.remote.CapabilityType;

import java.net.URL;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

@RunWith(Parameterized.class)
public class RuntimeProxySetupTest extends BaseTestWithServer {
    private static BMPCLocalManager localProxyManager;
    private BMPCProxy proxy;
    private URL url;

    @Parameterized.Parameters(name = "URL requested through Proxy: {0}")
    public static Collection<URL[]> data() throws Exception {
        List<URL[]> requestedUrls = new ArrayList<URL[]>();
        requestedUrls.add(new URL[]{new URL("http://www.google.com/")});
        requestedUrls.add(new URL[]{new URL("http://ivandemarino.me/ghostdriver/")});
        return requestedUrls;
    }

    public RuntimeProxySetupTest(URL url) {
        this.url = url;
    }

    @BeforeClass
    public static void startProxyManager() {
        localProxyManager = BMPCLocalLauncher.launchOnRandomPort();
    }

    @Before
    public void createProxy() throws Exception {
        proxy = localProxyManager.createProxy();
        sCaps.setCapability(CapabilityType.PROXY, proxy.asSeleniumProxy());
        prepareDriver();
    }

    @Test
    public void requestsProcessedByProxy() {
        proxy.newHar(url.toString());

        WebDriver driver = getDriver();
        driver.navigate().to(url);

        JsonObject har = proxy.har();
        assertNotNull(har);
        String firstUrlLoaded = har.getAsJsonObject("log")
                .getAsJsonArray("entries").get(0).getAsJsonObject()
                .getAsJsonObject("request")
                .getAsJsonPrimitive("url").getAsString();
        assertEquals(url.toString(), firstUrlLoaded);
    }

    @After
    public void closeProxy() {
        proxy.close();
    }

    @AfterClass
    public static void stopProxyManager() throws Exception {
        localProxyManager.closeAll();
        localProxyManager.stop();
    }
}
