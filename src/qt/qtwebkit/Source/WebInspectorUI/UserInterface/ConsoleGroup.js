/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
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

WebInspector.ConsoleGroup = function(parentGroup, isNewSession)
{
    WebInspector.Object.call(this);

    this.parentGroup = parentGroup;

    var element = document.createElement("div");
    element.className = "console-group";
    element.group = this;
    this.element = element;

    if (isNewSession)
        element.classList.add("new-session");

    var messagesElement = document.createElement("div");
    messagesElement.className = "console-group-messages";
    element.appendChild(messagesElement);
    this.messagesElement = messagesElement;
};

WebInspector.ConsoleGroup.prototype = {
    constructor: WebInspector.ConsoleGroup,

    // Public

    addMessage: function(msg)
    {
        var element = msg.toMessageElement();

        var wrapper = document.createElement("div");
        wrapper.className = WebInspector.LogContentView.ItemWrapperStyleClassName;
        wrapper.messageElement = wrapper.appendChild(element);

        if (msg.type === WebInspector.ConsoleMessage.MessageType.StartGroup || msg.type === WebInspector.ConsoleMessage.MessageType.StartGroupCollapsed) {
            this.messagesElement.parentNode.insertBefore(wrapper, this.messagesElement);
            element.addEventListener("click", this._titleClicked.bind(this));
            element.addEventListener("mousedown", this._titleMouseDown.bind(this));
            var groupElement = element.enclosingNodeOrSelfWithClass("console-group");
            if (groupElement && msg.type === WebInspector.ConsoleMessage.MessageType.StartGroupCollapsed)
                groupElement.classList.add("collapsed");
        } else
            this.messagesElement.appendChild(wrapper);
    },

    hasMessages: function()
    {
        return !!this.messagesElement.childNodes.length;
    },

    // Private

    _titleMouseDown: function(event)
    {
        event.preventDefault();
    },

    _titleClicked: function(event)
    {
        var groupTitleElement = event.target.enclosingNodeOrSelfWithClass("console-group-title");
        if (groupTitleElement) {
            var groupElement = groupTitleElement.enclosingNodeOrSelfWithClass("console-group");
            if (groupElement)
                if (groupElement.classList.contains("collapsed"))
                    groupElement.classList.remove("collapsed");
                else
                    groupElement.classList.add("collapsed");
            groupTitleElement.scrollIntoViewIfNeeded(true);
        }
    }
};

WebInspector.ConsoleGroup.prototype.__proto__ = WebInspector.Object.prototype;
