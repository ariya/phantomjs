/*
 * Copyright (C) 2007, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.StylePropertiesSection}
 */
WebInspector.BlankStylePropertiesSection = function(parentPane, defaultSelectorText)
{
    WebInspector.StylePropertiesSection.call(this, parentPane, {selectorText: defaultSelectorText, rule: {isViaInspector: true}}, true, false, false);
    this.element.classList.add("blank-section");
}

WebInspector.BlankStylePropertiesSection.prototype = {
    get isBlank()
    {
        return !this._normal;
    },

    expand: function()
    {
        if (!this.isBlank)
            WebInspector.StylePropertiesSection.prototype.expand.call(this);
    },

    editingSelectorCommitted: function(element, newContent, oldContent, context, moveDirection)
    {
        if (!this.isBlank) {
            WebInspector.StylePropertiesSection.prototype.editingSelectorCommitted.call(this, element, newContent, oldContent, context, moveDirection);
            return;
        }

        function successCallback(newRule, doesSelectorAffectSelectedNode)
        {
            var styleRule = { section: this, style: newRule.style, selectorText: newRule.selectorText, sourceURL: newRule.sourceURL, rule: newRule };
            this.makeNormal(styleRule);

            if (!doesSelectorAffectSelectedNode) {
                this.noAffect = true;
                this.element.classList.add("no-affect");
            }

            this._selectorRefElement.textContent = WebInspector.UIString("via inspector");
            this.expand();
            if (this.element.parentElement) // Might have been detached already.
                this._moveEditorFromSelector(moveDirection);
        }

        WebInspector.cssModel.addRule(this.pane.node.id, newContent, successCallback.bind(this), this.editingSelectorCancelled.bind(this));
    },

    editingSelectorCancelled: function()
    {
        if (!this.isBlank) {
            WebInspector.StylePropertiesSection.prototype.editingSelectorCancelled.call(this);
            return;
        }

        this.pane.removeSection(this);
    },

    makeNormal: function(styleRule)
    {
        this.element.classList.remove("blank-section");
        this.styleRule = styleRule;
        this.rule = styleRule.rule;

        // FIXME: replace this instance by a normal WebInspector.StylePropertiesSection.
        this._normal = true;
    }
}

WebInspector.BlankStylePropertiesSection.prototype.__proto__ = WebInspector.StylePropertiesSection.prototype;
