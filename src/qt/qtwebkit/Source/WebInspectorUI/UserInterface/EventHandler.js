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

WebInspector.EventHandler = function(delegate, eventHandlers)
{
    this._delegate = delegate;
    this._eventHandlers = eventHandlers;

    this._tracking = false;
    this._target = null;
};

WebInspector.EventHandler.prototype = {
    constructor: WebInspector.EventHandler,

    // Public

    trackEvents: function(target)
    {
        if (this._tracking && target === this._target)
            return;

        if (this._tracking && this._target)
            this.stopTrackingEvents();

        Object.getOwnPropertyNames(this._eventHandlers).forEach(function(eventName) {
            target.addEventListener(eventName, this);
        }, this);

        this._target = target;
        this._tracking = true;
    },
    
    stopTrackingEvents: function()
    {
        if (!this._tracking)
            return;

        Object.getOwnPropertyNames(this._eventHandlers).forEach(function(eventType) {
            this._target.removeEventListener(eventType, this);
        }, this);

        this._tracking = false;
    },

    handleEvent: function(event)
    {
        if (event.currentTarget !== this._target)
            return;

        var handler = this._eventHandlers[event.type];
        if (handler)
            handler.call(this._delegate, event);
    }
}

WebInspector.EventHandler.prototype.__proto__ = WebInspector.Object.prototype;
