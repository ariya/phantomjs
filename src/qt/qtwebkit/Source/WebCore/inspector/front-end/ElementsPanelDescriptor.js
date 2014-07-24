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
WebInspector.ElementsPanelDescriptor = function()
{
    WebInspector.PanelDescriptor.call(this, "elements", WebInspector.UIString("Elements"), "ElementsPanel", "ElementsPanel.js");
    WebInspector.ContextMenu.registerProvider(this);
}

WebInspector.ElementsPanelDescriptor.prototype = {
    /** 
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Object} target
     */
    appendApplicableItems: function(event, contextMenu, target)
    {
        if (!(target instanceof WebInspector.RemoteObject))
            return;
        var remoteObject = /** @type {WebInspector.RemoteObject} */ (target);
        if (remoteObject.subtype !== "node")
            return;
        this.panel().appendApplicableItems(event, contextMenu, target);
    },

    registerShortcuts: function()
    {
        var elementsSection = WebInspector.shortcutsScreen.section(WebInspector.UIString("Elements Panel"));

        var navigate = WebInspector.ElementsPanelDescriptor.ShortcutKeys.NavigateUp.concat(WebInspector.ElementsPanelDescriptor.ShortcutKeys.NavigateDown);
        elementsSection.addRelatedKeys(navigate, WebInspector.UIString("Navigate elements"));

        var expandCollapse = WebInspector.ElementsPanelDescriptor.ShortcutKeys.Expand.concat(WebInspector.ElementsPanelDescriptor.ShortcutKeys.Collapse);
        elementsSection.addRelatedKeys(expandCollapse, WebInspector.UIString("Expand/collapse"));

        elementsSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.EditAttribute, WebInspector.UIString("Edit attribute"));
        elementsSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.HideElement, WebInspector.UIString("Hide element"));
        elementsSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.ToggleEditAsHTML, WebInspector.UIString("Toggle edit as HTML"));

        var stylesPaneSection = WebInspector.shortcutsScreen.section(WebInspector.UIString("Styles Pane"));

        var nextPreviousProperty = WebInspector.ElementsPanelDescriptor.ShortcutKeys.NextProperty.concat(WebInspector.ElementsPanelDescriptor.ShortcutKeys.PreviousProperty);
        stylesPaneSection.addRelatedKeys(nextPreviousProperty, WebInspector.UIString("Next/previous property"));

        stylesPaneSection.addRelatedKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.IncrementValue, WebInspector.UIString("Increment value"));
        stylesPaneSection.addRelatedKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.DecrementValue, WebInspector.UIString("Decrement value"));

        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.IncrementBy10, WebInspector.UIString("Increment by %f", 10));
        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.DecrementBy10, WebInspector.UIString("Decrement by %f", 10));

        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.IncrementBy100, WebInspector.UIString("Increment by %f", 100));
        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.DecrementBy100, WebInspector.UIString("Decrement by %f", 100));

        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.IncrementBy01, WebInspector.UIString("Increment by %f", 0.1));
        stylesPaneSection.addAlternateKeys(WebInspector.ElementsPanelDescriptor.ShortcutKeys.DecrementBy01, WebInspector.UIString("Decrement by %f", 0.1));
    },

    __proto__: WebInspector.PanelDescriptor.prototype
}

WebInspector.ElementsPanelDescriptor.ShortcutKeys = {
    NavigateUp: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Up)
    ],

    NavigateDown: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Down)
    ],

    Expand: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Right)
    ],

    Collapse: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Left)
    ],

    EditAttribute: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Enter)
    ],

    HideElement: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.H)
    ],

    ToggleEditAsHTML: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.F2)
    ],

    NextProperty: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Tab)
    ],

    PreviousProperty: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Tab, WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    IncrementValue: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Up)
    ],

    DecrementValue: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Down)
    ],

    IncrementBy10: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageUp),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Up, WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    DecrementBy10: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageDown),
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.Down, WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    IncrementBy100: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageUp, WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    DecrementBy100: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageDown, WebInspector.KeyboardShortcut.Modifiers.Shift)
    ],

    IncrementBy01: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageUp, WebInspector.KeyboardShortcut.Modifiers.Alt)
    ],

    DecrementBy01: [
        WebInspector.KeyboardShortcut.makeDescriptor(WebInspector.KeyboardShortcut.Keys.PageDown, WebInspector.KeyboardShortcut.Modifiers.Alt)
    ]
};
