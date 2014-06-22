/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.SidebarPane}
 */
WebInspector.ScopeChainSidebarPane = function()
{
    WebInspector.SidebarPane.call(this, WebInspector.UIString("Scope Variables"));
    this._sections = [];
    this._expandedSections = {};
    this._expandedProperties = [];
}

WebInspector.ScopeChainSidebarPane.prototype = {
    update: function(callFrame)
    {
        this.bodyElement.removeChildren();

        if (!callFrame) {
            var infoElement = document.createElement("div");
            infoElement.className = "info";
            infoElement.textContent = WebInspector.UIString("Not Paused");
            this.bodyElement.appendChild(infoElement);
            return;
        }

        for (var i = 0; i < this._sections.length; ++i) {
            var section = this._sections[i];
            if (!section.title)
                continue;
            if (section.expanded)
                this._expandedSections[section.title] = true;
            else
                delete this._expandedSections[section.title];
        }

        this._sections = [];

        var foundLocalScope = false;
        var scopeChain = callFrame.scopeChain;
        for (var i = 0; i < scopeChain.length; ++i) {
            var scope = scopeChain[i];
            var title = null;
            var subtitle = scope.object.description;
            var emptyPlaceholder = null;
            var extraProperties = null;
            var declarativeScope;
            
            switch (scope.type) {
                case "local":
                    foundLocalScope = true;
                    title = WebInspector.UIString("Local");
                    emptyPlaceholder = WebInspector.UIString("No Variables");
                    subtitle = null;
                    if (callFrame.this)
                        extraProperties = [ new WebInspector.RemoteObjectProperty("this", WebInspector.RemoteObject.fromPayload(callFrame.this)) ];
                    if (i == 0) {
                        var details = WebInspector.debuggerModel.debuggerPausedDetails();
                        var exception = details.reason === WebInspector.DebuggerModel.BreakReason.Exception ? details.auxData : 0;
                        if (exception) {
                            extraProperties = extraProperties || [];
                            var exceptionObject = /** @type {RuntimeAgent.RemoteObject} */ (exception);
                            extraProperties.push(new WebInspector.RemoteObjectProperty("<exception>", WebInspector.RemoteObject.fromPayload(exceptionObject)));
                        }
                    }
                    declarativeScope = true;
                    break;
                case "closure":
                    title = WebInspector.UIString("Closure");
                    emptyPlaceholder = WebInspector.UIString("No Variables");
                    subtitle = null;
                    declarativeScope = true;
                    break;
                case "catch":
                    title = WebInspector.UIString("Catch");
                    subtitle = null;
                    declarativeScope = true;
                    break;
                case "with":
                    title = WebInspector.UIString("With Block");
                    declarativeScope = false;
                    break;
                case "global":
                    title = WebInspector.UIString("Global");
                    declarativeScope = false;
                    break;
            }

            if (!title || title === subtitle)
                subtitle = null;
            
            var scopeRef;
            if (declarativeScope)
                scopeRef = new WebInspector.ScopeRef(i, callFrame.id, undefined);
            else
                scopeRef = undefined;
            

            var section = new WebInspector.ObjectPropertiesSection(WebInspector.RemoteObject.fromScopePayload(scope.object, scopeRef), title, subtitle, emptyPlaceholder, true, extraProperties, WebInspector.ScopeVariableTreeElement);
            section.editInSelectedCallFrameWhenPaused = true;
            section.pane = this;

            if (scope.type === "global")
                section.expanded = false;
            else if (!foundLocalScope || scope.type === "local" || title in this._expandedSections)
                section.expanded = true;

            this._sections.push(section);
            this.bodyElement.appendChild(section.element);
        }
    },

    __proto__: WebInspector.SidebarPane.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ObjectPropertyTreeElement}
 * @param {WebInspector.RemoteObjectProperty} property
 */
WebInspector.ScopeVariableTreeElement = function(property)
{
    WebInspector.ObjectPropertyTreeElement.call(this, property);
}

WebInspector.ScopeVariableTreeElement.prototype = {
    onattach: function()
    {
        WebInspector.ObjectPropertyTreeElement.prototype.onattach.call(this);
        if (this.hasChildren && this.propertyIdentifier in this.treeOutline.section.pane._expandedProperties)
            this.expand();
    },

    onexpand: function()
    {
        this.treeOutline.section.pane._expandedProperties[this.propertyIdentifier] = true;
    },

    oncollapse: function()
    {
        delete this.treeOutline.section.pane._expandedProperties[this.propertyIdentifier];
    },

    get propertyIdentifier()
    {
        if ("_propertyIdentifier" in this)
            return this._propertyIdentifier;
        var section = this.treeOutline.section;
        this._propertyIdentifier = section.title + ":" + (section.subtitle ? section.subtitle + ":" : "") + this.propertyPath();
        return this._propertyIdentifier;
    },

    __proto__: WebInspector.ObjectPropertyTreeElement.prototype
}
