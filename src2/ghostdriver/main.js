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

var server = require("webserver").create(), //< webserver
    router,                                 //< router request handler
    _log;                                   //< logger for "main.js"

// "ghostdriver" global namespace
ghostdriver = {
    system  : require("system"),
    hub     : require("./hub_register.js"),
    logger  : require("./logger.js"),
    config  : null,                         //< this will be set below
    version : "1.1.0"
};

// create logger
_log = ghostdriver.logger.create("GhostDriver");

// Initialize the configuration
require("./config.js").init(ghostdriver.system.args);
ghostdriver.config = require("./config.js").get();

// Enable "strict mode" for the 'parseURI' library
require("./third_party/parseuri.js").options.strictMode = true;

// Load all the core dependencies
// NOTE: We need to provide PhantomJS with the "require" module ASAP. This is a pretty s**t way to load dependencies
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

    // Start the server
    if (server.listen(ghostdriver.config.port, { "keepAlive" : true }, router.handle)) {
        _log.info("Main", "running on port " + server.port);

        // If a Selenium Grid HUB was provided, register to it!
        if (ghostdriver.config.hub !== null) {
            _log.info("Main", "registering to Selenium HUB"+
                " '" + ghostdriver.config.hub + "'" +
                " using '" + ghostdriver.config.ip + ":" + ghostdriver.config.port + "'");
            ghostdriver.hub.register(ghostdriver.config.ip,
                ghostdriver.config.port,
                ghostdriver.config.hub);
        }
    } else {
        throw new Error("Could not start Ghost Driver");
        phantom.exit(1);
    }
} catch (e) {
    _log.error("main.fail", JSON.stringify(e));
    phantom.exit(1);
}
