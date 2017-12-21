/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2017, Jason Gowan <gowanjason@gmail.com>
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

const
log_levels = [
    "OFF",
    "SEVERE",
    "WARNING",
    "INFO",
    "CONFIG",
    "FINE",
    "FINER",
    "FINEST",
    "ALL"
];

/**
 * (Super-simple) Logger
 *
 * @param context {String} Logger level
 */
function WebDriverLogger(log_level) {
    var _push;

    // default to no-opt
    if (log_level === "OFF" || log_levels.indexOf(log_level) === -1) {
        _push = function(_) {};
    } else {
        _push = function(arg) { this.log.push(arg); };
    }

    return {
        log: [],
        push: _push
    };
};

/**
 * Export: Create Logger with Log Level
 *
 * @param context {String} Log Level of the new Logger
 */
exports.create = function (log_level) {
    return new WebDriverLogger(log_level);
};
