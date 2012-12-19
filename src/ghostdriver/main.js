/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012, Ivan De Marino <http://ivandemarino.me>
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

// Load dependencies
// NOTE: We need to provide PhantomJS with the "require" module ASAP. This is a pretty s**t way to load dependencies
var ghostdriver = {
        system  : require('system'),
        hub     : require('./hub_register'),
        version : "1.0.2"
    },
    server = require('webserver').create(),
    router,
    parseURI,
    listenOn,
    listenOnIp = "127.0.0.1",
    listenOnPort = "8910";

// Enable "strict mode" for the 'parseURI' library
parseURI = require("./third_party/parseuri.js");
parseURI.options.strictMode = true;

phantom.injectJs("session.js");
phantom.injectJs("inputs.js");
phantom.injectJs("request_handlers/request_handler.js");
phantom.injectJs("request_handlers/status_request_handler.js");
phantom.injectJs("request_handlers/shutdown_request_handler.js");
phantom.injectJs("request_handlers/session_manager_request_handler.js");
phantom.injectJs("request_handlers/session_request_handler.js");
phantom.injectJs("request_handlers/webelement_request_handler.js");
phantom.injectJs("request_handlers/router_request_handler.js");
phantom.injectJs("webelementlocator.js");

try {
    // HTTP Request Router
    router = new ghostdriver.RouterReqHand();

    // Check if parameters were given, regarding the "ip:port" to listen to
    if (ghostdriver.system.args[1]) {
        if (ghostdriver.system.args[1].indexOf(':') >= 0) {
            listenOn = ghostdriver.system.args[1].split(':');
            listenOnIp = listenOn[0];
            listenOnPort = listenOn[1];
        } else {
            listenOnPort = ghostdriver.system.args[1];
        }
    }

    // Start the server
    if (server.listen(listenOnPort, router.handle)) {
        console.log('Ghost Driver running on port ' + server.port);

        // If parameters regarding a Selenium Grid HUB were given, register to it!
        if (ghostdriver.system.args[2]) {
            ghostdriver.hub.register(listenOnIp, listenOnPort, ghostdriver.system.args[2]);
        }
    } else {
        throw new Error("ERROR: Could not start Ghost Driver");
        phantom.exit(1);
    }
} catch (e) {
    console.error(e);
    phantom.exit(1);
}
