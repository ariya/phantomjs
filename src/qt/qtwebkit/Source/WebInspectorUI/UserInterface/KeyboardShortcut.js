/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.KeyboardShortcut = function(modifiers, key, callback, targetElement)
{
    WebInspector.Object.call(this);

    console.assert(key);
    console.assert(!callback || typeof callback === "function");
    console.assert(!targetElement || targetElement instanceof Element);

    if (typeof key === "string") {
        key = key[0].toUpperCase();
        key = new WebInspector.Key(key.charCodeAt(0), key);
    }

    if (callback && !targetElement)
        targetElement = document;

    this._modifiers = modifiers || WebInspector.KeyboardShortcut.Modifier.None;
    this._key = key;
    this._targetElement = targetElement;
    this._callback = callback;
    this._disabled = false;
    this._implicitlyPreventsDefault = true;

    if (targetElement) {
        var targetKeyboardShortcuts = targetElement._keyboardShortcuts;
        if (!targetKeyboardShortcuts)
            targetKeyboardShortcuts = targetElement._keyboardShortcuts = [];

        targetKeyboardShortcuts.push(this);

        if (!WebInspector.KeyboardShortcut._registeredKeyDownListener) {
            WebInspector.KeyboardShortcut._registeredKeyDownListener = true;
            window.addEventListener("keydown", WebInspector.KeyboardShortcut._handleKeyDown);
        }
    }
};

WebInspector.KeyboardShortcut._handleKeyDown = function(event)
{
    if (event.defaultPrevented)
        return;

    for (var targetElement = event.target; targetElement; targetElement = targetElement.parentNode) {
        if (!targetElement._keyboardShortcuts)
            continue;

        for (var i = 0; i < targetElement._keyboardShortcuts.length; ++i) {
            var keyboardShortcut = targetElement._keyboardShortcuts[i];
            if (!keyboardShortcut.matchesEvent(event))
                continue;

            keyboardShortcut.callback(event, keyboardShortcut);

            if (keyboardShortcut.implicitlyPreventsDefault)
                event.preventDefault();

            return;
        }
    }
};

WebInspector.KeyboardShortcut.Modifier = {
    None: 0,
    Shift: 1,
    Control: 2,
    Option: 4,
    Command: 8
};

WebInspector.Key = function(keyCode, displayName)
{
    this._keyCode = keyCode;
    this._displayName = displayName;
};

WebInspector.Key.prototype = {
    get keyCode()
    {
        return this._keyCode;
    },

    get displayName()
    {
        return this._displayName;
    },

    toString: function()
    {
        return this._displayName;
    }
};

WebInspector.KeyboardShortcut.Key = {
    Backspace: new WebInspector.Key(8, "\u232b"),
    Tab: new WebInspector.Key(9, "\u21e5"),
    Enter: new WebInspector.Key(13, "\u21a9"),
    Escape: new WebInspector.Key(27, "\u238b"),
    Space: new WebInspector.Key(32, "Space"),
    PageUp: new WebInspector.Key(33, "\u21de"),
    PageDown: new WebInspector.Key(34, "\u21df"),
    End: new WebInspector.Key(35, "\u2198"),
    Home: new WebInspector.Key(36, "\u2196"),
    Left: new WebInspector.Key(37, "\u2190"),
    Up: new WebInspector.Key(38, "\u2191"),
    Right: new WebInspector.Key(39, "\u2192"),
    Down: new WebInspector.Key(40, "\u2193"),
    Delete: new WebInspector.Key(46, "\u2326"),
    Zero: new WebInspector.Key(48, "0"),
    F1: new WebInspector.Key(112, "F1"),
    F2: new WebInspector.Key(113, "F2"),
    F3: new WebInspector.Key(114, "F3"),
    F4: new WebInspector.Key(115, "F4"),
    F5: new WebInspector.Key(116, "F5"),
    F6: new WebInspector.Key(117, "F6"),
    F7: new WebInspector.Key(118, "F7"),
    F8: new WebInspector.Key(119, "F8"),
    F9: new WebInspector.Key(120, "F9"),
    F10: new WebInspector.Key(121, "F10"),
    F11: new WebInspector.Key(122, "F11"),
    F12: new WebInspector.Key(123, "F12"),
    Semicolon: new WebInspector.Key(186, ";"),
    Plus: new WebInspector.Key(187, "+"),
    Comma: new WebInspector.Key(188, ","),
    Minus: new WebInspector.Key(189, "-"),
    Period: new WebInspector.Key(190, "."),
    Slash: new WebInspector.Key(191, "/"),
    Apostrophe: new WebInspector.Key(192, "`"),
    SingleQuote: new WebInspector.Key(222, "\'")
};

WebInspector.KeyboardShortcut.prototype = {
    constructor: WebInspector.KeyboardShortcut,

    // Public

    get modifiers()
    {
        return this._modifiers;
    },

    get key()
    {
        return this._key;
    },

    get displayName()
    {
        var result = "";

        if (this._modifiers & WebInspector.KeyboardShortcut.Modifier.Control)
            result += "\u2303";
        if (this._modifiers & WebInspector.KeyboardShortcut.Modifier.Option)
            result += "\u2325";
        if (this._modifiers & WebInspector.KeyboardShortcut.Modifier.Shift)
            result += "\u21e7";
        if (this._modifiers & WebInspector.KeyboardShortcut.Modifier.Command)
            result += "\u2318";

        result += this._key.toString();

        return result;
    },

    get callback()
    {
        return this._callback;
    },

    get disabled()
    {
        return this._disabled;
    },

    set disabled(disabled)
    {
        this._disabled = disabled || false;
    },

    get implicitlyPreventsDefault()
    {
        return this._implicitlyPreventsDefault;
    },

    set implicitlyPreventsDefault(implicitly)
    {
        this._implicitlyPreventsDefault = implicitly;
    },

    unbind: function()
    {
        this._disabled = true;

        if (!this._targetElement)
            return;

        var targetKeyboardShortcuts = this._targetElement._keyboardShortcuts;
        if (!targetKeyboardShortcuts)
            return;

        targetKeyboardShortcuts.remove(this);
    },

    matchesEvent: function(event)
    {
        if (this._disabled)
            return false;

        if (this._key.keyCode !== event.keyCode)
            return false;

        var eventModifiers = WebInspector.KeyboardShortcut.Modifier.None;
        if (event.shiftKey)
            eventModifiers |= WebInspector.KeyboardShortcut.Modifier.Shift;
        if (event.ctrlKey)
            eventModifiers |= WebInspector.KeyboardShortcut.Modifier.Control;
        if (event.altKey)
            eventModifiers |= WebInspector.KeyboardShortcut.Modifier.Option;
        if (event.metaKey)
            eventModifiers |= WebInspector.KeyboardShortcut.Modifier.Command;
        return this._modifiers === eventModifiers;
    }
};

WebInspector.KeyboardShortcut.prototype.__proto__ = WebInspector.Object.prototype;
