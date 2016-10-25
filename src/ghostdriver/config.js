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

// Default configuration
var defaultConfig = {
        "ip"        : "127.0.0.1",
        "port"      : "8910",
        "hub"       : null,
        "proxy"     : "org.openqa.grid.selenium.proxy.DefaultRemoteProxy",
        "version"   : "",
        "logFile"   : null,
        "logLevel"  : "INFO",
        "logColor"  : false
    },
    config = {
        "ip"        : defaultConfig.ip,
        "port"      : defaultConfig.port,
        "hub"       : defaultConfig.hub,
        "proxy"     : defaultConfig.proxy,
        "version"   : defaultConfig.version,
        "logFile"   : defaultConfig.logFile,
        "logLevel"  : defaultConfig.logLevel,
        "logColor"  : defaultConfig.logColor
    },
    logOutputFile = null,
    logger = require("./logger.js"),
    _log = logger.create("Config");

function apply () {
    // Normalise and Set Console Logging Level
    config.logLevel = config.logLevel.toUpperCase();
    if (!logger.console.LEVELS.hasOwnProperty(config.logLevel)) {
        config.logLevel = defaultConfig.logLevel;
    }
    logger.console.setLevel(logger.console.LEVELS[config.logLevel]);

    // Normalise and Set Console Color
    try {
        config.logColor = JSON.parse(config.logColor);
    } catch (e) {
        config.logColor = defaultConfig.logColor;
    }
    if (config.logColor) {
        logger.console.enableColor();
    } else {
        logger.console.disableColor();
    }

    // Add a Log File (if any)
    if (config.logFile !== null) {
        logger.addLogFile(config.logFile);
    }
}

exports.init = function(cliArgs) {
    var i, k,
        regexp = new RegExp("^--([a-z]+)=([a-z0-9_/\\\\:.]+)$", "i"),
        regexpRes;

    // Loop over all the Command Line Arguments
    // If any of the form '--param=value' is found, it's compared against
    // the 'config' object to see if 'config.param' exists.
    for (i = cliArgs.length -1; i >= 1; --i) {
        // Apply Regular Expression
        regexpRes = regexp.exec(cliArgs[i]);
        if (regexpRes !== null && regexpRes.length === 3 &&
            config.hasOwnProperty(regexpRes[1])) {
            config[regexpRes[1]] = regexpRes[2];
        }
    }

    // Apply/Normalize the Configuration before returning
    apply();

    _log.debug("config.init", JSON.stringify(config));
};

exports.get = function() {
    return config;
};
