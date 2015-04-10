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

WebInspector.CodeMirrorDragToAlterNumberController = function(codeMirror)
{
    this._codeMirror = codeMirror;
    this._active = false;
    this._dragging = false;
    this._enabled = false;
    this._tracksMouseClickAndDrag = false;
};

WebInspector.CodeMirrorDragToAlterNumberController.StyleClassName = "drag-to-adjust";

WebInspector.CodeMirrorDragToAlterNumberController.prototype = {
    constructor: WebInspector.CodeMirrorDragToAlterNumberController,

    // Public

    set enabled(enabled)
    {
        if (this._enabled === enabled)
            return;

        this._element = this._codeMirror.getWrapperElement();

        if (enabled) {
            this._element.addEventListener("mouseenter", this);
            this._element.addEventListener("mouseleave", this);
        } else {
            this._element.removeEventListener("mouseenter", this);
            this._element.removeEventListener("mouseleave", this);
        }
    },

    // Protected

    handleEvent: function(event)
    {
        switch(event.type) {
        case "mouseenter":
            if (!this._dragging)
                this._setActive(true);
            break;
        case "mouseleave":
            if (!this._dragging)
                this._setActive(false);
            break;
        case "mousemove":
            if (this._dragging)
                this._mouseWasDragged(event);
            else
                this._mouseMoved(event);
            break;
        case "mousedown":
            this._mouseWasPressed(event);
            break;
        case "mouseup":
            this._mouseWasReleased(event);
            break;
        case "contextmenu":
            event.preventDefault();
            break;
        }
    },

    // Private

    _setActive: function(active)
    {
        if (this._active === active || this._codeMirror.getOption("readOnly"))
            return;

        if (active) {
            WebInspector.notifications.addEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._modifiersDidChange, this);
            this._element.addEventListener("mousemove", this);
        } else {
            WebInspector.notifications.removeEventListener(WebInspector.Notification.GlobalModifierKeysDidChange, this._modifiersDidChange, this);
            this._element.removeEventListener("mousemove", this);
            this._hoveredTokenInfo = null;
            this._setTracksMouseClickAndDrag(false);
        }

        this._active = active;
    },

    _setDragging: function(dragging)
    {
        if (this._dragging === dragging)
            return;
        
        console.assert(window.event);
        if (dragging)
            WebInspector.elementDragStart(this._element, this, this, window.event, "col-resize", window);
        else
            WebInspector.elementDragEnd(window.event);

        this._dragging = dragging;
    },

    _setTracksMouseClickAndDrag: function(tracksMouseClickAndDrag)
    {
        if (this._tracksMouseClickAndDrag === tracksMouseClickAndDrag)
            return;
        
        if (tracksMouseClickAndDrag) {
            this._element.classList.add(WebInspector.CodeMirrorDragToAlterNumberController.StyleClassName);
            window.addEventListener("mousedown", this, true);
            window.addEventListener("contextmenu", this, true);
        } else {
            this._element.classList.remove(WebInspector.CodeMirrorDragToAlterNumberController.StyleClassName);
            window.removeEventListener("mousedown", this, true);
            window.removeEventListener("contextmenu", this, true);
            this._setDragging(false);
        }
        
        this._tracksMouseClickAndDrag = tracksMouseClickAndDrag;
    },

    _modifiersDidChange: function(event)
    {
        this._setTracksMouseClickAndDrag(this._hoveredTokenInfo && this._hoveredTokenInfo.containsNumber && WebInspector.modifierKeys.altKey);
    },
    
    _mouseMoved: function(event)
    {
        var position = this._codeMirror.coordsChar({left: event.pageX, top: event.pageY});
        var token = this._codeMirror.getTokenAt(position);

        if (!token || !token.type || !token.string) {
            if (this._hoveredTokenInfo)
                this._reset();
            return;
        }

        // Stop right here if we're hovering the same token as we were last time.
        if (this._hoveredTokenInfo && this._hoveredTokenInfo.line === position.line &&
            this._hoveredTokenInfo.token.start === token.start && this._hoveredTokenInfo.token.end === token.end)
            return;

        var containsNumber = token.type.indexOf("number") !== -1;
        this._hoveredTokenInfo = {
            token: token,
            line: position.line,
            containsNumber: containsNumber,
            startPosition: {
                ch: token.start,
                line: position.line
            },
            endPosition: {
                ch: token.end,
                line: position.line
            }
        };

        this._setTracksMouseClickAndDrag(containsNumber && event.altKey);
    },
    
    _mouseWasPressed: function(event)
    {
        this._lastX = event.screenX;

        this._setDragging(true);

        event.preventDefault();
        event.stopPropagation();
    },

    _mouseWasDragged: function(event)
    {
        var x = event.screenX;
        var amount = x - this._lastX;

        if (Math.abs(amount) < 1)
            return;

        this._lastX = x;

        if (event.ctrlKey)
            amount /= 10;
        else if (event.shiftKey)
            amount *= 10;

        this._codeMirror.alterNumberInRange(amount, this._hoveredTokenInfo.startPosition, this._hoveredTokenInfo.endPosition, false);

        event.preventDefault();
        event.stopPropagation();
    },

    _mouseWasReleased: function(event)
    {
        this._setDragging(false);

        event.preventDefault();
        event.stopPropagation();

        this._reset();
    },
    
    _reset: function()
    {
        this._hoveredTokenInfo = null;
        this._setTracksMouseClickAndDrag(false);
        this._element.classList.remove(WebInspector.CodeMirrorDragToAlterNumberController.StyleClassName);
    }
};

CodeMirror.defineOption("dragToAdjustNumbers", true, function(codeMirror, value, oldValue) {
    if (!codeMirror.dragToAlterNumberController)
        codeMirror.dragToAlterNumberController = new WebInspector.CodeMirrorDragToAlterNumberController(codeMirror);
    codeMirror.dragToAlterNumberController.enabled = value;
});
