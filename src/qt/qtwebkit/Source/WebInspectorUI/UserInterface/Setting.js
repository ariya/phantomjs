/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.Setting = function(name, defaultValue)
{
    this._name = name;
    this._localStorageKey = WebInspector.Setting.LocalStorageKeyPrefix + name;
    this._defaultValue = defaultValue;
}

WebInspector.Object.addConstructorFunctions(WebInspector.Setting);

WebInspector.Setting.Event = {
    Changed: "setting-changed"
};

WebInspector.Setting.LocalStorageKeyPrefix = "com.apple.WebInspector.";

WebInspector.Setting.prototype = {
    constructor: WebInspector.Setting,

    // Public

    get name()
    {
        return this._name;
    },

    get value()
    {
        if ("_value" in this)
            return this._value;

        // Make a copy of the default value so changes to object values don't modify the default value.
        this._value = JSON.parse(JSON.stringify(this._defaultValue));

        if (window.localStorage && this._localStorageKey in window.localStorage) {
            try {
                this._value = JSON.parse(window.localStorage[this._localStorageKey]);
            } catch(e) {
                delete window.localStorage[this._localStorageKey];
            }
        }

        return this._value;
    },

    set value(value)
    {
        this._value = value;

        if (window.localStorage) {
            try {
                // Use Object.shallowEqual to properly compare objects.
                if (Object.shallowEqual(this._value, this._defaultValue))
                    delete window.localStorage[this._localStorageKey];
                else
                    window.localStorage[this._localStorageKey] = JSON.stringify(this._value);
            } catch(e) {
                console.error("Error saving setting with name: " + this._name);
            }
        }

        this.dispatchEventToListeners(WebInspector.Setting.Event.Changed, this._value, {name: this._name});
    }
}

WebInspector.Setting.prototype.__proto__ = WebInspector.Object.prototype;
