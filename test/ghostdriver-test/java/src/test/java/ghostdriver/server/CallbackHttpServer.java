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

package ghostdriver.server;

import org.mortbay.jetty.Server;
import org.mortbay.jetty.servlet.Context;
import org.mortbay.jetty.servlet.ServletHolder;

import java.util.HashMap;
import java.util.Map;

import static java.lang.String.format;

public class CallbackHttpServer {

    protected Server server;
    protected final Map<String, HttpRequestCallback> httpReqCallbacks = new HashMap<String, HttpRequestCallback>();

    public CallbackHttpServer() {
        // Default HTTP GET request callback: returns files in "test/fixture"
        setHttpHandler("GET", new GetFixtureHttpRequestCallback());
    }

    public HttpRequestCallback getHttpHandler(String httpMethod) {
        return httpReqCallbacks.get(httpMethod.toUpperCase());
    }

    public void setHttpHandler(String httpMethod, HttpRequestCallback getHandler) {
        httpReqCallbacks.put(httpMethod.toUpperCase(), getHandler);
    }

    public void start() throws Exception {
        server = new Server(0);
        Context context = new Context(server, "/");
        addServlets(context);
        server.start();
    }

    public void stop() throws Exception {
        server.stop();
    }

    protected void addServlets(Context context) {
        context.addServlet(new ServletHolder(new CallbackServlet(this)), "/*");
    }

    public int getPort() {
        return server.getConnectors()[0].getLocalPort();
    }

    public String getBaseUrl() {
        return format("http://localhost:%d/", getPort());
    }
}
