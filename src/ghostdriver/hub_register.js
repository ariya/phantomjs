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

/* generate node configuration for this node */
var nodeconf = function(ip, port, hub, proxy, version) {
        var ref$, hubHost, hubPort;

        ref$ = hub.match(/([\w\d\.]+):(\d+)/);
        hubHost = ref$[1];
        hubPort = +ref$[2]; //< ensure it's of type "number"

        var ghostdriver = ghostdriver || {};

        return {
            capabilities: [{
                browserName: "phantomjs",
                version: version,
                platform: ghostdriver.system.os.name + '-' + ghostdriver.system.os.version + '-' + ghostdriver.system.os.architecture,
                maxInstances: 1,
                seleniumProtocol: "WebDriver"
            }],
            configuration: {
                hub: hub,
                hubHost: hubHost,
                hubPort: hubPort,
                host: ip,
                port: port,
                proxy: proxy,
                // Note that multiple webdriver sessions or instances within a single
                // Ghostdriver process will interact in unexpected and undesirable ways.
                maxSession: 1,
                register: true,
                registerCycle: 5000,
                role: "wd",
                url: "http://" + ip + ":" + port,
                remoteHost: "http://" + ip + ":" + port
            }
        };
    },
    _log = require("./logger.js").create("HUB Register");

module.exports = {
    register: function(ip, port, hub, proxy, version) {
        var page;

        try {
            page = require('webpage').create();
            port = +port; //< ensure it's of type "number"
            if(!hub.match(/\/$/)) {
                hub += '/';
            }

            /* Register with selenium grid server */
            page.open(hub + 'grid/register', {
                operation: 'post',
                data: JSON.stringify(nodeconf(ip, port, hub, proxy, version)),
                headers: {
                    'Content-Type': 'application/json'
                }
            }, function(status) {
                if(status !== 'success') {
                    _log.error("register", "Unable to contact grid " + hub + ": " + status);
                    phantom.exit(1);
                }
                if(page.framePlainText !== "ok") {
                    _log.error("register", "Problem registering with grid " + hub + ": " + page.content);
                    phantom.exit(1);
                }
                _log.info("register", "Registered with grid hub: " + hub + " (" + page.framePlainText + ")");
            });
        } catch (e) {
            throw new Error("Could not register to Selenium Grid Hub: " + hub);
        }
    }
};
