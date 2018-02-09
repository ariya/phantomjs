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

// Init Console++
require("./third_party/console++.js");

// Constants
const
separator = " - ";

/**
 * (Super-simple) Logger
 *
 * @param context {String} Logger context
 */
function Logger (context) {
    var loggerObj, i;

    if (!context || context.length === 0) {
        throw new Error("Invalid 'context' for Logger: " + context);
    }

    loggerObj = {
        debug   : function(scope, message) {
            console.debug(context + separator +
                scope +
                (message && message.length > 0 ? separator + message : "")
            );
        },
        info    : function(scope, message) {
            console.info(context + separator +
                scope +
                (message && message.length > 0 ? separator + message : "")
            );
        },
        warn    : function(scope, message) {
            console.warn(context + separator +
                scope +
                (message && message.length > 0 ? separator + message : "")
            );
        },
        error   : function(scope, message) {
            console.error(context + separator +
                scope +
                (message && message.length > 0 ? separator + message : "")
            );
        }
    };


    return loggerObj;
}

/**
 * Export: Create Logger with Context
 *
 * @param context {String} Context of the new Logger
 */
exports.create = function (context) {
    return new Logger(context);
};

/**
 * Export: Add Log File.
 *
 * @param logFileName {String Name of the file were to output (append) the Logs.
 */
exports.addLogFile = function(logFileName) {
    var fs = require("fs"),
        f = fs.open(fs.absolute(logFileName), 'a');

    // Append line to Log File
    console.onOutput(function(msg, levelName) {
        f.writeLine(msg);
        f.flush();
    });

    // Flush the Log File when process exits
    phantom.aboutToExit.connect(f.flush);
};

/**
 * Export: Console object
 */
exports.console = console;
