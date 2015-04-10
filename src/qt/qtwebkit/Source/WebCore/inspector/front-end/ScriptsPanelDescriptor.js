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
 * @implements {WebInspector.ContextMenu.Provider}
 */
WebInspector.ScriptsPanelDescriptor = function()
{
    WebInspector.PanelDescriptor.call(this, "scripts", WebInspector.UIString("Sources"), "ScriptsPanel", "ScriptsPanel.js");
    WebInspector.ContextMenu.registerProvider(this);
}

WebInspector.ScriptsPanelDescriptor.prototype = {
    /** 
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Object} target
     */
    appendApplicableItems: function(event, contextMenu, target)
    {
        var hasApplicableItems = target instanceof WebInspector.UISourceCode;

        if (!hasApplicableItems && target instanceof WebInspector.RemoteObject) {
            var remoteObject = /** @type {WebInspector.RemoteObject} */ (target);
            if (remoteObject.type !== "function")
                return;
        }

        this.panel().appendApplicableItems(event, contextMenu, target);
    },

    registerShortcuts: function()
    {
        var section = WebInspector.shortcutsScreen.section(WebInspector.UIString("Sources Panel"));

        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.PauseContinue, WebInspector.UIString("Pause/Continue"));
        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.StepOver, WebInspector.UIString("Step over"));
        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.StepInto, WebInspector.UIString("Step into"));
        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.StepOut, WebInspector.UIString("Step out"));

        var nextAndPrevFrameKeys = WebInspector.ScriptsPanelDescriptor.ShortcutKeys.NextCallFrame.concat(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.PrevCallFrame);
        section.addRelatedKeys(nextAndPrevFrameKeys, WebInspector.UIString("Next/previous call frame"));

        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.EvaluateSelectionInConsole, WebInspector.UIString("Evaluate selection in console"));
        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.GoToMember, WebInspector.UIString("Go to member"));
        section.addAlternateKeys(WebInspector.ScriptsPanelDescriptor.ShortcutKeys.ToggleBreakpoint, WebInspector.UIString("Toggle breakpoint"));
    },

    __proto__: WebInspector.PanelDescriptor.prototype
}

WebInspector.ScriptsPanelDescriptor.ShortcutKeys = {
    PauseContinue: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F8),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Slash, WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    StepOver: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F10),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.SingleQuote, WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    StepInto: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F11),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Semicolon, WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    StepOut: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F11, WebInspector.KeyboardShortcut.Modifiers.Shift),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Semicolon, WebInspector.KeyboardShortcut.Modifiers.Shift | WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    EvaluateSelectionInConsole: [
        WebInspector.KeyboardShortcut.makeDescriptor("e", WebInspector.KeyboardShortcut.Modifiers.Shift | WebInspector.KeyboardShortcut.Modifiers.Ctrl)
    ],

    GoToMember: [
        WebInspector.KeyboardShortcut.makeDescriptor("o", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta | WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    ToggleBreakpoint: [
        WebInspector.KeyboardShortcut.makeDescriptor("b", WebInspector.KeyboardShortcut.Modifiers.CtrlOrMeta)
    ],

    NextCallFrame: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Period, WebInspector.KeyboardShortcut.Modifiers.Ctrl)
    ],

    PrevCallFrame: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Comma, WebInspector.KeyboardShortcut.Modifiers.Ctrl)
    ]
};
