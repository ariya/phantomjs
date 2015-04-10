/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2014, Jim Evans <james.h.evans.jr@gmail.com> - Salesforce.com
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

var ghostdriver = ghostdriver || {};

ghostdriver.Inputs = function () {
    // private:
    const
    _specialKeys = {
        '\uE000': "Escape",        // NULL
        '\uE001': "Cancel",        // Cancel
        '\uE002': "F1",            // Help
        '\uE003': "Backspace",     // Backspace
        '\uE004': "Tab",           // Tab
        '\uE005': "Clear",         // Clear
        '\uE006': "\n",
        '\uE007': "Enter",
        '\uE008': "Shift",         // Shift
        '\uE009': "Control",       // Control
        '\uE00A': "Alt",           // Alt
        '\uE00B': "Pause",         // Pause
        '\uE00C': "Escape",        // Escape
        '\uE00D': "Space",         // Space
        '\uE00E': "PageUp",        // PageUp
        '\uE00F': "PageDown",      // PageDown
        '\uE010': "End",           // End
        '\uE011': "Home",          // Home
        '\uE012': "Left",          // Left arrow
        '\uE013': "Up",            // Up arrow
        '\uE014': "Right",         // Right arrow
        '\uE015': "Down",          // Down arrow
        '\uE016': "Insert",        // Insert
        '\uE017': "Delete",        // Delete
        '\uE018': ";",     // Semicolon
        '\uE019': "=",     // Equals
        '\uE01A': "0",     // Numpad 0
        '\uE01B': "1",     // Numpad 1
        '\uE01C': "2",     // Numpad 2
        '\uE01D': "3",     // Numpad 3
        '\uE01E': "4",     // Numpad 4
        '\uE01F': "5",     // Numpad 5
        '\uE020': "6",     // Numpad 6
        '\uE021': "7",     // Numpad 7
        '\uE022': "8",     // Numpad 8
        '\uE023': "9",     // Numpad 9
        '\uE024': "*",     // Multiply
        '\uE025': "+",     // Add
        '\uE026': ",",     // Separator
        '\uE027': "-",     // Subtract
        '\uE028': ".",     // Decimal
        '\uE029': "/",      // Divide
        '\uE031': "F1",            // F1
        '\uE032': "F2",            // F2
        '\uE033': "F3",            // F3
        '\uE034': "F4",            // F4
        '\uE035': "F5",            // F5
        '\uE036': "F6",            // F6
        '\uE037': "F7",            // F7
        '\uE038': "F8",            // F8
        '\uE039': "F9",            // F9
        '\uE03A': "F10",           // F10
        '\uE03B': "F11",           // F11
        '\uE03C': "F12",           // F12
        '\uE03D': "Meta"           // Command/Meta
    },

    _implicitShiftKeys = {
        "A": "a",
        "B": "b",
        "C": "c",
        "D": "d",
        "E": "e",
        "F": "f",
        "G": "g",
        "H": "h",
        "I": "i",
        "J": "j",
        "K": "k",
        "L": "l",
        "M": "m",
        "N": "n",
        "O": "o",
        "P": "p",
        "Q": "q",
        "R": "r",
        "S": "s",
        "T": "t",
        "U": "u",
        "V": "v",
        "W": "w",
        "X": "x",
        "Y": "y",
        "Z": "z",
        "!": "1",
        "@": "2",
        "#": "3",
        "$": "4",
        "%": "5",
        "^": "6",
        "&": "7",
        "*": "8",
        "(": "9",
        ")": "0",
        "_": "-",
        "+": "=",
        "{": "[",
        "}": "]",
        "|": "\\",
        ":": ";",
        "<": ",",
        ">": ".",
        "?": "/",
        "~": "`",
        "\"": "'"
    },

    _shiftKeys = {
        "a": "A",
        "b": "B",
        "c": "C",
        "d": "D",
        "e": "E",
        "f": "F",
        "g": "G",
        "h": "H",
        "i": "I",
        "j": "J",
        "k": "K",
        "l": "L",
        "m": "M",
        "n": "N",
        "o": "O",
        "p": "P",
        "q": "Q",
        "r": "R",
        "s": "S",
        "t": "T",
        "u": "U",
        "v": "V",
        "w": "W",
        "x": "X",
        "y": "Y",
        "z": "Z",
        "1": "!",
        "2": "@",
        "3": "#",
        "4": "$",
        "5": "%",
        "6": "^",
        "7": "&",
        "8": "*",
        "9": "(",
        "0": ")",
        "-": "_",
        "=": "+",
        "[": "{",
        "]": "}",
        "\\": "|",
        ";": ":",
        ",": "<",
        ".": ">",
        "/": "?",
        "`": "~",
        "'": "\""
    },

    _modifierKeyValues = {
        "SHIFT": 0x02000000,   // A Shift key on the keyboard is pressed.
        "CONTROL": 0x04000000, // A Ctrl key on the keyboard is pressed.
        "ALT": 0x08000000,     // An Alt key on the keyboard is pressed.
        "META": 0x10000000,    // A Meta key on the keyboard is pressed.
        "NUMPAD": 0x20000000   // Keypad key.
    };

    var
    _mousePos = { x: 0, y: 0 },
    _keyboardState = {},
    _currentModifierKeys = 0,

    _isModifierKey = function (key) {
        return key === "\uE008" || key === "\uE009" || key === "\uE00A" || key === "\uE03D";
    },

    _isModifierKeyPressed = function (key) {
        return _currentModifierKeys & _modifierKeyValues[_specialKeys[key].toUpperCase()];
    },

    _sendKeys = function (session, keys) {
        var keySequence = keys.split('');
        for (var i = 0; i < keySequence.length; i++) {
            var key = keys[i];
            var actualKey = _translateKey(session, key);

            if (key === '\uE000') {
                _clearModifierKeys(session);
            } else {
                if (_isModifierKey(key)) {
                    if (_isModifierKeyPressed(key)) {
                        _keyUp(session, actualKey);
                    } else {
                        _keyDown(session, actualKey);
                    }
                } else {
                    if (_implicitShiftKeys.hasOwnProperty(actualKey)) {
                        session.getCurrentWindow().sendEvent("keydown", _translateKey(session, "\uE008"));
                        _pressKey(session, actualKey);
                        session.getCurrentWindow().sendEvent("keyup", _translateKey(session, "\uE008"));
                    } else {
                        if ((_currentModifierKeys & _modifierKeyValues.SHIFT) && _shiftKeys.hasOwnProperty(actualKey)) {
                            _pressKey(session, _shiftKeys[actualKey]);
                        } else {
                            _pressKey(session, actualKey);
                        }
                    }
                }
            }
        }
    },

    _clearModifierKeys = function (session) {
        if (_currentModifierKeys & _modifierKeyValues.SHIFT) {
            _keyUp(session, _translateKey(session, "\uE008"));
        }
        if (_currentModifierKeys & _modifierKeyValues.CONTROL) {
            _keyUp(session, _translateKey(session, "\uE009"));
        }
        if (_currentModifierKeys & _modifierKeyValues.ALT) {
            _keyUp(session, _translateKey(session, "\uE00A"));
        }
    },

    _updateModifierKeys = function (modifierKeyValue, on) {
        if (on) {
            _currentModifierKeys = _currentModifierKeys | modifierKeyValue;
        } else {
            _currentModifierKeys = _currentModifierKeys & ~modifierKeyValue;
        }
    },

    _translateKey = function (session, key) {
        var
            actualKey = key,
            phantomjskeys = session.getCurrentWindow().event.key;
        if (_specialKeys.hasOwnProperty(key)) {
            actualKey = _specialKeys[key];
            if (phantomjskeys.hasOwnProperty(actualKey)) {
                actualKey = phantomjskeys[actualKey];
            }
        }
        return actualKey;
    },

    _pressKey = function (session, key) {
        // translate WebDriver key value to key code.
        _keyEvent(session, "keypress", key);
    },

    _keyDown = function (session, key) {
        _keyEvent(session, "keydown", key);
        if (key == _translateKey(session, "\uE008")) {
            _updateModifierKeys(_modifierKeyValues.SHIFT, true);
        } else if (key == _translateKey(session, "\uE009")) {
            _updateModifierKeys(_modifierKeyValues.CONTROL, true);
        } else if (key == _translateKey(session, "\uE00A")) {
            _updateModifierKeys(_modifierKeyValues.ALT, true);
        }
    },

    _keyUp = function (session, key) {
        if (key == _translateKey(session, "\uE008")) {
            _updateModifierKeys(_modifierKeyValues.SHIFT, false);
        } else if (key == _translateKey(session, "\uE009")) {
            _updateModifierKeys(_modifierKeyValues.CONTROL, false);
        } else if (key == _translateKey(session, "\uE00A")) {
            _updateModifierKeys(_modifierKeyValues.ALT, false);
        }
        _keyEvent(session, "keyup", key);
    },

    _mouseClick = function (session, coords) {
        _mouseMove(session, coords);
        _mouseButtonEvent(session, "click", "left");
    },

    _mouseMove = function (session, coords) {
        session.getCurrentWindow().sendEvent("mousemove", coords.x, coords.y);
        _mousePos = { x: coords.x, y: coords.y };
    },

    _mouseButtonDown = function (session, button) {
        _mouseButtonEvent(session, "mousedown", button);
    },

    _mouseButtonUp = function (session, button) {
        _mouseButtonEvent(session, "mouseUp", button);
    },

    _keyEvent = function (session, eventType, keyCode) {
        eventType = eventType || "keypress";
        session.getCurrentWindow().sendEvent(eventType, keyCode, null, null, _currentModifierKeys);
    },

    _mouseButtonEvent = function (session, eventType, button) {
        button = button || "left";
        eventType = eventType || "click";
        session.getCurrentWindow().sendEvent(eventType,
            _mousePos.x, _mousePos.y, //< x, y
            button, _currentModifierKeys);
    };

    return {
        getCurrentCoordinates: function () { return _mousePos; },
        mouseClick: _mouseClick,
        mouseMove: _mouseMove,
        mouseButtonDown: _mouseButtonDown,
        mouseButtonUp: _mouseButtonUp,
        mouseButtonClick: _mouseButtonEvent,
        sendKeys: _sendKeys,
        clearModifierKeys: _clearModifierKeys
    };
};
