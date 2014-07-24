/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.PanelDescriptor}
 */
WebInspector.TimelinePanelDescriptor = function()
{
    WebInspector.PanelDescriptor.call(this, "timeline", WebInspector.UIString("Timeline"), "TimelinePanel", "TimelinePanel.js");
}

WebInspector.TimelinePanelDescriptor.prototype = {
    registerShortcuts: function()
    {
        var section = WebInspector.shortcutsScreen.section(WebInspector.UIString("Timeline Panel"));
        section.addAlternateKeys(WebInspector.TimelinePanelDescriptor.ShortcutKeys.StartStopRecording, WebInspector.UIString("Start/stop recording"));
        if (InspectorFrontendHost.canSave())
            section.addAlternateKeys(WebInspector.TimelinePanelDescriptor.ShortcutKeys.SaveToFile, WebInspector.UIString("Save timeline data"));
        section.addAlternateKeys(WebInspector.TimelinePanelDescriptor.ShortcutKeys.LoadFromFile, WebInspector.UIString("Load timeline data"));
    },

    __proto__: WebInspector.PanelDescriptor.prototype
}

WebInspector.TimelinePanelDescriptor.ShortcutKeys = {
    StartStopRecording: [
        WebInspector.KeyboardShortcut.makeDescriptor("e", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    SaveToFile: [
        WebInspector.KeyboardShortcut.makeDescriptor("s", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    LoadFromFile: [
        WebInspector.KeyboardShortcut.makeDescriptor("o", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ]
}
