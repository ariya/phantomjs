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

WebInspector.PageObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.PageObserver.prototype = {
    constructor: WebInspector.PageObserver,

    // Events defined by the "Page" domain (see WebCore/inspector/Inspector.json).

    domContentEventFired: function(timestamp)
    {
        // Covered by Timeline "MarkDOMContent" record.
    },

    loadEventFired: function(timestamp)
    {
        WebInspector.timelineManager.pageDidLoad(timestamp);
    },

    frameNavigated: function(frame, loaderId)
    {
        WebInspector.frameResourceManager.frameDidNavigate(frame, loaderId);
    },

    frameDetached: function(frameId)
    {
        WebInspector.frameResourceManager.frameDidDetach(frameId);
    },
    
    frameStartedLoading: function(frameId)
    {
        // Not handled yet.
    },

    frameStoppedLoading: function(frameId)
    {
        // Not handled yet.
    },

    frameScheduledNavigation: function(frameId, delay)
    {
        // Not handled yet.
    },

    frameClearedScheduledNavigation: function(frameId)
    {
        // Not handled yet.
    },

    javascriptDialogOpening: function(message)
    {
        // Not handled yet.
    },

    javascriptDialogClosed: function()
    {
        // Not handled yet.
    },

    scriptsEnabled: function(enabled)
    {
        // Not handled yet.
    }
};

WebInspector.PageObserver.prototype.__proto__ = WebInspector.Object.prototype;
