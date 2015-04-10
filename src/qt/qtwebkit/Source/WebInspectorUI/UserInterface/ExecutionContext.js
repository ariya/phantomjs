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

WebInspector.ExecutionContext = function(id, name, isPageContext, frame)
{
    WebInspector.Object.call(this);

    console.assert(typeof id === "number" || id === WebInspector.QuickConsole.MainFrameContextExecutionIdentifier);
    console.assert(typeof name === "string");

    this._id = id;
    this._name = name;
    this._isPageContext = isPageContext || false;
    this._frame = frame || null;
};

WebInspector.ExecutionContext.supported = function()
{
    // Execution contexts were added to the Inspector protocol alongside RuntimeAgent.enable and
    // disable methods, which turn on and off sending Runtime agent execution context created events.
    // So we can feature detect support for execution contexts with these RuntimeAgent functions.
    return typeof RuntimeAgent.enable === "function";
}

WebInspector.ExecutionContext.prototype = {
    constructor: WebInspector.ExecutionContext,

    // Public

    get id()
    {
        return this._id;
    },

    get name()
    {
        return this._name;
    },

    get isPageContext()
    {
        return this._isPageContext;
    },

    get frame()
    {
        return this._frame;
    }
};

WebInspector.ExecutionContext.prototype.__proto__ = WebInspector.Object.prototype;
