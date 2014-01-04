/*
This file is part of the Console++ by Ivan De Marino <http://ivandemarino.me>.

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

if (console.LEVELS) {
    // Already loaded. No need to manipulate the "console" further.
    // NOTE: NodeJS already caches modules. This is just defensive coding.
    exports = console;
    return;
}

// private:
var _ANSICODES = {
        'reset'     : '\033[0m',
        'bold'      : '\033[1m',
        'italic'    : '\033[3m',
        'underline' : '\033[4m',
        'blink'     : '\033[5m',
        'black'     : '\033[30m',
        'red'       : '\033[31m',
        'green'     : '\033[32m',
        'yellow'    : '\033[33m',
        'blue'      : '\033[34m',
        'magenta'   : '\033[35m',
        'cyan'      : '\033[36m',
        'white'     : '\033[37m'
    },
    _LEVELS     = {
        NONE        : 0,
        OFF         : 0,    //< alias for "NONE"
        ERROR       : 1,
        WARN        : 2,
        WARNING     : 2,    //< alias for "WARN"
        INFO        : 3,
        INFORMATION : 3,    //< alias for "INFO"
        DEBUG       : 4
    },
    _LEVELS_COLOR = [   //< _LEVELS_COLOR position matches the _LEVELS values
        "red",
        "yellow",
        "cyan",
        "green"
    ],
    _LEVELS_NAME = [    //< _LEVELS_NAME position matches the _LEVELS values
        "NONE",
        "ERROR",
        "WARN ",
        "INFO ",
        "DEBUG"
    ],
    _console    = {
        error   : console.error,
        warn    : console.warn,
        info    : console.info,
        debug   : console.log,
        log     : console.log
    },
    _level = _LEVELS.DEBUG,
    _colored = true,
    _messageColored = false,
    _timed = true,
    _onOutput = null;

/**
 * Take a string and apply console ANSI colors for expressions "#color{msg}"
 * NOTE: Does nothing if "console.colored === false".
 *
 * @param str Input String
 * @returns Same string but with colors applied
 */
var _applyColors = function(str) {
    var tag = /#([a-z]+)\{|\}/,
        cstack = [],
        matches = null,
        orig = null,
        name = null,
        code = null;

    while (tag.test(str)) {
        matches = tag.exec(str);
        orig = matches[0];

        if (console.isColored()) {
            if (orig === '}') {
                cstack.pop();
            } else {
                name = matches[1];
                if (name in _ANSICODES) {
                    code = _ANSICODES[name];
                    cstack.push(code);
                }
            }

            str = str.replace(orig, _ANSICODES.reset + cstack.join(''));
        } else {
            str = str.replace(orig, '');
        }
    }
    return str;
};

/**
 * Decorate the Arguments passed to the console methods we override.
 * First element, the message, is now colored, timed and more (based on config).
 *
 * @param argsArray Array of arguments to decorate
 * @param level Logging level to apply (regulates coloring and text)
 * @returns Array of Arguments, decorated.
 */
var _decorateArgs = function(argsArray, level) {
    var args = Array.prototype.slice.call(argsArray, 1),
        msg = argsArray[0],
        levelMsg;

    if (console.isColored()) {
        levelMsg = _applyColors("#" + console.getLevelColor(level) + "{" + console.getLevelName(level) + "}");
        msg = _applyColors(msg);

        if (console.isMessageColored()) {
            msg = _applyColors("#" + console.getLevelColor(level) + "{" + msg + "}");
        }
    } else {
        levelMsg = console.getLevelName(level);
    }

    msg = _formatMessage(msg, levelMsg);

    args.splice(0, 0, msg);

    return args;
};

/**
 * Formats the Message content.
 * @param msg The message itself
 * @param levelMsg The portion of message that contains the Level (maybe colored)
 * @retuns The formatted message
 */
var _formatMessage = function(msg, levelMsg) {
    if (console.isTimestamped()) {
        return "[" + levelMsg + " - " + new Date().toJSON() + "] " + msg;
    } else {
        return "[" + levelMsg + "] " + msg;
    }
};

/**
 * Invokes the "console.onOutput()" callback, if it was set by user.
 * This is useful in case the user wants to write the console output to another media as well.
 *
 * The callback is invoked with 2 parameters:
 * - formattedMessage: formatted message, ready for output
 * - levelName: the name of the logging level, to inform the user
 *
 * @param msg The Message itself
 * @param level The Message Level (Number)
 */
var _invokeOnOutput = function(msg, level) {
    var formattedMessage,
        levelName;

    if (_onOutput !== null && typeof(_onOutput) === "function") {
        levelName = console.getLevelName(level);
        formattedMessage = _formatMessage(msg, levelName);

        _onOutput.call(null, formattedMessage, levelName);
    }
};


// public:
// CONSTANT: Logging Levels
console.LEVELS  = _LEVELS;

// Set/Get Level
console.setLevel = function(level) {
    _level = level;
};
console.getLevel = function() {
    return _level;
};
console.getLevelName = function(level) {
    return _LEVELS_NAME[typeof(level) === "undefined" ? _level : level];
};
console.getLevelColor = function(level) {
    return _LEVELS_COLOR[typeof(level) === "undefined" ? _level : level];
};
console.isLevelVisible = function(levelToCompare) {
    return _level >= levelToCompare;
};

// Enable/Disable Colored Output
console.enableColor = function() {
    _colored = true;
};
console.disableColor = function() {
    _colored = false;
};
console.isColored = function() {
    return _colored;
};

// Enable/Disable Colored Message Output
console.enableMessageColor = function() {
    _messageColored = true;
};
console.disableMessageColor = function() {
    _messageColored = false;
};
console.isMessageColored = function() {
    return _messageColored;
};

// Enable/Disable Timestamped Output
console.enableTimestamp = function() {
    _timed = true;
};
console.disableTimestamp = function() {
    _timed = false;
};
console.isTimestamped = function() {
    return _timed;
};

// Set OnOutput Callback (useful to write to file or something)
// Callback: `function(formattedMessage, levelName)`
console.onOutput = function(callback) {
    _onOutput = callback;
};

// Decodes coloring markup in string
console.str2clr = function(str) {
    return console.isColored() ? _applyColors(str): str;
};

// Overrides some key "console" Object methods
console.error = function(msg) {
    if (arguments.length > 0 && this.isLevelVisible(_LEVELS.ERROR)) {
        _console.error.apply(this, _decorateArgs(arguments, _LEVELS.ERROR));
        _invokeOnOutput(msg, _LEVELS.ERROR);
     }
};
console.warn = function(msg) {
    if (arguments.length > 0 && this.isLevelVisible(_LEVELS.WARN)) {
        _console.warn.apply(this, _decorateArgs(arguments, _LEVELS.WARN));
        _invokeOnOutput(msg, _LEVELS.WARN);
    }
};
console.info = function(msg) {
    if (arguments.length > 0 && this.isLevelVisible(_LEVELS.INFO)) {
        _console.info.apply(this, _decorateArgs(arguments, _LEVELS.INFO));
        _invokeOnOutput(msg, _LEVELS.INFO);
    }
};
console.debug = function(msg) {
    if (arguments.length > 0 && this.isLevelVisible(_LEVELS.DEBUG)) {
        _console.debug.apply(this, _decorateArgs(arguments, _LEVELS.DEBUG));
        _invokeOnOutput(msg, _LEVELS.DEBUG);
    }
};
console.log = function(msg) {
    if (arguments.length > 0) {
        _console.log.apply(this, arguments);
    }
};

exports = console;
