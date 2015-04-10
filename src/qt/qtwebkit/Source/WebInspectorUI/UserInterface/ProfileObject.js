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

WebInspector.ProfileObject = function(type, title, id, isRecording)
{
    WebInspector.Object.call(this);

    console.assert(type);
    console.assert(title);
    console.assert(id);

    this._type = type;
    this._title = title;
    this._id = id;
    this._isRecording = isRecording || false;
};

WebInspector.ProfileObject.Event = {
    FinshedRecording: "profile-object-finished-recording"
};

WebInspector.ProfileObject.prototype = {
    constructor: WebInspector.ProfileObject,
    
    get type()
    {
        return this._type;
    },

    set type(type)
    {
        this._type = type;
    },
    
    get title()
    {
        return this._title;
    },

    set title(title)
    {
        this._title = title;
    },

    get id()
    {
        return this._id;
    },

    set id(id)
    {
        this._id = id;
    },

    get recording()
    {
        return this._isRecording;
    },

    set recording(flag)
    {
        if (this._isRecording === flag)
            return;

        this._isRecording = flag;
        if (!flag)
            this.dispatchEventToListeners(WebInspector.ProfileObject.Event.FinshedRecording);
    }
};

WebInspector.ProfileObject.prototype.__proto__ = WebInspector.Object.prototype;
