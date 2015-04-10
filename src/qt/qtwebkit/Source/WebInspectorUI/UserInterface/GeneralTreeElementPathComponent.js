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

WebInspector.GeneralTreeElementPathComponent = function(generalTreeElement, representedObject) {
    WebInspector.HierarchicalPathComponent.call(this, generalTreeElement.mainTitle, generalTreeElement.classNames, representedObject || generalTreeElement.representedObject);

    this._generalTreeElement = generalTreeElement;
    generalTreeElement.addEventListener(WebInspector.GeneralTreeElement.Event.MainTitleDidChange, this._mainTitleDidChange, this);
};

WebInspector.GeneralTreeElementPathComponent.prototype = {
    constructor: WebInspector.GeneralTreeElementPathComponent,

    // Public

    get generalTreeElement()
    {
        return this._generalTreeElement;
    },

    get previousSibling()
    {
        if (!this._generalTreeElement.previousSibling)
            return null;
        return new WebInspector.GeneralTreeElementPathComponent(this._generalTreeElement.previousSibling);
    },

    get nextSibling()
    {
        if (!this._generalTreeElement.nextSibling)
            return null;
        return new WebInspector.GeneralTreeElementPathComponent(this._generalTreeElement.nextSibling);
    },
    
    // Private
    
    _mainTitleDidChange: function(event)
    {
        this.displayName = this._generalTreeElement.mainTitle;
    }
};

WebInspector.GeneralTreeElementPathComponent.prototype.__proto__ = WebInspector.HierarchicalPathComponent.prototype;
